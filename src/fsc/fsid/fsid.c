#include "fsc/fsid/fsid.h"
#include "fsc/fsc_error.h"

/*
 * ============================================================
 * global allocator state
 * ============================================================
 */

static fs_atomic64_t g_fsid_next;

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

fs_error_t fsid_init(void)
{
    FS_LOG_DUMP_INFO("enter");

    fs_atomic64_init(&g_fsid_next, 1);

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void fsid_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    fs_atomic64_store(&g_fsid_next, 1);

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
    int64_t value;

    FS_LOG_DUMP_INFO("enter: fsid=%p", (void *)fsid);

    if (fsid == NULL) {
        err = fsc_error(FSC_SUB_FSID, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: fsid is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    value = fs_atomic64_inc(&g_fsid_next);
    if (value <= 1) {
        err = fsc_error(FSC_SUB_FSID, FS_ERRNO_EOVERFLOW);
        FS_LOG_DUMP_ERROR("allocate fsid failed: overflow, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    *fsid = (fsc_fsid_t)(value - 1);

    FS_LOG_DUMP_INFO("exit: ok, fsid=%llu",
                     (unsigned long long)*fsid);
    return FS_OK;
}

fs_error_t fsid_free(
                fsc_fsid_t fsid)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter: fsid=%llu",
                     (unsigned long long)fsid);

    if (!fsid_is_valid(fsid)) {
        err = fsc_error(FSC_SUB_FSID, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid fsid, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
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

    valid = (fsid != FSID_INVALID);

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
