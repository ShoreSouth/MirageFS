#include "lsa/include/lsa_api.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"

static lsa_ret_t lsa_validate_known_flags(fs_flags_t flags, fs_flags_t known,
                                          fs_op_t op)
{
    lsa_ret_t err;

    if ((flags & ~known) != 0U)
    {
        err = lsa_error(op, EINVAL);
        FS_LOG_DUMP_ERROR("flag check failed: flags=0x%x known=0x%x, "
                          "err=%s (0x%x)",
                          flags, known, fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

static lsa_ret_t lsa_validate_type_flags(fs_flags_t flags, fs_op_t op)
{
    lsa_ret_t err;

    if (fs_flag_test(flags, FS_FLAG_DIRECTORY) &&
        fs_flag_test(flags, FS_FLAG_REGULAR))
    {
        err = lsa_error(op, EINVAL);
        FS_LOG_DUMP_ERROR("flag check failed: DIRECTORY conflicts with "
                          "REGULAR, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

/* ============================================================
 * lookup
 * ============================================================
 */

lsa_ret_t lsa_lookup(int dirfd, const char *name, fs_flags_t flags, int *fd)
{
    int open_flags;
    int newfd;
    lsa_ret_t err;
    struct stat st;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, flags=0x%x", dirfd,
                     name ? name : "(null)", flags);

    if (name == NULL || fd == NULL)
    {
        err = lsa_error(FS_OP_LOOKUP, EINVAL);
        FS_LOG_DUMP_ERROR("lookup: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = lsa_validate_known_flags(
            flags, FS_FLAG_NOFOLLOW | FS_FLAG_DIRECTORY | FS_FLAG_REGULAR,
            FS_OP_LOOKUP);
    if (fs_failed(err))
    {
        return err;
    }

    err = lsa_validate_type_flags(flags, FS_OP_LOOKUP);
    if (fs_failed(err))
    {
        return err;
    }

    open_flags = O_PATH | O_CLOEXEC;

    if (fs_flag_test(flags, FS_FLAG_NOFOLLOW))
    {
        open_flags |= O_NOFOLLOW;
    }
    if (fs_flag_test(flags, FS_FLAG_DIRECTORY))
    {
        open_flags |= O_DIRECTORY;
    }

    newfd = openat(dirfd, name, open_flags);
    if (newfd < 0)
    {
        err = lsa_error(FS_OP_LOOKUP, errno);
        FS_LOG_DUMP_ERROR("lookup failed: name=%s, err=%s (0x%x)", name,
                          fs_error_str(err), err);
        return err;
    }

    if (fs_flag_test(flags, FS_FLAG_REGULAR))
    {
        if (fstat(newfd, &st) < 0)
        {
            err = lsa_error(FS_OP_LOOKUP, errno);
            (void)close(newfd);
            FS_LOG_DUMP_ERROR("lookup fstat failed: name=%s, "
                              "err=%s (0x%x)",
                              name, fs_error_str(err), err);
            return err;
        }

        if (!S_ISREG(st.st_mode))
        {
            err = lsa_error(FS_OP_LOOKUP,
                            S_ISDIR(st.st_mode) ? EISDIR : EINVAL);
            (void)close(newfd);
            FS_LOG_DUMP_ERROR("lookup type check failed: name=%s, "
                              "err=%s (0x%x)",
                              name, fs_error_str(err), err);
            return err;
        }
    }

    *fd = newfd;

    FS_LOG_DUMP_INFO("exit: ok, fd=%d", newfd);
    return FS_OK;
}

/* ============================================================
 * create
 * ============================================================
 */

lsa_ret_t lsa_create(int dirfd, const char *name, fs_flags_t flags, mode_t mode,
                     int *fd)
{
    int open_flags;
    int newfd;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, flags=0x%x, mode=%o", dirfd,
                     name ? name : "(null)", flags, mode);

    if (name == NULL || fd == NULL)
    {
        err = lsa_error(FS_OP_CREATE, EINVAL);
        FS_LOG_DUMP_ERROR("create: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = lsa_validate_known_flags(flags,
                                   FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE |
                                           FS_FLAG_NOFOLLOW | FS_FLAG_SYNC |
                                           FS_FLAG_DIRECT | FS_FLAG_REGULAR |
                                           FS_FLAG_TRUNCATE | FS_FLAG_APPEND,
                                   FS_OP_CREATE);
    if (fs_failed(err))
    {
        return err;
    }

    if (fs_flag_test(flags, FS_FLAG_REPLACE) &&
        fs_flag_test(flags, FS_FLAG_EXCLUSIVE))
    {
        err = lsa_error(FS_OP_CREATE, EINVAL);
        FS_LOG_DUMP_ERROR("flag check failed: REPLACE conflicts with "
                          "EXCLUSIVE, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    open_flags = O_CREAT | O_RDWR | O_CLOEXEC;

    if (!fs_flag_test(flags, FS_FLAG_REPLACE))
    {
        open_flags |= O_EXCL;
    }
    if (fs_flag_test(flags, FS_FLAG_TRUNCATE))
    {
        open_flags |= O_TRUNC;
    }
    if (fs_flag_test(flags, FS_FLAG_NOFOLLOW))
    {
        open_flags |= O_NOFOLLOW;
    }
    if (fs_flag_test(flags, FS_FLAG_SYNC))
    {
        open_flags |= O_SYNC;
    }
#ifdef O_DIRECT
    if (fs_flag_test(flags, FS_FLAG_DIRECT))
    {
        open_flags |= O_DIRECT;
    }
#endif
    if (fs_flag_test(flags, FS_FLAG_APPEND))
    {
        open_flags |= O_APPEND;
    }

    newfd = openat(dirfd, name, open_flags, mode);
    if (newfd < 0)
    {
        err = lsa_error(FS_OP_CREATE, errno);
        FS_LOG_DUMP_ERROR("create failed: name=%s, err=%s (0x%x)", name,
                          fs_error_str(err), err);
        return err;
    }

    *fd = newfd;

    FS_LOG_DUMP_INFO("exit: ok, fd=%d", newfd);
    return FS_OK;
}

/* ============================================================
 * mkdir
 * ============================================================
 */

lsa_ret_t lsa_mkdir(int dirfd, const char *name, fs_flags_t flags, mode_t mode)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, flags=0x%x, mode=%o", dirfd,
                     name ? name : "(null)", flags, mode);

    if (name == NULL)
    {
        err = lsa_error(FS_OP_MKDIR, EINVAL);
        FS_LOG_DUMP_ERROR("mkdir: invalid argument (name is NULL), "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = lsa_validate_known_flags(flags, FS_FLAG_EXCLUSIVE | FS_FLAG_DIRECTORY,
                                   FS_OP_MKDIR);
    if (fs_failed(err))
    {
        return err;
    }

    if (mkdirat(dirfd, name, mode) < 0)
    {
        err = lsa_error(FS_OP_MKDIR, errno);
        FS_LOG_DUMP_ERROR("mkdir failed: name=%s, err=%s (0x%x)", name,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * unlink
 * ============================================================
 */

lsa_ret_t lsa_unlink(int dirfd, const char *name, fs_flags_t flags)
{
    lsa_ret_t err;
    struct stat st;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, flags=0x%x", dirfd,
                     name ? name : "(null)", flags);

    if (name == NULL)
    {
        err = lsa_error(FS_OP_UNLINK, EINVAL);
        FS_LOG_DUMP_ERROR("unlink: invalid argument (name is NULL), "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = lsa_validate_known_flags(flags, FS_FLAG_NOFOLLOW | FS_FLAG_REGULAR,
                                   FS_OP_UNLINK);
    if (fs_failed(err))
    {
        return err;
    }

    if (fs_flag_test(flags, FS_FLAG_REGULAR))
    {
        err = lsa_fstatat(dirfd, name, flags & FS_FLAG_NOFOLLOW, &st);
        if (fs_failed(err))
        {
            return err;
        }
        if (!S_ISREG(st.st_mode))
        {
            err = lsa_error(FS_OP_UNLINK,
                            S_ISDIR(st.st_mode) ? EISDIR : EINVAL);
            FS_LOG_DUMP_ERROR("unlink type check failed: name=%s, "
                              "err=%s (0x%x)",
                              name, fs_error_str(err), err);
            return err;
        }
    }

    if (unlinkat(dirfd, name, 0) < 0)
    {
        err = lsa_error(FS_OP_UNLINK, errno);
        FS_LOG_DUMP_ERROR("unlink failed: name=%s, err=%s (0x%x)", name,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * rmdir
 * ============================================================
 */

lsa_ret_t lsa_rmdir(int dirfd, const char *name, fs_flags_t flags)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, flags=0x%x", dirfd,
                     name ? name : "(null)", flags);

    if (name == NULL)
    {
        err = lsa_error(FS_OP_RMDIR, EINVAL);
        FS_LOG_DUMP_ERROR("rmdir: invalid argument (name is NULL), "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = lsa_validate_known_flags(flags, FS_FLAG_DIRECTORY, FS_OP_RMDIR);
    if (fs_failed(err))
    {
        return err;
    }

    if (unlinkat(dirfd, name, AT_REMOVEDIR) < 0)
    {
        err = lsa_error(FS_OP_RMDIR, errno);
        FS_LOG_DUMP_ERROR("rmdir failed: name=%s, err=%s (0x%x)", name,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * rename
 * ============================================================
 */

lsa_ret_t lsa_rename(int old_dirfd, const char *old_name, int new_dirfd,
                     const char *new_name, fs_flags_t flags)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: old_dirfd=%d, old_name=%s, "
                     "new_dirfd=%d, new_name=%s",
                     old_dirfd, old_name ? old_name : "(null)", new_dirfd,
                     new_name ? new_name : "(null)");

    if (old_name == NULL || new_name == NULL)
    {
        err = lsa_error(FS_OP_RENAME, EINVAL);
        FS_LOG_DUMP_ERROR("rename: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (renameat(old_dirfd, old_name, new_dirfd, new_name) < 0)
    {
        err = lsa_error(FS_OP_RENAME, errno);
        FS_LOG_DUMP_ERROR("rename failed: old=%s new=%s, err=%s (0x%x)",
                          old_name, new_name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * link
 * ============================================================
 */

lsa_ret_t lsa_link(int old_dirfd, const char *old_name, int new_dirfd,
                   const char *new_name, fs_flags_t flags)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: old_dirfd=%d, old_name=%s, "
                     "new_dirfd=%d, new_name=%s",
                     old_dirfd, old_name ? old_name : "(null)", new_dirfd,
                     new_name ? new_name : "(null)");

    if (old_name == NULL || new_name == NULL)
    {
        err = lsa_error(FS_OP_LINK, EINVAL);
        FS_LOG_DUMP_ERROR("link: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (linkat(old_dirfd, old_name, new_dirfd, new_name, 0) < 0)
    {
        err = lsa_error(FS_OP_LINK, errno);
        FS_LOG_DUMP_ERROR("link failed: old=%s new=%s, err=%s (0x%x)", old_name,
                          new_name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * symlink
 * ============================================================
 */

lsa_ret_t lsa_symlink(const char *target, int dirfd, const char *name,
                      fs_flags_t flags)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: target=%s, dirfd=%d, name=%s",
                     target ? target : "(null)", dirfd, name ? name : "(null)");

    if (target == NULL || name == NULL)
    {
        err = lsa_error(FS_OP_SYMLINK, EINVAL);
        FS_LOG_DUMP_ERROR("symlink: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (symlinkat(target, dirfd, name) < 0)
    {
        err = lsa_error(FS_OP_SYMLINK, errno);
        FS_LOG_DUMP_ERROR("symlink failed: name=%s, err=%s (0x%x)", name,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * readlink
 * ============================================================ */

lsa_ret_t lsa_readlink(int dirfd, const char *name, char *buf, size_t size,
                       size_t *actual)
{
    ssize_t ret;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, buf=%p, size=%lu", dirfd,
                     name ? name : "(null)", (void *)buf, (unsigned long)size);

    if ((name == NULL) || (buf == NULL) || (actual == NULL) || (size == 0U))
    {
        err = lsa_error(FS_OP_READLINK, EINVAL);
        FS_LOG_DUMP_ERROR("readlink: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    ret = readlinkat(dirfd, name, buf, size);
    if (ret < 0)
    {
        err = lsa_error(FS_OP_READLINK, errno);
        FS_LOG_DUMP_ERROR("readlink failed: name=%s, err=%s (0x%x)", name,
                          fs_error_str(err), err);
        return err;
    }

    *actual = (size_t)ret;
    if ((size_t)ret < size)
    {
        buf[ret] = '\0';
    }

    FS_LOG_DUMP_INFO("exit: ok, actual=%lu", (unsigned long)*actual);
    return FS_OK;
}

/* ============================================================
 * mknod
 * ============================================================
 */

lsa_ret_t lsa_mknod(int dirfd, const char *name, fs_type_t type, mode_t mode,
                    const lsa_device_t *device)
{
    lsa_ret_t err;
    mode_t node_mode;
    dev_t dev;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, type=%u, mode=%o", dirfd,
                     name ? name : "(null)", (unsigned int)type, mode);

    if (name == NULL)
    {
        err = lsa_error(FS_OP_MKNOD, EINVAL);
        FS_LOG_DUMP_ERROR("mknod: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    node_mode = mode & FS_PERM_MASK;
    dev = 0;

    switch (type)
    {
    case FS_TYPE_FIFO:
        node_mode |= S_IFIFO;
        break;
    case FS_TYPE_BLK:
        if (device == NULL)
        {
            return lsa_error(FS_OP_MKNOD, EINVAL);
        }
        node_mode |= S_IFBLK;
        dev = makedev(device->major_id, device->minor_id);
        break;
    case FS_TYPE_CHR:
        if (device == NULL)
        {
            return lsa_error(FS_OP_MKNOD, EINVAL);
        }
        node_mode |= S_IFCHR;
        dev = makedev(device->major_id, device->minor_id);
        break;
    default:
        err = lsa_error(FS_OP_MKNOD, EINVAL);
        FS_LOG_DUMP_ERROR("mknod: unsupported type=%u, err=%s (0x%x)",
                          (unsigned int)type, fs_error_str(err), err);
        return err;
    }

    if (mknodat(dirfd, name, node_mode, dev) < 0)
    {
        err = lsa_error(FS_OP_MKNOD, errno);
        FS_LOG_DUMP_ERROR("mknod failed: name=%s, err=%s (0x%x)", name,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}
