#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_getattr(const fuid_t *fuid, fs_flags_t flags,
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

    if (out_attr != NULL)
    {
        memset(out_attr, 0, sizeof(*out_attr));
    }

    if ((fuid == NULL) || (out_attr == NULL))
    {
        err = fops_error(FS_OP_GETATTR, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid getattr args, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

    err = fops_validate_getattr_flags(flags, FS_OP_GETATTR);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_open_object(fuid, O_PATH | O_CLOEXEC, &meta, &fd, FS_OP_GETATTR);
    if (fs_failed(err))
    {
        goto out;
    }

    err = lsa_fstat(fd, &st);
    if (fs_failed(err))
    {
        goto out;
    }

    fops_attr_from_stat(out_attr, &st);
    err = fops_check_type_flags(out_attr->type, flags, FS_OP_GETATTR);

out:
    fops_close_object(meta, fd);
    FS_LOG_DUMP_INFO("exit: err=%s (0x%x)", fs_error_str(err), err);
    return err;
}

fs_error_t fops_setattr(const fuid_t *fuid, const fops_setattr_t *attr,
                        fs_flags_t flags)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;
    int open_flags;
    struct stat st;

    meta = NULL;
    fd = -1;

    if ((fuid == NULL) || (attr == NULL))
    {
        return fops_error(FS_OP_SETATTR, EINVAL);
    }

    err = fops_validate_setattr_flags(flags, FS_OP_SETATTR);
    if (fs_failed(err))
    {
        return err;
    }

    if ((attr->valid_mask & ~(FOPS_SETATTR_MODE | FOPS_SETATTR_UID |
                              FOPS_SETATTR_GID | FOPS_SETATTR_SIZE)) != 0U)
    {
        return fops_error(FS_OP_SETATTR, EINVAL);
    }

    open_flags = ((attr->valid_mask & FOPS_SETATTR_SIZE) != 0U)
                         ? (O_RDWR | O_CLOEXEC)
                         : (O_RDONLY | O_CLOEXEC);

    err = fops_open_object(fuid, open_flags, &meta, &fd, FS_OP_SETATTR);
    if (fs_failed(err))
    {
        return err;
    }

    err = lsa_fstat(fd, &st);
    if (fs_failed(err))
    {
        goto out;
    }
    err = fops_check_type_flags(fops_type_from_mode(st.st_mode), flags,
                                FS_OP_SETATTR);
    if (fs_failed(err))
    {
        goto out;
    }

    if ((attr->valid_mask & FOPS_SETATTR_MODE) != 0U)
    {
        err = lsa_fchmod(fd, attr->mode & FS_PERM_MASK);
        if (fs_failed(err))
        {
            goto out;
        }
    }

    if (((attr->valid_mask & FOPS_SETATTR_UID) != 0U) ||
        ((attr->valid_mask & FOPS_SETATTR_GID) != 0U))
    {
        uid_t uid;
        gid_t gid;

        uid = ((attr->valid_mask & FOPS_SETATTR_UID) != 0U) ? attr->uid
                                                            : (uid_t)-1;
        gid = ((attr->valid_mask & FOPS_SETATTR_GID) != 0U) ? attr->gid
                                                            : (gid_t)-1;
        err = lsa_fchown(fd, uid, gid);
        if (fs_failed(err))
        {
            goto out;
        }
    }

    if ((attr->valid_mask & FOPS_SETATTR_SIZE) != 0U)
    {
        err = lsa_ftruncate(fd, (off_t)attr->size);
        if (fs_failed(err))
        {
            goto out;
        }
    }

out:
    fops_close_object(meta, fd);
    return err;
}

fs_error_t fops_access(const fuid_t *fuid, int mask, fs_flags_t flags)
{
    fs_error_t err;
    obj_meta_t *meta;
    int fd;
    struct stat st;

    meta = NULL;
    fd = -1;

    err = fops_validate_getattr_flags(flags, FS_OP_ACCESS);
    if (fs_failed(err))
    {
        return err;
    }

    err = fops_open_object(fuid, O_PATH | O_CLOEXEC, &meta, &fd, FS_OP_ACCESS);
    if (fs_failed(err))
    {
        return err;
    }

    err = lsa_fstat(fd, &st);
    if (fs_failed(err))
    {
        goto out;
    }
    err = fops_check_type_flags(fops_type_from_mode(st.st_mode), flags,
                                FS_OP_ACCESS);
    if (fs_failed(err))
    {
        goto out;
    }

    err = lsa_faccess(fd, mask);

out:
    fops_close_object(meta, fd);
    return err;
}
