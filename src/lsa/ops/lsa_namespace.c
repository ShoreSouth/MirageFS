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

    if (name == NULL ||
        fd == NULL) {

        FS_LOG_DUMP_ERROR(
                "lookup invalid argument");

        return lsa_error(
                    FS_OP_LOOKUP,
                    EINVAL);
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

        FS_LOG_DUMP_ERROR(
                "lookup failed: "
                "name=%s "
                "errno=%d(%s)",
                name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_LOOKUP,
                    errno);
    }

    *fd = newfd;

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

    if (name == NULL ||
        fd == NULL) {

        FS_LOG_DUMP_ERROR(
                "create invalid argument");

        return lsa_error(
                    FS_OP_CREATE,
                    EINVAL);
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

        FS_LOG_DUMP_ERROR(
                "create failed: "
                "name=%s "
                "errno=%d(%s)",
                name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_CREATE,
                    errno);
    }

    *fd = newfd;

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
    (void)flags;

    if (name == NULL) {

        FS_LOG_DUMP_ERROR(
                "mkdir invalid argument");

        return lsa_error(
            FS_OP_MKDIR,
            EINVAL);
    }

    if (mkdirat(
                dirfd,
                name,
                mode) < 0) {

        FS_LOG_DUMP_ERROR(
                "mkdir failed: "
                "name=%s "
                "errno=%d(%s)",
                name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_MKDIR,
                    errno);
    }

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
    (void)flags;

    if (name == NULL) {

        FS_LOG_DUMP_ERROR(
                "unlink invalid argument");

                return lsa_error(
                    FS_OP_UNLINK,
                    EINVAL);
    }

    if (unlinkat(
                dirfd,
                name,
                0) < 0) {

        FS_LOG_DUMP_ERROR(
                "unlink failed: "
                "name=%s "
                "errno=%d(%s)",
                name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_UNLINK,
                    errno);
    }

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
    (void)flags;

    if (name == NULL) {

        FS_LOG_DUMP_ERROR(
                "rmdir invalid argument");

        return lsa_error(
            FS_OP_RMDIR,
            EINVAL);
    }

    if (unlinkat(
                dirfd,
                name,
                AT_REMOVEDIR) < 0) {

        FS_LOG_DUMP_ERROR(
                "rmdir failed: "
                "name=%s "
                "errno=%d(%s)",
                name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_RMDIR,
                    errno);
    }

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
    (void)flags;

    if (old_name == NULL ||
        new_name == NULL) {

        FS_LOG_DUMP_ERROR(
            "rename invalid argument");

        return lsa_error(
                FS_OP_RENAME,
                EINVAL);
    }

    if (renameat(
                old_dirfd,
                old_name,
                new_dirfd,
                new_name) < 0) {

        FS_LOG_DUMP_ERROR(
            "rename failed: "
            "old_name=%s "
            "new_name=%s "
            "errno=%d(%s)",
            old_name,
            new_name,
            errno,
            strerror(errno));

        return lsa_error(
                    FS_OP_RENAME,
                    errno);
    }

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
    (void)flags;

    if (old_name == NULL ||
        new_name == NULL) {
 
        FS_LOG_DUMP_ERROR(
            "link invalid argument");

        return lsa_error(
                    FS_OP_LINK,
                    EINVAL);
    }

    if (linkat(
                old_dirfd,
                old_name,
                new_dirfd,
                new_name,
                0) < 0) {

        FS_LOG_DUMP_ERROR(
            "link failed: "
            "old_name=%s "
            "new_name=%s "
            "errno=%d(%s)",
            old_name,
            new_name,
            errno,
            strerror(errno));

        return lsa_error(
                    FS_OP_LINK,
                    errno);
    }

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
    (void)flags;

    if (target == NULL ||
        name == NULL) {

        FS_LOG_DUMP_ERROR(
            "symlink invalid argument");

        return lsa_error(
                    FS_OP_SYMLINK,
                    EINVAL);
    }

    if (symlinkat(
                target,
                dirfd,
                name) < 0) {

        FS_LOG_DUMP_ERROR(
            "symlink failed: "
            "target=%s "
            "name=%s "
            "errno=%d(%s)",
            target,
            name,
            errno,
            strerror(errno));

        return lsa_error(
                    FS_OP_SYMLINK,
                    errno);
    }

    return FS_OK;
}

/* ============================================================
 * mknod
 * ============================================================
 */

static int lsa_mknod_mode(
                fs_type_t type,
                mode_t perm,
                mode_t *mode)
{
    switch (type) {

    case FS_TYPE_FIFO:

        *mode = S_IFIFO | perm;
        return 0;

    case FS_TYPE_BLK:

        *mode = S_IFBLK | perm;
        return 0;

    case FS_TYPE_CHR:

        *mode = S_IFCHR | perm;
        return 0;

    default:

        return -1;
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

    if (name == NULL) {

        FS_LOG_DUMP_ERROR(
                "mknod invalid argument");

        return lsa_error(
                    FS_OP_MKNOD,
                    EINVAL);
    }

    if (lsa_mknod_mode(
                type,
                perm,
                &mode) != 0) {

        FS_LOG_DUMP_ERROR(
                "mknod invalid type=%u",
                (uint32_t)type);

        return lsa_error(
                    FS_OP_MKNOD,
                    EINVAL);
    }

    dev = 0;

    switch (type) {

    case FS_TYPE_BLK:
    case FS_TYPE_CHR:

        if (device == NULL) {

            FS_LOG_DUMP_ERROR(
                    "mknod device required");

            return lsa_error(
                        FS_OP_MKNOD,
                        EINVAL);
        }

        dev = makedev(
                    device->major_id,
                    device->minor_id);

        break;

    case FS_TYPE_FIFO:

        break;

    default:

        FS_LOG_DUMP_ERROR(
                "mknod invalid type=%u",
                (uint32_t)type);

        return lsa_error(
                    FS_OP_MKNOD,
                    EINVAL);
    }

    if (mknodat(
                dirfd,
                name,
                mode,
                dev) < 0) {

        FS_LOG_DUMP_ERROR(
                "mknod failed: "
                "name=%s "
                "type=%u "
                "errno=%d(%s)",
                name,
                (uint32_t)type,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_MKNOD,
                    errno);
    }

    return FS_OK;
}