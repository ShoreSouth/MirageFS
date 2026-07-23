#include "lsa/include/lsa_api.h"

#include <sys/stat.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"

/* ============================================================
 * fstat
 * ============================================================
 */

lsa_ret_t lsa_fstat(int fd, struct stat *st)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, st=%p", fd, (void *)st);

    if (st == NULL)
    {
        err = lsa_error(FS_OP_GETATTR, EINVAL);
        FS_LOG_DUMP_ERROR("fstat: invalid argument (st is NULL), "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (fstat(fd, st) < 0)
    {
        err = lsa_error(FS_OP_GETATTR, errno);
        FS_LOG_DUMP_ERROR("fstat failed: fd=%d, err=%s (0x%x)", fd,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * fstatat
 * ============================================================
 */

lsa_ret_t lsa_fstatat(int dirfd, const char *path, fs_flags_t flags,
                      struct stat *st)
{
    int stat_flags = 0;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, path=%s, flags=0x%x, st=%p", dirfd,
                     path ? path : "(null)", flags, (void *)st);

    if (path == NULL || st == NULL)
    {
        err = lsa_error(FS_OP_GETATTR, EINVAL);
        FS_LOG_DUMP_ERROR("fstatat: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (fs_flag_test(flags, FS_FLAG_NOFOLLOW))
    {
        stat_flags |= AT_SYMLINK_NOFOLLOW;
    }

    if (fstatat(dirfd, path, st, stat_flags) < 0)
    {
        err = lsa_error(FS_OP_GETATTR, errno);
        FS_LOG_DUMP_ERROR("fstatat failed: path=%s, err=%s (0x%x)", path,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * fchmod
 * ============================================================
 */

lsa_ret_t lsa_fchmod(int fd, mode_t mode)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, mode=%o", fd, mode);

    if (fchmod(fd, mode) < 0)
    {
        err = lsa_error(FS_OP_SETATTR, errno);
        FS_LOG_DUMP_ERROR("fchmod failed: fd=%d, mode=%o, err=%s (0x%x)", fd,
                          mode, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * fchown
 * ============================================================
 */

lsa_ret_t lsa_fchown(int fd, uid_t uid, gid_t gid)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, uid=%u, gid=%u", fd, (unsigned int)uid,
                     (unsigned int)gid);

    if (fchown(fd, uid, gid) < 0)
    {
        err = lsa_error(FS_OP_SETATTR, errno);
        FS_LOG_DUMP_ERROR("fchown failed: fd=%d, uid=%u, gid=%u, "
                          "err=%s (0x%x)",
                          fd, (unsigned int)uid, (unsigned int)gid,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * faccess
 * ============================================================
 */

lsa_ret_t lsa_faccess(int fd, int mode)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, mode=0x%x", fd, mode);

#ifdef AT_EMPTY_PATH
    if (faccessat(fd, "", mode, AT_EMPTY_PATH) < 0)
    {
        err = lsa_error(FS_OP_ACCESS, errno);
        FS_LOG_DUMP_ERROR("faccess failed: fd=%d, mode=0x%x, "
                          "err=%s (0x%x)",
                          fd, mode, fs_error_str(err), err);
        return err;
    }
#else
    (void)fd;
    (void)mode;
    err = lsa_error(FS_OP_ACCESS, ENOSYS);
    FS_LOG_DUMP_ERROR("faccess unsupported: err=%s (0x%x)", fs_error_str(err),
                      err);
    return err;
#endif

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}
