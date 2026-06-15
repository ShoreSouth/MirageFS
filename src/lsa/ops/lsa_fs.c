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
    if (st == NULL) {

        FS_LOG_DUMP_ERROR(
                "fstatfs invalid argument");

        return lsa_error(
                    FS_OP_STATFS,
                    EINVAL);
    }

    if (fstatfs(fd, st) < 0) {

        FS_LOG_DUMP_ERROR(
                "fstatfs failed: "
                "fd=%d "
                "errno=%d(%s)",
                fd,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_STATFS,
                    errno);
    }

    return FS_OK;
}

/* ============================================================
 * syncfs
 * ============================================================
 */

lsa_ret_t lsa_syncfs(
                int fd)
{
    if (syncfs(fd) < 0) {

        FS_LOG_DUMP_ERROR(
                "syncfs failed: "
                "fd=%d "
                "errno=%d(%s)",
                fd,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_SYNCFS,
                    errno);
    }

    return FS_OK;
}