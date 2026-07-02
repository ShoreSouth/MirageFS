#include "fsc/fsid/fsid.h"

#include <string.h>

#include "fsc/fsc_error.h"

/*
 * ============================================================
 * allocator config
 * ============================================================
 */

#define FSID_SLOT_BITS       32U
#define FSID_SLOT_MASK       0xffffffffULL
#define FSID_GENERATION_INIT 1U
#define FSID_MAX_SLOTS       4096U

/*
 * ============================================================
 * allocator state
 * ============================================================
 */

typedef struct fsid_allocator {

    fs_mutex_t lock; /* 保护 slot bitmap、generation 和 free-list */

    uint32_t free_count; /* free_stack 中可分配 slot 数量 */
    uint32_t free_stack[FSID_MAX_SLOTS]; /* 空闲 slot 栈，slot 从 1 开始 */

    uint32_t generation[FSID_MAX_SLOTS + 1U]; /* slot 当前 generation */
    uint8_t allocated[FSID_MAX_SLOTS + 1U];   /* slot 是否已分配 */

} fsid_allocator_t;

static fsid_allocator_t g_fsid_allocator;

/*
 * ============================================================
 * private helper
 * ============================================================
 */

static uint32_t fsid_slot(
                fsc_fsid_t fsid)
{
    return (uint32_t)((uint64_t)fsid & FSID_SLOT_MASK);
}

static uint32_t fsid_generation(
                fsc_fsid_t fsid)
{
    return (uint32_t)(((uint64_t)fsid) >> FSID_SLOT_BITS);
}

static fsc_fsid_t fsid_make(
                uint32_t slot,
                uint32_t generation)
{
    return (fsc_fsid_t)((((uint64_t)generation) << FSID_SLOT_BITS) |
                        (uint64_t)slot);
}

static bool fsid_slot_is_valid(
                uint32_t slot)
{
    return (slot != 0U) && (slot <= FSID_MAX_SLOTS);
}

static void fsid_allocator_reset(void)
{
    uint32_t i;

    memset(&g_fsid_allocator.free_stack,
           0,
           sizeof(g_fsid_allocator.free_stack));
    memset(&g_fsid_allocator.allocated,
           0,
           sizeof(g_fsid_allocator.allocated));

    for (i = 1U; i <= FSID_MAX_SLOTS; i++) {
        g_fsid_allocator.generation[i] = FSID_GENERATION_INIT;
        g_fsid_allocator.free_stack[i - 1U] = FSID_MAX_SLOTS - i + 1U;
    }

    g_fsid_allocator.free_count = FSID_MAX_SLOTS;
}

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

fs_error_t fsid_init(void)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter");

    memset(&g_fsid_allocator, 0, sizeof(g_fsid_allocator));

    err = fs_mutex_init(&g_fsid_allocator.lock, "fsid_allocator", 0);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_mutex_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    fsid_allocator_reset();

    FS_LOG_DUMP_INFO("exit: ok, slots=%u", FSID_MAX_SLOTS);
    return FS_OK;
}

void fsid_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    fs_mutex_destroy(&g_fsid_allocator.lock);
    memset(&g_fsid_allocator, 0, sizeof(g_fsid_allocator));

    FS_LOG_DUMP_INFO("exit: done");
}

/*
 * ============================================================
 * allocation
 * ============================================================
 */

fs_error_t fsid_alloc(
                fsc_fsid_t *fsid)
{
    fs_error_t err;
    uint32_t slot;
    uint32_t generation;

    FS_LOG_DUMP_INFO("enter: fsid=%p", (void *)fsid);

    if (fsid == NULL) {
        err = fsc_error(FSC_SUB_FSID, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: fsid is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    fs_mutex_lock(&g_fsid_allocator.lock);

    if (g_fsid_allocator.free_count == 0U) {
        fs_mutex_unlock(&g_fsid_allocator.lock);
        err = fsc_error(FSC_SUB_FSID, FS_ERRNO_ENOSPC);
        FS_LOG_DUMP_ERROR("allocate fsid failed: no free slot, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    g_fsid_allocator.free_count--;
    slot = g_fsid_allocator.free_stack[g_fsid_allocator.free_count];
    generation = g_fsid_allocator.generation[slot];

    g_fsid_allocator.allocated[slot] = 1U;
    *fsid = fsid_make(slot, generation);

    fs_mutex_unlock(&g_fsid_allocator.lock);

    FS_LOG_DUMP_INFO("exit: ok, fsid=%llu, slot=%u, generation=%u",
                     (unsigned long long)*fsid,
                     slot,
                     generation);
    return FS_OK;
}

fs_error_t fsid_free(
                fsc_fsid_t fsid)
{
    fs_error_t err;
    uint32_t slot;
    uint32_t generation;
    uint32_t next_generation;

    FS_LOG_DUMP_INFO("enter: fsid=%llu",
                     (unsigned long long)fsid);

    if (!fsid_is_valid(fsid)) {
        err = fsc_error(FSC_SUB_FSID, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid fsid, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    slot = fsid_slot(fsid);
    generation = fsid_generation(fsid);

    fs_mutex_lock(&g_fsid_allocator.lock);

    if ((g_fsid_allocator.allocated[slot] == 0U) ||
        (g_fsid_allocator.generation[slot] != generation)) {
        fs_mutex_unlock(&g_fsid_allocator.lock);
        err = fsc_error(FSC_SUB_FSID, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("free fsid failed: stale or unallocated fsid, "
                          "slot=%u, generation=%u, err=%s (0x%x)",
                          slot,
                          generation,
                          fs_error_str(err), err);
        return err;
    }

    g_fsid_allocator.allocated[slot] = 0U;

    next_generation = generation + 1U;
    if (next_generation == 0U) {
        next_generation = FSID_GENERATION_INIT;
    }
    g_fsid_allocator.generation[slot] = next_generation;

    g_fsid_allocator.free_stack[g_fsid_allocator.free_count] = slot;
    g_fsid_allocator.free_count++;

    fs_mutex_unlock(&g_fsid_allocator.lock);

    FS_LOG_DUMP_INFO("exit: ok, slot=%u, next_generation=%u",
                     slot,
                     next_generation);
    return FS_OK;
}

/*
 * ============================================================
 * value ops
 * ============================================================
 */

bool fsid_is_valid(
                fsc_fsid_t fsid)
{
    bool valid;
    uint32_t slot;
    uint32_t generation;

    slot = fsid_slot(fsid);
    generation = fsid_generation(fsid);

    valid = (fsid != FSID_INVALID) &&
            fsid_slot_is_valid(slot) &&
            (generation != 0U);

    FS_LOG_DUMP_INFO("exit: %s", valid ? "true" : "false");
    return valid;
}

uint64_t fsid_hash(
                fsc_fsid_t fsid)
{
    uint64_t value;

    value = (uint64_t)fsid;
    value ^= value >> 33;
    value *= 0xff51afd7ed558ccdULL;
    value ^= value >> 33;
    value *= 0xc4ceb9fe1a85ec53ULL;
    value ^= value >> 33;

    return value;
}
