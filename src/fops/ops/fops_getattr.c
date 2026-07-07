#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_getattr(const fuid_t *fuid,
                        fs_flags_t flags,
                        fops_attr_t *out_attr)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;
    struct stat st;

    FS_LOG_DUMP_INFO("enter: fuid=%p flags=0x%x out_attr=%p",
                     (const void *)fuid, flags, (void *)out_attr);

    meta = NULL;
    fd = -1;

    if (out_attr != NULL) {
        memset(out_attr, 0, sizeof(*out_attr));
    }

    if ((fuid == NULL) || (out_attr == NULL)) {
        err = fops_error(FS_OP_GETATTR, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid getattr args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    err = fops_validate_getattr_flags(flags, FS_OP_GETATTR);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_open_object(fuid,
                           O_PATH | O_CLOEXEC,
                           &meta,
                           &fd,
                           FS_OP_GETATTR);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_fstat(fd, &st);
    if (fs_failed(err)) {
        goto out;
    }

    fops_attr_from_stat(out_attr, &st);
    err = fops_check_type_flags(out_attr->type, flags, FS_OP_GETATTR);

out:
    fops_close_object(meta, fd);
    FS_LOG_DUMP_INFO("exit: err=%s (0x%x)", fs_error_str(err), err);
    return err;
}