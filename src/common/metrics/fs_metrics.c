#include "common/metrics/fs_metrics.h"

#include "common/os/fs_os.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

typedef struct fs_metrics_entry
{
    char name[FS_METRICS_NAME_MAX + 1U];
    char scope[FS_METRICS_SCOPE_MAX + 1U];
    atomic_uint_fast64_t count;
    atomic_uint_fast64_t errors;
    atomic_uint_fast64_t bytes;
    atomic_uint_fast64_t total_ns;
    atomic_uint_fast64_t min_ns;
    atomic_uint_fast64_t max_ns;
    atomic_uint_fast64_t inflight;
    atomic_uint_fast64_t peak_inflight;
    atomic_uint_fast64_t buckets[FS_METRICS_BUCKET_NR];
    bool detailed;
} fs_metrics_entry_t;

typedef struct fs_metrics_state
{
    fs_metrics_entry_t entries[FS_METRICS_MAX];
    atomic_uint_fast32_t mode;
    size_t count;
    bool initialized;
    bool frozen;
    pthread_mutex_t register_lock;
} fs_metrics_state_t;

static fs_metrics_state_t g_metrics;

static const uint64_t g_bucket_upper_ns[FS_METRICS_BUCKET_NR] = {
        1000ULL,       2000ULL,       5000ULL,       10000ULL,
        20000ULL,      50000ULL,      100000ULL,     250000ULL,
        500000ULL,     1000000ULL,    2500000ULL,    5000000ULL,
        10000000ULL,   50000000ULL,   250000000ULL,  UINT64_MAX,
};

static void fs_metrics_update_max(atomic_uint_fast64_t *target, uint64_t value)
{
    uint64_t current;

    current = atomic_load_explicit(target, memory_order_relaxed);
    while ((value > current) &&
           !atomic_compare_exchange_weak_explicit(
                   target, &current, value, memory_order_relaxed,
                   memory_order_relaxed))
    {
    }
}

static void fs_metrics_update_min(atomic_uint_fast64_t *target, uint64_t value)
{
    uint64_t current;

    current = atomic_load_explicit(target, memory_order_relaxed);
    while (((current == 0U) || (value < current)) &&
           !atomic_compare_exchange_weak_explicit(
                   target, &current, value, memory_order_relaxed,
                   memory_order_relaxed))
    {
    }
}

static size_t fs_metrics_bucket_index(uint64_t elapsed_ns)
{
    size_t bucket;

    for (bucket = 0U; bucket < FS_METRICS_BUCKET_NR; bucket++)
    {
        if (elapsed_ns <= g_bucket_upper_ns[bucket])
        {
            return bucket;
        }
    }

    return FS_METRICS_BUCKET_NR - 1U;
}

void fs_metrics_init(fs_metrics_mode_t mode)
{
    if (g_metrics.initialized)
    {
        return;
    }

    memset(&g_metrics, 0, sizeof(g_metrics));
    (void)pthread_mutex_init(&g_metrics.register_lock, NULL);
    atomic_init(&g_metrics.mode,
                fs_metrics_mode_is_valid(mode) ? (uint32_t)mode
                                               : FS_METRICS_OFF);
    g_metrics.initialized = true;
}

void fs_metrics_deinit(void)
{
    if (!g_metrics.initialized)
    {
        return;
    }

    (void)pthread_mutex_destroy(&g_metrics.register_lock);
    memset(&g_metrics, 0, sizeof(g_metrics));
}

bool fs_metrics_mode_is_valid(fs_metrics_mode_t mode)
{
    return (mode >= FS_METRICS_OFF) && (mode < FS_METRICS_MODE_MAX);
}

const char *fs_metrics_mode_to_str(fs_metrics_mode_t mode)
{
    static const char *names[FS_METRICS_MODE_MAX] = {
            [FS_METRICS_OFF] = "off",
            [FS_METRICS_CORE] = "core",
            [FS_METRICS_DETAILED] = "detailed",
    };

    return fs_metrics_mode_is_valid(mode) ? names[mode] : "unknown";
}

fs_metrics_mode_t fs_metrics_get_mode(void)
{
    if (!g_metrics.initialized)
    {
        return FS_METRICS_OFF;
    }

    return (fs_metrics_mode_t)atomic_load_explicit(&g_metrics.mode,
                                                   memory_order_relaxed);
}

void fs_metrics_set_mode(fs_metrics_mode_t mode)
{
    if (g_metrics.initialized && fs_metrics_mode_is_valid(mode))
    {
        atomic_store_explicit(&g_metrics.mode, (uint32_t)mode,
                              memory_order_relaxed);
    }
}

bool fs_metrics_register(const char *scope, const char *name, bool detailed,
                         fs_metric_id_t *id_out)
{
    fs_metrics_entry_t *entry;
    size_t index;
    bool registered;

    if (id_out != NULL)
    {
        *id_out = FS_METRIC_ID_INVALID;
    }
    if (!g_metrics.initialized || (scope == NULL) || (name == NULL) ||
        (id_out == NULL))
    {
        return false;
    }

    registered = false;
    (void)pthread_mutex_lock(&g_metrics.register_lock);
    if (!g_metrics.frozen && (g_metrics.count < FS_METRICS_MAX))
    {
        index = g_metrics.count++;
        entry = &g_metrics.entries[index];
        (void)snprintf(entry->scope, sizeof(entry->scope), "%s", scope);
        (void)snprintf(entry->name, sizeof(entry->name), "%s", name);
        entry->detailed = detailed;
        *id_out = (fs_metric_id_t)index;
        registered = true;
    }
    (void)pthread_mutex_unlock(&g_metrics.register_lock);

    return registered;
}

void fs_metrics_freeze(void)
{
    if (g_metrics.initialized)
    {
        g_metrics.frozen = true;
    }
}

fs_metrics_token_t fs_metrics_begin(fs_metric_id_t id)
{
    fs_metrics_token_t token;
    fs_metrics_entry_t *entry;
    fs_metrics_mode_t mode;
    uint64_t inflight;

    token.start_ns = 0U;
    token.id = FS_METRIC_ID_INVALID;
    token.active = false;

    if (!g_metrics.initialized)
    {
        return token;
    }

    mode = fs_metrics_get_mode();
    if ((mode == FS_METRICS_OFF) || (id >= g_metrics.count))
    {
        return token;
    }

    entry = &g_metrics.entries[id];
    if (entry->detailed && (mode != FS_METRICS_DETAILED))
    {
        return token;
    }

    inflight = atomic_fetch_add_explicit(&entry->inflight, 1U,
                                         memory_order_relaxed) +
               1U;
    fs_metrics_update_max(&entry->peak_inflight, inflight);

    token.start_ns = fs_get_monotonic_ns();
    token.id = id;
    token.active = true;
    return token;
}

void fs_metrics_end(fs_metrics_token_t token, bool success, uint64_t bytes)
{
    fs_metrics_entry_t *entry;
    uint64_t elapsed_ns;
    size_t bucket;

    if (!token.active || !g_metrics.initialized ||
        (token.id >= g_metrics.count))
    {
        return;
    }

    elapsed_ns = fs_get_monotonic_ns() - token.start_ns;
    entry = &g_metrics.entries[token.id];
    bucket = fs_metrics_bucket_index(elapsed_ns);

    atomic_fetch_add_explicit(&entry->count, 1U, memory_order_relaxed);
    atomic_fetch_add_explicit(&entry->bytes, bytes, memory_order_relaxed);
    atomic_fetch_add_explicit(&entry->total_ns, elapsed_ns,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&entry->buckets[bucket], 1U,
                              memory_order_relaxed);
    if (!success)
    {
        atomic_fetch_add_explicit(&entry->errors, 1U, memory_order_relaxed);
    }
    fs_metrics_update_min(&entry->min_ns, elapsed_ns);
    fs_metrics_update_max(&entry->max_ns, elapsed_ns);
    atomic_fetch_sub_explicit(&entry->inflight, 1U, memory_order_relaxed);
}

void fs_metrics_snapshot(fs_metrics_snapshot_t *snapshot)
{
    size_t metric_index;
    size_t bucket;

    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->captured_realtime_ns = fs_get_time_ns();
    snapshot->captured_monotonic_ns = fs_get_monotonic_ns();
    if (!g_metrics.initialized)
    {
        return;
    }

    snapshot->count = g_metrics.count;
    for (metric_index = 0U; metric_index < snapshot->count; metric_index++)
    {
        const fs_metrics_entry_t *entry = &g_metrics.entries[metric_index];
        fs_metrics_value_t *value = &snapshot->values[metric_index];

        (void)snprintf(value->scope, sizeof(value->scope), "%s",
                       entry->scope);
        (void)snprintf(value->name, sizeof(value->name), "%s", entry->name);
        value->count = atomic_load_explicit(&entry->count,
                                            memory_order_relaxed);
        value->errors = atomic_load_explicit(&entry->errors,
                                             memory_order_relaxed);
        value->bytes = atomic_load_explicit(&entry->bytes,
                                            memory_order_relaxed);
        value->total_ns = atomic_load_explicit(&entry->total_ns,
                                               memory_order_relaxed);
        value->min_ns = atomic_load_explicit(&entry->min_ns,
                                             memory_order_relaxed);
        value->max_ns = atomic_load_explicit(&entry->max_ns,
                                             memory_order_relaxed);
        value->inflight = atomic_load_explicit(&entry->inflight,
                                               memory_order_relaxed);
        value->peak_inflight = atomic_load_explicit(&entry->peak_inflight,
                                                    memory_order_relaxed);
        value->detailed = entry->detailed;
        for (bucket = 0U; bucket < FS_METRICS_BUCKET_NR; bucket++)
        {
            value->buckets[bucket] = atomic_load_explicit(
                    &entry->buckets[bucket], memory_order_relaxed);
        }
    }
}

uint64_t fs_metrics_bucket_upper_ns(size_t bucket)
{
    return (bucket < FS_METRICS_BUCKET_NR) ? g_bucket_upper_ns[bucket]
                                           : UINT64_MAX;
}

uint64_t fs_metrics_percentile_ns(const fs_metrics_value_t *value,
                                  uint32_t percentile)
{
    uint64_t target;
    uint64_t seen;
    size_t bucket;

    if ((value == NULL) || (value->count == 0U) || (percentile == 0U) ||
        (percentile > 100U))
    {
        return 0U;
    }

    target = (value->count * percentile + 99U) / 100U;
    seen = 0U;
    for (bucket = 0U; bucket < FS_METRICS_BUCKET_NR; bucket++)
    {
        seen += value->buckets[bucket];
        if (seen >= target)
        {
            return g_bucket_upper_ns[bucket];
        }
    }

    return value->max_ns;
}
