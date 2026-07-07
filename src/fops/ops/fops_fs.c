#include "fops/include/fops.h"

#include <fcntl.h>

#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_statfs(const fuid_t *fuid,
                       fops_statfs_t *out_statfs)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;

    meta = NULL;
    fd = -1;

    err = fops_open_object(fuid, O_RDONLY | O_CLOEXEC, &meta, &fd,
                           FS_OP_STATFS);
    if (fs_failed(err)) {
        return err;
    }

    err = lsa_statfs(fd, out_statfs);
    fops_close_object(meta, fd);
    return err;
}

fs_error_t fops_syncfs(const fuid_t *fuid)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;

    meta = NULL;
    fd = -1;

    err = fops_open_object(fuid, O_RDONLY | O_CLOEXEC, &meta, &fd,
                           FS_OP_SYNCFS);
    if (fs_failed(err)) {
        return err;
    }

    err = lsa_syncfs(fd);
    fops_close_object(meta, fd);
    return err;
}
