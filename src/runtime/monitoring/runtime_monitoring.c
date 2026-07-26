#include "runtime/internal/runtime_monitoring.h"

#include "common/metrics/fs_metrics.h"
#include "common/os/fs_os.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define RUNTIME_MONITORING_SESSION_MAX 32U
#define RUNTIME_MONITORING_RUN_ID_MAX 63U
#define RUNTIME_MONITORING_PATH_MAX 511U

typedef struct runtime_monitoring_session
{
    uint64_t id;
    fsc_fsid_t fsid;
    char name[FSC_NAMESPACE_NAME_MAX];
    uint64_t started_realtime_ns;
    uint64_t started_monotonic_ns;
    uint64_t ended_realtime_ns;
    uint64_t ended_monotonic_ns;
    fs_metrics_snapshot_t start_snapshot;
    fs_metrics_snapshot_t end_snapshot;
} runtime_monitoring_session_t;

typedef struct runtime_monitoring_state
{
    bool initialized;
    bool session_active;
    FsMetricsConfig_t config;
    char run_id[RUNTIME_MONITORING_RUN_ID_MAX + 1U];
    uint64_t started_realtime_ns;
    uint64_t started_monotonic_ns;
    uint64_t ended_realtime_ns;
    uint64_t ended_monotonic_ns;
    uint64_t next_session_id;
    size_t session_count;
    size_t dropped_sessions;
    runtime_monitoring_session_t sessions[RUNTIME_MONITORING_SESSION_MAX];
    runtime_monitoring_session_t active_session;
    fs_metrics_snapshot_t final_snapshot;
    fs_metrics_snapshot_t *history;
    size_t history_count;
    size_t history_next;
    bool sampler_started;
    bool sampler_stop;
    pthread_t sampler_thread;
    pthread_mutex_t sampler_lock;
    pthread_cond_t sampler_cond;
} runtime_monitoring_state_t;

static runtime_monitoring_state_t g_monitoring;

static void runtime_monitoring_history_append(
        const fs_metrics_snapshot_t *snapshot)
{
    if ((g_monitoring.history == NULL) ||
        (g_monitoring.config.history_capacity == 0U))
    {
        return;
    }

    g_monitoring.history[g_monitoring.history_next] = *snapshot;
    g_monitoring.history_next =
            (g_monitoring.history_next + 1U) %
            g_monitoring.config.history_capacity;
    if (g_monitoring.history_count < g_monitoring.config.history_capacity)
    {
        g_monitoring.history_count++;
    }
}

static void *runtime_monitoring_sampler_main(void *arg)
{
    struct timespec wakeup;
    fs_metrics_snapshot_t snapshot;

    (void)arg;
    (void)pthread_mutex_lock(&g_monitoring.sampler_lock);
    while (!g_monitoring.sampler_stop)
    {
        (void)clock_gettime(CLOCK_REALTIME, &wakeup);
        wakeup.tv_sec +=
                (time_t)(g_monitoring.config.sample_interval_ms / 1000U);
        wakeup.tv_nsec +=
                (long)(g_monitoring.config.sample_interval_ms % 1000U) *
                1000000L;
        if (wakeup.tv_nsec >= 1000000000L)
        {
            wakeup.tv_sec++;
            wakeup.tv_nsec -= 1000000000L;
        }
        (void)pthread_cond_timedwait(&g_monitoring.sampler_cond,
                                     &g_monitoring.sampler_lock, &wakeup);
        if (g_monitoring.sampler_stop)
        {
            break;
        }

        fs_metrics_snapshot(&snapshot);
        runtime_monitoring_history_append(&snapshot);
    }
    (void)pthread_mutex_unlock(&g_monitoring.sampler_lock);
    return NULL;
}

static void runtime_monitoring_sampler_start(void)
{
    size_t history_bytes;

    if ((g_monitoring.config.mode == FS_METRICS_OFF) ||
        (g_monitoring.config.history_capacity == 0U) ||
        (g_monitoring.config.sample_interval_ms == 0U))
    {
        return;
    }

    history_bytes = (size_t)g_monitoring.config.history_capacity *
                    sizeof(fs_metrics_snapshot_t);
    g_monitoring.history = calloc(1U, history_bytes);
    if (g_monitoring.history == NULL)
    {
        return;
    }

    (void)pthread_mutex_init(&g_monitoring.sampler_lock, NULL);
    (void)pthread_cond_init(&g_monitoring.sampler_cond, NULL);
    if (pthread_create(&g_monitoring.sampler_thread, NULL,
                       runtime_monitoring_sampler_main, NULL) != 0)
    {
        (void)pthread_cond_destroy(&g_monitoring.sampler_cond);
        (void)pthread_mutex_destroy(&g_monitoring.sampler_lock);
        free(g_monitoring.history);
        g_monitoring.history = NULL;
        return;
    }
    g_monitoring.sampler_started = true;
}

static void runtime_monitoring_sampler_stop(void)
{
    if (!g_monitoring.sampler_started)
    {
        return;
    }

    (void)pthread_mutex_lock(&g_monitoring.sampler_lock);
    g_monitoring.sampler_stop = true;
    (void)pthread_cond_signal(&g_monitoring.sampler_cond);
    (void)pthread_mutex_unlock(&g_monitoring.sampler_lock);
    (void)pthread_join(g_monitoring.sampler_thread, NULL);
    (void)pthread_cond_destroy(&g_monitoring.sampler_cond);
    (void)pthread_mutex_destroy(&g_monitoring.sampler_lock);
    g_monitoring.sampler_started = false;
}

static void runtime_monitoring_format_time(uint64_t realtime_ns, char *buf,
                                           size_t size)
{
    time_t seconds;
    struct tm local_time;
    char date[32];

    seconds = (time_t)(realtime_ns / 1000000000ULL);
    (void)localtime_r(&seconds, &local_time);
    (void)strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%S%z", &local_time);
    (void)snprintf(buf, size, "%s", date);
}

static uint64_t runtime_monitoring_delta(uint64_t end, uint64_t start)
{
    return (end >= start) ? (end - start) : 0U;
}

static fs_metrics_value_t runtime_monitoring_value_delta(
        const fs_metrics_value_t *end, const fs_metrics_value_t *start)
{
    fs_metrics_value_t value;
    size_t bucket;

    memset(&value, 0, sizeof(value));
    (void)snprintf(value.name, sizeof(value.name), "%s", end->name);
    (void)snprintf(value.scope, sizeof(value.scope), "%s", end->scope);
    value.count = runtime_monitoring_delta(end->count, start->count);
    value.errors = runtime_monitoring_delta(end->errors, start->errors);
    value.bytes = runtime_monitoring_delta(end->bytes, start->bytes);
    value.total_ns = runtime_monitoring_delta(end->total_ns, start->total_ns);
    /*
     * 累计 min/max 不能做差得到会话窗口值；窗口分位数仍可由 bucket
     * 增量计算。V1 对会话窗口不伪造 min/max。
     */
    value.min_ns = 0U;
    value.max_ns = 0U;
    value.inflight = end->inflight;
    value.peak_inflight = 0U;
    value.detailed = end->detailed;
    for (bucket = 0U; bucket < FS_METRICS_BUCKET_NR; bucket++)
    {
        value.buckets[bucket] = runtime_monitoring_delta(
                end->buckets[bucket], start->buckets[bucket]);
    }
    return value;
}

static void runtime_monitoring_write_metric_json(
        FILE *file, const fs_metrics_value_t *value, uint64_t duration_ns,
        bool last)
{
    double duration_seconds;
    double rate;
    double throughput;
    uint64_t average_ns;

    duration_seconds = (duration_ns == 0U)
                               ? 0.0
                               : (double)duration_ns / 1000000000.0;
    rate = (duration_seconds == 0.0)
                   ? 0.0
                   : (double)value->count / duration_seconds;
    throughput = (duration_seconds == 0.0)
                         ? 0.0
                         : (double)value->bytes / duration_seconds;
    average_ns =
            (value->count == 0U) ? 0U : value->total_ns / value->count;

    (void)fprintf(
            file,
            "    {\"scope\":\"%s\",\"operation\":\"%s\","
            "\"count\":%" PRIu64 ",\"errors\":%" PRIu64
            ",\"bytes\":%" PRIu64 ",\"iops\":%.3f,"
            "\"throughput_bytes_per_sec\":%.3f,"
            "\"latency_ns\":{\"avg\":%" PRIu64 ",\"min\":%" PRIu64
            ",\"max\":%" PRIu64 ",\"p50\":%" PRIu64
            ",\"p95\":%" PRIu64 ",\"p99\":%" PRIu64 "},"
            "\"inflight\":%" PRIu64 ",\"peak_inflight\":%" PRIu64 "}%s\n",
            value->scope, value->name, value->count, value->errors,
            value->bytes, rate, throughput, average_ns, value->min_ns,
            value->max_ns, fs_metrics_percentile_ns(value, 50U),
            fs_metrics_percentile_ns(value, 95U),
            fs_metrics_percentile_ns(value, 99U), value->inflight,
            value->peak_inflight, last ? "" : ",");
}

static void runtime_monitoring_snapshot_totals(
        const fs_metrics_snapshot_t *snapshot, uint64_t *operations,
        uint64_t *errors, uint64_t *read_bytes, uint64_t *write_bytes)
{
    size_t metric_index;

    *operations = 0U;
    *errors = 0U;
    *read_bytes = 0U;
    *write_bytes = 0U;
    for (metric_index = 0U; metric_index < snapshot->count; metric_index++)
    {
        const fs_metrics_value_t *value = &snapshot->values[metric_index];

        if (strcmp(value->scope, "filesystem") != 0)
        {
            continue;
        }
        *operations += value->count;
        *errors += value->errors;
        if (strcmp(value->name, "READ") == 0)
        {
            *read_bytes += value->bytes;
        }
        else if (strcmp(value->name, "WRITE") == 0)
        {
            *write_bytes += value->bytes;
        }
    }
}

static bool runtime_monitoring_write_json(const char *path)
{
    FILE *file;
    char started[40];
    char ended[40];
    uint64_t duration_ns;
    size_t metric_index;
    size_t session_index;
    size_t history_index;

    file = fopen(path, "w");
    if (file == NULL)
    {
        return false;
    }

    runtime_monitoring_format_time(g_monitoring.started_realtime_ns, started,
                                   sizeof(started));
    runtime_monitoring_format_time(g_monitoring.ended_realtime_ns, ended,
                                   sizeof(ended));
    duration_ns = runtime_monitoring_delta(g_monitoring.ended_monotonic_ns,
                                           g_monitoring.started_monotonic_ns);

    (void)fprintf(file,
                  "{\n  \"schema_version\":\"miragefs.metrics.v1\",\n"
                  "  \"run_id\":\"%s\",\n  \"started_at\":\"%s\",\n"
                  "  \"ended_at\":\"%s\",\n  \"duration_ns\":%" PRIu64
                  ",\n  \"metrics_mode\":\"%s\",\n"
                  "  \"dropped_sessions\":%zu,\n  \"metrics\":[\n",
                  g_monitoring.run_id, started, ended, duration_ns,
                  fs_metrics_mode_to_str(g_monitoring.config.mode),
                  g_monitoring.dropped_sessions);

    for (metric_index = 0U;
         metric_index < g_monitoring.final_snapshot.count; metric_index++)
    {
        runtime_monitoring_write_metric_json(
                file, &g_monitoring.final_snapshot.values[metric_index],
                duration_ns,
                metric_index + 1U == g_monitoring.final_snapshot.count);
    }

    (void)fprintf(file, "  ],\n  \"sessions\":[\n");
    for (session_index = 0U; session_index < g_monitoring.session_count;
         session_index++)
    {
        const runtime_monitoring_session_t *session =
                &g_monitoring.sessions[session_index];
        uint64_t session_duration = runtime_monitoring_delta(
                session->ended_monotonic_ns, session->started_monotonic_ns);

        runtime_monitoring_format_time(session->started_realtime_ns, started,
                                       sizeof(started));
        runtime_monitoring_format_time(session->ended_realtime_ns, ended,
                                       sizeof(ended));
        (void)fprintf(
                file,
                "    {\"session_id\":%" PRIu64 ",\"fsid\":%" PRIu64
                ",\"name\":\"%s\",\"started_at\":\"%s\","
                "\"ended_at\":\"%s\",\"duration_ns\":%" PRIu64
                ",\"metrics\":[\n",
                session->id, (uint64_t)session->fsid, session->name, started,
                ended, session_duration);

        for (metric_index = 0U; metric_index < session->end_snapshot.count;
             metric_index++)
        {
            fs_metrics_value_t value = runtime_monitoring_value_delta(
                    &session->end_snapshot.values[metric_index],
                    &session->start_snapshot.values[metric_index]);
            runtime_monitoring_write_metric_json(
                    file, &value, session_duration,
                    metric_index + 1U == session->end_snapshot.count);
        }
        (void)fprintf(file, "    ]}%s\n",
                      session_index + 1U == g_monitoring.session_count ? ""
                                                                       : ",");
    }
    (void)fprintf(file, "  ],\n  \"history\":[\n");
    for (history_index = 0U; history_index < g_monitoring.history_count;
         history_index++)
    {
        size_t physical_index =
                (g_monitoring.history_next +
                 g_monitoring.config.history_capacity -
                 g_monitoring.history_count + history_index) %
                g_monitoring.config.history_capacity;
        const fs_metrics_snapshot_t *snapshot =
                &g_monitoring.history[physical_index];
        uint64_t operations;
        uint64_t errors;
        uint64_t read_bytes;
        uint64_t write_bytes;

        runtime_monitoring_snapshot_totals(snapshot, &operations, &errors,
                                           &read_bytes, &write_bytes);
        (void)fprintf(
                file,
                "    {\"captured_at_ns\":%" PRIu64
                ",\"operations\":%" PRIu64 ",\"errors\":%" PRIu64
                ",\"read_bytes\":%" PRIu64 ",\"write_bytes\":%" PRIu64
                "}%s\n",
                snapshot->captured_realtime_ns, operations, errors, read_bytes,
                write_bytes,
                history_index + 1U == g_monitoring.history_count ? "" : ",");
    }
    (void)fprintf(file, "  ]\n}\n");
    (void)fclose(file);
    return true;
}

static bool runtime_monitoring_write_html(const char *path)
{
    FILE *file;
    char started[40];
    char ended[40];
    double duration_seconds;
    uint64_t total_operations;
    uint64_t total_errors;
    uint64_t read_bytes;
    uint64_t write_bytes;
    size_t metric_index;
    size_t session_index;

    file = fopen(path, "w");
    if (file == NULL)
    {
        return false;
    }

    total_operations = 0U;
    total_errors = 0U;
    read_bytes = 0U;
    write_bytes = 0U;
    for (metric_index = 0U;
         metric_index < g_monitoring.final_snapshot.count; metric_index++)
    {
        const fs_metrics_value_t *value =
                &g_monitoring.final_snapshot.values[metric_index];
        if (strcmp(value->scope, "filesystem") == 0)
        {
            total_operations += value->count;
            total_errors += value->errors;
            if (strcmp(value->name, "READ") == 0)
            {
                read_bytes += value->bytes;
            }
            else if (strcmp(value->name, "WRITE") == 0)
            {
                write_bytes += value->bytes;
            }
        }
    }

    runtime_monitoring_format_time(g_monitoring.started_realtime_ns, started,
                                   sizeof(started));
    runtime_monitoring_format_time(g_monitoring.ended_realtime_ns, ended,
                                   sizeof(ended));
    duration_seconds =
            (double)runtime_monitoring_delta(g_monitoring.ended_monotonic_ns,
                                             g_monitoring.started_monotonic_ns) /
            1000000000.0;

    (void)fprintf(
            file,
            "<!doctype html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
            "<meta name=\"viewport\" content=\"width=device-width,"
            "initial-scale=1\"><title>MirageFS Performance Report</title>"
            "<style>body{font:14px system-ui;margin:32px;background:#f6f8fb;"
            "color:#172033}h1{margin-bottom:4px}.sub{color:#64748b}"
            ".cards{display:grid;grid-template-columns:repeat(auto-fit,"
            "minmax(160px,1fr));gap:12px;margin:24px 0}.card,table{background:"
            "white;border:1px solid #dfe5ee;border-radius:10px}.card{padding:"
            "16px}.value{font-size:24px;font-weight:700;margin-top:8px}"
            "table{width:100%%;border-collapse:collapse;overflow:hidden}"
            "th,td{padding:9px 12px;text-align:right;border-bottom:1px solid "
            "#edf0f5}th:first-child,td:first-child,th:nth-child(2),"
            "td:nth-child(2){text-align:left}th{background:#eef3f9}"
            "</style></head><body><h1>MirageFS Performance Report</h1>"
            "<div class=\"sub\">Run %s · %s — %s · mode %s</div>"
            "<div class=\"cards\"><div class=\"card\">Duration"
            "<div class=\"value\">%.3f s</div></div><div class=\"card\">"
            "Filesystem operations<div class=\"value\">%" PRIu64
            "</div></div><div class=\"card\">Errors<div class=\"value\">%" PRIu64
            "</div></div><div class=\"card\">Read / Write"
            "<div class=\"value\">%.2f / %.2f MiB</div></div></div>"
            "<h2>Operation metrics</h2><table><thead><tr><th>Scope</th>"
            "<th>Operation</th><th>Count</th><th>Errors</th><th>Bytes</th>"
            "<th>Avg μs</th><th>P95 μs</th><th>P99 μs</th><th>Max μs</th>"
            "</tr></thead><tbody>",
            g_monitoring.run_id, started, ended,
            fs_metrics_mode_to_str(g_monitoring.config.mode), duration_seconds,
            total_operations, total_errors,
            (double)read_bytes / (1024.0 * 1024.0),
            (double)write_bytes / (1024.0 * 1024.0));

    for (metric_index = 0U;
         metric_index < g_monitoring.final_snapshot.count; metric_index++)
    {
        const fs_metrics_value_t *value =
                &g_monitoring.final_snapshot.values[metric_index];
        uint64_t average_ns =
                value->count == 0U ? 0U : value->total_ns / value->count;

        if (value->count == 0U)
        {
            continue;
        }
        (void)fprintf(
                file,
                "<tr><td>%s</td><td>%s</td><td>%" PRIu64 "</td><td>%" PRIu64
                "</td><td>%" PRIu64 "</td><td>%.3f</td><td>%.3f</td>"
                "<td>%.3f</td><td>%.3f</td></tr>",
                value->scope, value->name, value->count, value->errors,
                value->bytes, (double)average_ns / 1000.0,
                (double)fs_metrics_percentile_ns(value, 95U) / 1000.0,
                (double)fs_metrics_percentile_ns(value, 99U) / 1000.0,
                (double)value->max_ns / 1000.0);
    }

    (void)fprintf(file, "</tbody></table><h2>Filesystem sessions</h2>"
                        "<table><thead><tr><th>Session</th><th>FSID</th>"
                        "<th>Name</th><th>Duration ms</th></tr></thead><tbody>");
    for (session_index = 0U; session_index < g_monitoring.session_count;
         session_index++)
    {
        const runtime_monitoring_session_t *session =
                &g_monitoring.sessions[session_index];
        double session_ms =
                (double)runtime_monitoring_delta(session->ended_monotonic_ns,
                                                 session->started_monotonic_ns) /
                1000000.0;

        (void)fprintf(file,
                      "<tr><td>%" PRIu64 "</td><td>%" PRIu64
                      "</td><td>%s</td><td>%.3f</td></tr>",
                      session->id, (uint64_t)session->fsid, session->name,
                      session_ms);
    }

    (void)fprintf(file,
                  "</tbody></table><p class=\"sub\">JSON is the canonical "
                  "machine-readable report. Percentiles are histogram "
                  "approximations.</p></body></html>\n");
    (void)fclose(file);
    return true;
}

static void runtime_monitoring_write_reports(void)
{
    char json_path[RUNTIME_MONITORING_PATH_MAX + 1U];
    char html_path[RUNTIME_MONITORING_PATH_MAX + 1U];

    if (!g_monitoring.config.report_enable ||
        (g_monitoring.config.mode == FS_METRICS_OFF))
    {
        return;
    }

    if ((mkdir(g_monitoring.config.report_directory, 0755) < 0) &&
        (errno != EEXIST))
    {
        (void)fprintf(stderr, "MirageFS: cannot create metrics report dir %s\n",
                      g_monitoring.config.report_directory);
        return;
    }

    if (g_monitoring.config.report_json_enable)
    {
        (void)snprintf(json_path, sizeof(json_path), "%s/miragefs-%s.json",
                       g_monitoring.config.report_directory,
                       g_monitoring.run_id);
        if (runtime_monitoring_write_json(json_path))
        {
            (void)printf("performance report: %s\n", json_path);
        }
    }
    if (g_monitoring.config.report_html_enable)
    {
        (void)snprintf(html_path, sizeof(html_path), "%s/miragefs-%s.html",
                       g_monitoring.config.report_directory,
                       g_monitoring.run_id);
        if (runtime_monitoring_write_html(html_path))
        {
            (void)printf("performance report: %s\n", html_path);
        }
    }
}

void runtime_monitoring_init(const FsMetricsConfig_t *config)
{
    if ((config == NULL) || g_monitoring.initialized)
    {
        return;
    }

    memset(&g_monitoring, 0, sizeof(g_monitoring));
    g_monitoring.config = *config;
    g_monitoring.started_realtime_ns = fs_get_time_ns();
    g_monitoring.started_monotonic_ns = fs_get_monotonic_ns();
    g_monitoring.next_session_id = 1U;
    (void)snprintf(g_monitoring.run_id, sizeof(g_monitoring.run_id),
                   "%" PRIu64 "-%ld",
                   (uint64_t)(g_monitoring.started_realtime_ns / 1000000ULL),
                   (long)getpid());
    g_monitoring.initialized = true;
    runtime_monitoring_sampler_start();
}

void runtime_monitoring_session_begin(fsc_fsid_t fsid, const char *name)
{
    runtime_monitoring_session_t *session;

    if (!g_monitoring.initialized || (name == NULL))
    {
        return;
    }

    runtime_monitoring_session_end();
    session = &g_monitoring.active_session;
    memset(session, 0, sizeof(*session));
    session->id = g_monitoring.next_session_id++;
    session->fsid = fsid;
    (void)snprintf(session->name, sizeof(session->name), "%s", name);
    session->started_realtime_ns = fs_get_time_ns();
    session->started_monotonic_ns = fs_get_monotonic_ns();
    fs_metrics_snapshot(&session->start_snapshot);
    g_monitoring.session_active = true;
}

void runtime_monitoring_session_end(void)
{
    runtime_monitoring_session_t *session;

    if (!g_monitoring.initialized || !g_monitoring.session_active)
    {
        return;
    }

    session = &g_monitoring.active_session;
    session->ended_realtime_ns = fs_get_time_ns();
    session->ended_monotonic_ns = fs_get_monotonic_ns();
    fs_metrics_snapshot(&session->end_snapshot);
    if (g_monitoring.session_count < RUNTIME_MONITORING_SESSION_MAX)
    {
        g_monitoring.sessions[g_monitoring.session_count++] = *session;
    }
    else
    {
        g_monitoring.dropped_sessions++;
    }
    memset(session, 0, sizeof(*session));
    g_monitoring.session_active = false;
}

void runtime_monitoring_deinit(void)
{
    if (!g_monitoring.initialized)
    {
        return;
    }

    runtime_monitoring_sampler_stop();
    runtime_monitoring_session_end();
    g_monitoring.ended_realtime_ns = fs_get_time_ns();
    g_monitoring.ended_monotonic_ns = fs_get_monotonic_ns();
    fs_metrics_snapshot(&g_monitoring.final_snapshot);
    runtime_monitoring_history_append(&g_monitoring.final_snapshot);
    runtime_monitoring_write_reports();
    free(g_monitoring.history);
    g_monitoring.history = NULL;
    memset(&g_monitoring, 0, sizeof(g_monitoring));
}
