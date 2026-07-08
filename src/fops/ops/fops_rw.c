#include "fops/include/fops.h"

#include <fcntl.h>
#include <sys/types.h>

#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_read(fops_file_t *file,
                     void *buf,
                     size_t size,
                     size_t *actual)
{
    fs_error_t err;

    err = fops_file_check(file, FS_OP_READ);
    if (fs_failed(err)) {
        return err;
    }

    return lsa_read(file->fd, buf, size, actual);
}

fs_error_t fops_write(fops_file_t *file,
                      const void *buf,
                      size_t size,
                      size_t *actual)
{
    fs_error_t err;

    err = fops_file_check(file, FS_OP_WRITE);
    if (fs_failed(err)) {
        return err;
    }

    return lsa_write(file->fd, buf, size, actual);
}

fs_error_t fops_pread(fops_file_t *file,
                      void *buf,
                      size_t size,
                      off_t offset,
                      size_t *actual)
{
    fs_error_t err;

    err = fops_file_check(file, FS_OP_READ);
    if (fs_failed(err)) {
        return err;
    }

    return lsa_pread(file->fd, buf, size, offset, actual);
}

fs_error_t fops_pwrite(fops_file_t *file,
                       const void *buf,
                       size_t size,
                       off_t offset,
                       size_t *actual)
{
    fs_error_t err;

    err = fops_file_check(file, FS_OP_WRITE);
    if (fs_failed(err)) {
        return err;
    }

    return lsa_pwrite(file->fd, buf, size, offset, actual);
}

fs_error_t fops_truncate(const fuid_t *fuid,
                         uint64_t size,
                         fs_flags_t flags)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;

    meta = NULL;
    fd = -1;

    err = fops_validate_getattr_flags(flags, FS_OP_TRUNCATE);
    if (fs_failed(err)) {
        return err;
    }

    err = fops_open_object(fuid,
                           O_RDWR | O_CLOEXEC,
                           &meta,
                           &fd,
                           FS_OP_TRUNCATE);
    if (fs_failed(err)) {
        return err;
    }

    err = lsa_ftruncate(fd, (off_t)size);
    fops_close_object(meta, fd);
    return err;
}
