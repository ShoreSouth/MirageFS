#include <fcntl.h>
#include <unistd.h>

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
        return lsa_error(
                    FS_OP_MKDIR,
                    EINVAL);
    }

    if (mkdirat(
                dirfd,
                name,
                mode) < 0) {
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
        return lsa_error(
                    FS_OP_UNLINK,
                    EINVAL);
    }

    if (unlinkat(
                dirfd,
                name,
                0) < 0) {
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
        return lsa_error(
                    FS_OP_RMDIR,
                    EINVAL);
    }

    if (unlinkat(
                dirfd,
                name,
                AT_REMOVEDIR) < 0) {
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
        return lsa_error(
                    FS_OP_RENAME,
                    EINVAL);
    }

    if (renameat(
                old_dirfd,
                old_name,
                new_dirfd,
                new_name) < 0) {
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
        return lsa_error(
                    FS_OP_SYMLINK,
                    EINVAL);
    }

    if (symlinkat(
                target,
                dirfd,
                name) < 0) {
        return lsa_error(
                    FS_OP_SYMLINK,
                    errno);
    }

    return FS_OK;
}