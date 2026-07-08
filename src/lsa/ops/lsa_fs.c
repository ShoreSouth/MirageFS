#include "lsa/include/lsa_api.h"

#include <errno.h>
#include <string.h>
#include <sys/statfs.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"

/* ============================================================
 * statfs
 * ============================================================
 */

lsa_ret_t lsa_statfs(
                int fd,
                struct statfs *st)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, st=%p", fd, (void *)st);

    if (st == NULL) {
        err = lsa_error(FS_OP_STATFS, EINVAL);
        FS_LOG_DUMP_ERROR("statfs: invalid argument (st is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (fstatfs(fd, st) < 0) {
        err = lsa_error(FS_OP_STATFS, errno);
        FS_LOG_DUMP_ERROR("statfs failed: fd=%d, err=%s (0x%x)",
                          fd, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * syncfs
 * ============================================================
 */

lsa_ret_t lsa_syncfs(
                int fd)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d", fd);

    if (syncfs(fd) < 0) {
        err = lsa_error(FS_OP_SYNCFS, errno);
        FS_LOG_DUMP_ERROR("syncfs failed: fd=%d, err=%s (0x%x)",
                          fd, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}
