#include <fcntl.h>
#include <unistd.h>
#include <sys/sysmacros.h>

#include "common/fs_common.h"
#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"

/* ============================================================
 * lookup
 * ============================================================
 */

lsa_ret_t lsa_lookup(
                int dirfd,
                const char *name,
                fs_flags_t flags,
                int *fd)
{
    int open_flags;
    int newfd;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, flags=0x%x",
                     dirfd, name ? name : "(null)", flags);

    if (name == NULL ||
        fd == NULL) {

        err = lsa_error(FS_OP_LOOKUP, EINVAL);
        FS_LOG_DUMP_ERROR("lookup: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    open_flags = O_PATH;

    if (fs_flag_test(flags, FS_FLAG_NOFOLLOW)) {
        open_flags |= O_NOFOLLOW;
    }

    newfd = openat(
                    dirfd,
                    name,
                    open_flags);

    if (newfd < 0) {
        err = lsa_error(FS_OP_LOOKUP, errno);
        FS_LOG_DUMP_ERROR("lookup failed: name=%s, err=%s (0x%x)",
                          name, fs_error_str(err), err);
        return err;
    }

    *fd = newfd;

    FS_LOG_DUMP_INFO("exit: ok, fd=%d", newfd);
    return FS_OK;
}

/* ============================================================
 * create
 * ============================================================
 */

lsa_ret_t lsa_create(
                int dirfd,
                const char *name,
                fs_flags_t flags,
                mode_t mode,
                int *fd)
{
    int open_flags;
    int newfd;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, flags=0x%x, mode=%o",
                     dirfd, name ? name : "(null)", flags, mode);

    if (name == NULL ||
        fd == NULL) {

        err = lsa_error(FS_OP_CREATE, EINVAL);
        FS_LOG_DUMP_ERROR("create: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    open_flags = O_CREAT | O_RDWR;

    if (fs_flag_test(flags, FS_FLAG_REPLACE)) {
        open_flags |= O_TRUNC;
    } else {
        open_flags |= O_EXCL;
    }

    newfd = openat(
                    dirfd,
                    name,
                    open_flags,
                    mode);

    if (newfd < 0) {
        err = lsa_error(FS_OP_CREATE, errno);
        FS_LOG_DUMP_ERROR("create failed: name=%s, err=%s (0x%x)",
                          name, fs_error_str(err), err);
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

lsa_ret_t lsa_mkdir(
                int dirfd,
                const char *name,
                fs_flags_t flags,
                mode_t mode)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, mode=%o",
                     dirfd, name ? name : "(null)", mode);

    if (name == NULL) {
        err = lsa_error(FS_OP_MKDIR, EINVAL);
        FS_LOG_DUMP_ERROR("mkdir: invalid argument (name is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (mkdirat(
                dirfd,
                name,
                mode) < 0) {
        err = lsa_error(FS_OP_MKDIR, errno);
        FS_LOG_DUMP_ERROR("mkdir failed: name=%s, err=%s (0x%x)",
                          name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * unlink
 * ============================================================
 */

lsa_ret_t lsa_unlink(
                int dirfd,
                const char *name,
                fs_flags_t flags)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s",
                     dirfd, name ? name : "(null)");

    if (name == NULL) {
        err = lsa_error(FS_OP_UNLINK, EINVAL);
        FS_LOG_DUMP_ERROR("unlink: invalid argument (name is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (unlinkat(
                dirfd,
                name,
                0) < 0) {
        err = lsa_error(FS_OP_UNLINK, errno);
        FS_LOG_DUMP_ERROR("unlink failed: name=%s, err=%s (0x%x)",
                          name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * rmdir
 * ============================================================
 */

lsa_ret_t lsa_rmdir(
                int dirfd,
                const char *name,
                fs_flags_t flags)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s",
                     dirfd, name ? name : "(null)");

    if (name == NULL) {
        err = lsa_error(FS_OP_RMDIR, EINVAL);
        FS_LOG_DUMP_ERROR("rmdir: invalid argument (name is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (unlinkat(
                dirfd,
                name,
                AT_REMOVEDIR) < 0) {
        err = lsa_error(FS_OP_RMDIR, errno);
        FS_LOG_DUMP_ERROR("rmdir failed: name=%s, err=%s (0x%x)",
                          name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * rename
 * ============================================================
 */

lsa_ret_t lsa_rename(
                int old_dirfd,
                const char *old_name,
                int new_dirfd,
                const char *new_name,
                fs_flags_t flags)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: old_dirfd=%d, old_name=%s, "
                     "new_dirfd=%d, new_name=%s",
                     old_dirfd, old_name ? old_name : "(null)",
                     new_dirfd, new_name ? new_name : "(null)");

    if (old_name == NULL ||
        new_name == NULL) {

        err = lsa_error(FS_OP_RENAME, EINVAL);
        FS_LOG_DUMP_ERROR("rename: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (renameat(
                old_dirfd,
                old_name,
                new_dirfd,
                new_name) < 0) {
        err = lsa_error(FS_OP_RENAME, errno);
        FS_LOG_DUMP_ERROR("rename failed: old_name=%s, new_name=%s, "
                          "err=%s (0x%x)",
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

lsa_ret_t lsa_link(
                int old_dirfd,
                const char *old_name,
                int new_dirfd,
                const char *new_name,
                fs_flags_t flags)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: old_dirfd=%d, old_name=%s, "
                     "new_dirfd=%d, new_name=%s",
                     old_dirfd, old_name ? old_name : "(null)",
                     new_dirfd, new_name ? new_name : "(null)");

    if (old_name == NULL ||
        new_name == NULL) {

        err = lsa_error(FS_OP_LINK, EINVAL);
        FS_LOG_DUMP_ERROR("link: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (linkat(
                old_dirfd,
                old_name,
                new_dirfd,
                new_name,
                0) < 0) {
        err = lsa_error(FS_OP_LINK, errno);
        FS_LOG_DUMP_ERROR("link failed: old_name=%s, new_name=%s, "
                          "err=%s (0x%x)",
                          old_name, new_name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * symlink
 * ============================================================
 */

lsa_ret_t lsa_symlink(
                const char *target,
                int dirfd,
                const char *name,
                fs_flags_t flags)
{
    lsa_ret_t err;

    (void)flags;

    FS_LOG_DUMP_INFO("enter: target=%s, dirfd=%d, name=%s",
                     target ? target : "(null)",
                     dirfd, name ? name : "(null)");

    if (target == NULL ||
        name == NULL) {

        err = lsa_error(FS_OP_SYMLINK, EINVAL);
        FS_LOG_DUMP_ERROR("symlink: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (symlinkat(
                target,
                dirfd,
                name) < 0) {
        err = lsa_error(FS_OP_SYMLINK, errno);
        FS_LOG_DUMP_ERROR("symlink failed: target=%s, name=%s, "
                          "err=%s (0x%x)",
                          target, name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * mknod helper
 * ============================================================
 */

static fs_error_t lsa_mknod_mode(
                fs_type_t type,
                mode_t perm,
                mode_t *mode)
{
    switch (type) {

    case FS_TYPE_FIFO:

        *mode = S_IFIFO | perm;
        return FS_OK;

    case FS_TYPE_BLK:

        *mode = S_IFBLK | perm;
        return FS_OK;

    case FS_TYPE_CHR:

        *mode = S_IFCHR | perm;
        return FS_OK;

    default:

        return lsa_error(FS_OP_MKNOD, EINVAL);
    }
}

/* ============================================================
 * mknod
 * ============================================================
 */

lsa_ret_t lsa_mknod(
                int dirfd,
                const char *name,
                fs_type_t type,
                mode_t perm,
                const lsa_device_t *device)
{
    mode_t mode;
    dev_t dev;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, name=%s, type=%u, perm=%o",
                     dirfd, name ? name : "(null)",
                     (uint32_t)type, perm);

    if (name == NULL) {
        err = lsa_error(FS_OP_MKNOD, EINVAL);
        FS_LOG_DUMP_ERROR("mknod: invalid argument (name is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    err = lsa_mknod_mode(type, perm, &mode);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("mknod: invalid type=%u, err=%s (0x%x)",
                          (uint32_t)type, fs_error_str(err), err);
        return err;
    }

    dev = 0;

    switch (type) {

    case FS_TYPE_BLK:
    case FS_TYPE_CHR:

        if (device == NULL) {
            err = lsa_error(FS_OP_MKNOD, EINVAL);
            FS_LOG_DUMP_ERROR("mknod: device required for blk/chr, "
                              "err=%s (0x%x)", fs_error_str(err), err);
            return err;
        }

        dev = makedev(
                    device->major_id,
                    device->minor_id);

        break;

    case FS_TYPE_FIFO:

        break;

    default:

        err = lsa_error(FS_OP_MKNOD, EINVAL);
        FS_LOG_DUMP_ERROR("mknod: invalid type=%u, err=%s (0x%x)",
                          (uint32_t)type, fs_error_str(err), err);
        return err;
    }

    if (mknodat(
                dirfd,
                name,
                mode,
                dev) < 0) {
        err = lsa_error(FS_OP_MKNOD, errno);
        FS_LOG_DUMP_ERROR("mknod failed: name=%s, type=%u, err=%s (0x%x)",
                          name, (uint32_t)type, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}
