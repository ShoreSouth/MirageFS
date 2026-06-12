#include <sys/stat.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"

/* ============================================================
 * fstat
 * ============================================================
 */

lsa_ret_t lsa_fstat(
                int fd,
                struct stat *st)
{
    if (st == NULL) {

        FS_LOG_DUMP_ERROR(
                "fstat invalid argument");

        return lsa_error(
                    FS_OP_GETATTR,
                    EINVAL);
    }

    if (fstat(fd, st) < 0) {

        FS_LOG_DUMP_ERROR(
                "fstat failed: fd=%d errno=%d(%s)",
                fd,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_GETATTR,
                    errno);
    }

    return FS_OK;
}

/* ============================================================
 * fstatat
 * ============================================================
 */

lsa_ret_t lsa_fstatat(
                int dirfd,
                const char *path,
                fs_flags_t flags,
                struct stat *st)
{
    int stat_flags = 0;

    if (path == NULL ||
        st == NULL) {

        FS_LOG_DUMP_ERROR(
                "fstatat invalid argument");

        return lsa_error(
                    FS_OP_GETATTR,
                    EINVAL);
    }

    if (fs_flag_test(flags, FS_FLAG_NOFOLLOW)) {
        stat_flags |= AT_SYMLINK_NOFOLLOW;
    }

    if (fstatat(
                dirfd,
                path,
                st,
                stat_flags) < 0) {

        FS_LOG_DUMP_ERROR(
                "fstatat failed: path=%s errno=%d(%s)",
                path,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_GETATTR,
                    errno);
    }

    return FS_OK;
}

/* ============================================================
 * fchmod
 * ============================================================
 */

lsa_ret_t lsa_fchmod(
                int fd,
                mode_t mode)
{
    if (fchmod(fd, mode) < 0) {

        FS_LOG_DUMP_ERROR(
                "fchmod failed: fd=%d mode=%o errno=%d(%s)",
                fd,
                mode,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_SETATTR,
                    errno);
    }

    return FS_OK;
}

/* ============================================================
 * fchown
 * ============================================================
 */

lsa_ret_t lsa_fchown(
                int fd,
                uid_t uid,
                gid_t gid)
{
    if (fchown(
                fd,
                uid,
                gid) < 0) {

        FS_LOG_DUMP_ERROR(
                "fchown failed: fd=%d uid=%u gid=%u errno=%d(%s)",
                fd,
                (unsigned int)uid,
                (unsigned int)gid,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_SETATTR,
                    errno);
    }

    return FS_OK;
}