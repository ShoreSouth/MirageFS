#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_getxattr(const fuid_t *fuid,
                         const char *name,
                         void *value,
                         size_t size,
                         size_t *actual)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;

    meta = NULL;
    fd = -1;

    err = fops_open_object(fuid, O_RDONLY | O_CLOEXEC, &meta, &fd,
                           FS_OP_GETXATTR);
    if (fs_failed(err)) {
        return err;
    }

    err = lsa_getxattr(fd, name, value, size, actual);
    fops_close_object(meta, fd);
    return err;
}

fs_error_t fops_setxattr(const fuid_t *fuid,
                         const char *name,
                         const void *value,
                         size_t size,
                         fs_flags_t flags)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;

    meta = NULL;
    fd = -1;

    err = fops_validate_xattr_flags(flags, FS_OP_SETXATTR);
    if (fs_failed(err)) {
        return err;
    }

    err = fops_open_object(fuid, O_RDWR | O_CLOEXEC, &meta, &fd,
                           FS_OP_SETXATTR);
    if (fs_failed(err)) {
        return err;
    }

    err = lsa_setxattr(fd, name, value, size, flags);
    fops_close_object(meta, fd);
    return err;
}

fs_error_t fops_listxattr(const fuid_t *fuid,
                          char *list,
                          size_t size,
                          size_t *actual)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;

    meta = NULL;
    fd = -1;

    err = fops_open_object(fuid, O_RDONLY | O_CLOEXEC, &meta, &fd,
                           FS_OP_LISTXATTR);
    if (fs_failed(err)) {
        return err;
    }

    err = lsa_listxattr(fd, list, size, actual);
    fops_close_object(meta, fd);
    return err;
}

fs_error_t fops_removexattr(const fuid_t *fuid,
                            const char *name)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;

    meta = NULL;
    fd = -1;

    err = fops_open_object(fuid, O_RDWR | O_CLOEXEC, &meta, &fd,
                           FS_OP_REMOVEXATTR);
    if (fs_failed(err)) {
        return err;
    }

    err = lsa_removexattr(fd, name);
    fops_close_object(meta, fd);
    return err;
}
