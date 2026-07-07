#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

static fs_error_t fops_validate_replace_flags(fs_flags_t flags, fs_op_t sub)
{
    fs_error_t err;
    const fs_flags_t known = FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE;

    if ((flags & ~known) != 0U) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("flag check failed: flags=0x%x, err=%s (0x%x)",
                          flags, fs_error_str(err), err);
        return err;
    }

    if (fs_flag_test(flags, FS_FLAG_REPLACE) &&
        fs_flag_test(flags, FS_FLAG_EXCLUSIVE)) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("flag check failed: REPLACE conflicts with "
                          "EXCLUSIVE, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}


static fs_error_t fops_validate_new_name_flags(fs_flags_t flags, fs_op_t sub)
{
    fs_error_t err;
    const fs_flags_t known = FS_FLAG_EXCLUSIVE;

    if ((flags & ~known) != 0U) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("flag check failed: flags=0x%x, err=%s (0x%x)",
                          flags, fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

static fs_error_t fops_open_parent_dir(const fuid_t *fuid,
                                       obj_meta_t **out_meta,
                                       int *out_fd,
                                       fs_op_t sub)
{
    fs_error_t err;

    if ((fuid == NULL) || !fuid_is_dir(fuid)) {
        err = fops_error(sub, ENOTDIR);
        FS_LOG_DUMP_ERROR("parent check failed: not dir, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    return fops_open_object(fuid,
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                            out_meta,
                            out_fd,
                            sub);
}

fs_error_t fops_rename(const fuid_t *old_parent_fuid,
                       const char *old_name,
                       const fuid_t *new_parent_fuid,
                       const char *new_name,
                       fs_flags_t flags)
{
    fs_error_t err;
    fops_object_result_t target;
    obj_meta_t *old_meta;
    obj_meta_t *new_meta;
    int old_fd;
    int new_fd;
    bool target_exists;

    old_meta = NULL;
    new_meta = NULL;
    old_fd = -1;
    new_fd = -1;
    target_exists = false;
    memset(&target, 0, sizeof(target));
    fuid_set_invalid(&target.fuid);

    err = fops_validate_replace_flags(flags, FS_OP_RENAME);
    if (fs_failed(err)) {
        goto out;
    }

    if ((old_parent_fuid == NULL) || (new_parent_fuid == NULL) ||
        (old_parent_fuid->fsid != new_parent_fuid->fsid)) {
        err = fops_error(FS_OP_RENAME, EXDEV);
        FS_LOG_DUMP_ERROR("rename failed: invalid/cross fs parent, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    err = fops_validate_name(old_name, FS_OP_RENAME, false);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_validate_name(new_name, FS_OP_RENAME, false);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_lookup_plus(old_parent_fuid, old_name, FS_FLAG_NONE, &target);
    if (fs_failed(err)) {
        goto out;
    }
    fuid_set_invalid(&target.fuid);

    err = fops_lookup_plus(new_parent_fuid, new_name, FS_FLAG_NONE, &target);
    if (fs_succeeded(err)) {
        target_exists = true;
        if (!fs_flag_test(flags, FS_FLAG_REPLACE)) {
            err = fops_error(FS_OP_RENAME, EEXIST);
            FS_LOG_DUMP_ERROR("rename failed: target exists, "
                              "err=%s (0x%x)", fs_error_str(err), err);
            goto out;
        }
    } else if (fs_err_errno(err) == FS_ERRNO_ENOENT) {
        err = FS_OK;
    } else {
        goto out;
    }

    err = fops_open_parent_dir(old_parent_fuid, &old_meta, &old_fd,
                               FS_OP_RENAME);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_open_parent_dir(new_parent_fuid, &new_meta, &new_fd,
                               FS_OP_RENAME);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_rename(old_fd, old_name, new_fd, new_name, flags);
    if (fs_failed(err)) {
        goto out;
    }

    if (target_exists && fuid_is_valid(&target.fuid)) {
        (void)objmgr_delete(&target.fuid);
    }

out:
    fops_close_object(new_meta, new_fd);
    fops_close_object(old_meta, old_fd);
    return err;
}

fs_error_t fops_link_plus(const fuid_t *old_parent_fuid,
                          const char *old_name,
                          const fuid_t *new_parent_fuid,
                          const char *new_name,
                          fs_flags_t flags,
                          fops_object_result_t *out)
{
    fs_error_t err;
    obj_meta_t *old_meta;
    obj_meta_t *new_meta;
    int old_fd;
    int new_fd;

    old_meta = NULL;
    new_meta = NULL;
    old_fd = -1;
    new_fd = -1;

    if (out != NULL) {
        memset(out, 0, sizeof(*out));
        fuid_set_invalid(&out->fuid);
    }

    err = fops_validate_new_name_flags(flags, FS_OP_LINK);
    if (fs_failed(err)) {
        goto out;
    }
    if ((out == NULL) || (old_parent_fuid == NULL) ||
        (new_parent_fuid == NULL) ||
        (old_parent_fuid->fsid != new_parent_fuid->fsid)) {
        err = fops_error(FS_OP_LINK, EINVAL);
        goto out;
    }

    err = fops_validate_name(old_name, FS_OP_LINK, false);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_validate_name(new_name, FS_OP_LINK, false);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_lookup_plus(new_parent_fuid, new_name, FS_FLAG_NONE, out);
    if (fs_succeeded(err)) {
        err = fops_error(FS_OP_LINK, EEXIST);
        goto out;
    }
    if (fs_err_errno(err) != FS_ERRNO_ENOENT) {
        goto out;
    }

    err = fops_open_parent_dir(old_parent_fuid, &old_meta, &old_fd,
                               FS_OP_LINK);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_open_parent_dir(new_parent_fuid, &new_meta, &new_fd,
                               FS_OP_LINK);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_link(old_fd, old_name, new_fd, new_name, flags);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_lookup_plus(new_parent_fuid, new_name, FS_FLAG_REGULAR, out);

out:
    fops_close_object(new_meta, new_fd);
    fops_close_object(old_meta, old_fd);
    return err;
}

fs_error_t fops_link(const fuid_t *old_parent_fuid,
                     const char *old_name,
                     const fuid_t *new_parent_fuid,
                     const char *new_name,
                     fs_flags_t flags,
                     fuid_t *out_fuid)
{
    fs_error_t err;
    fops_object_result_t result;

    if (out_fuid != NULL) {
        fuid_set_invalid(out_fuid);
    }
    if (out_fuid == NULL) {
        return fops_error(FS_OP_LINK, EINVAL);
    }

    err = fops_link_plus(old_parent_fuid, old_name, new_parent_fuid,
                         new_name, flags, &result);
    if (fs_failed(err)) {
        return err;
    }

    *out_fuid = result.fuid;
    return FS_OK;
}

fs_error_t fops_symlink_plus(const fuid_t *parent_fuid,
                             const char *name,
                             const char *target,
                             fs_flags_t flags,
                             fops_object_result_t *out)
{
    fs_error_t err;
    obj_meta_t *parent_meta;
    int parent_fd;

    parent_meta = NULL;
    parent_fd = -1;

    if (out != NULL) {
        memset(out, 0, sizeof(*out));
        fuid_set_invalid(&out->fuid);
    }
    if ((out == NULL) || (target == NULL)) {
        return fops_error(FS_OP_SYMLINK, EINVAL);
    }
    err = fops_validate_new_name_flags(flags, FS_OP_SYMLINK);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_validate_name(name, FS_OP_SYMLINK, false);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_lookup_plus(parent_fuid, name, FS_FLAG_NOFOLLOW, out);
    if (fs_succeeded(err)) {
        err = fops_error(FS_OP_SYMLINK, EEXIST);
        goto out;
    }
    if (fs_err_errno(err) != FS_ERRNO_ENOENT) {
        goto out;
    }

    err = fops_open_parent_dir(parent_fuid, &parent_meta, &parent_fd,
                               FS_OP_SYMLINK);
    if (fs_failed(err)) {
        goto out;
    }
    err = lsa_symlink(target, parent_fd, name, flags);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_lookup_plus(parent_fuid, name, FS_FLAG_NOFOLLOW, out);

out:
    fops_close_object(parent_meta, parent_fd);
    return err;
}

fs_error_t fops_symlink(const fuid_t *parent_fuid,
                        const char *name,
                        const char *target,
                        fs_flags_t flags,
                        fuid_t *out_fuid)
{
    fs_error_t err;
    fops_object_result_t result;

    if (out_fuid != NULL) {
        fuid_set_invalid(out_fuid);
    }
    if (out_fuid == NULL) {
        return fops_error(FS_OP_SYMLINK, EINVAL);
    }

    err = fops_symlink_plus(parent_fuid, name, target, flags, &result);
    if (fs_failed(err)) {
        return err;
    }

    *out_fuid = result.fuid;
    return FS_OK;
}

fs_error_t fops_mknod_plus(const fuid_t *parent_fuid,
                           const char *name,
                           fs_type_t type,
                           const fops_create_attr_t *attr,
                           const fops_device_t *device,
                           fs_flags_t flags,
                           fops_object_result_t *out)
{
    fs_error_t err;
    fs_error_t rollback_err;
    obj_meta_t *parent_meta;
    int parent_fd;
    lsa_device_t lsa_device;
    lsa_device_t *lsa_device_ptr;
    mode_t mode;

    parent_meta = NULL;
    parent_fd = -1;
    lsa_device_ptr = NULL;

    if (out != NULL) {
        memset(out, 0, sizeof(*out));
        fuid_set_invalid(&out->fuid);
    }
    if (out == NULL) {
        return fops_error(FS_OP_MKNOD, EINVAL);
    }

    err = fops_validate_new_name_flags(flags, FS_OP_MKNOD);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_validate_create_attr(attr, FOPS_CREATE_ATTR_MODE,
                                    FS_OP_MKNOD);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_validate_name(name, FS_OP_MKNOD, false);
    if (fs_failed(err)) {
        goto out;
    }
    err = fops_lookup_plus(parent_fuid, name, FS_FLAG_NOFOLLOW, out);
    if (fs_succeeded(err)) {
        err = fops_error(FS_OP_MKNOD, EEXIST);
        goto out;
    }
    if (fs_err_errno(err) != FS_ERRNO_ENOENT) {
        goto out;
    }

    if ((type == FS_TYPE_BLK) || (type == FS_TYPE_CHR)) {
        if (device == NULL) {
            err = fops_error(FS_OP_MKNOD, EINVAL);
            goto out;
        }
        lsa_device.major_id = device->major_id;
        lsa_device.minor_id = device->minor_id;
        lsa_device_ptr = &lsa_device;
    }

    err = fops_open_parent_dir(parent_fuid, &parent_meta, &parent_fd,
                               FS_OP_MKNOD);
    if (fs_failed(err)) {
        goto out;
    }

    mode = fops_create_mode(attr, FS_MODE_FILE_DEFAULT);
    err = lsa_mknod(parent_fd, name, type, mode, lsa_device_ptr);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_lookup_plus(parent_fuid, name, FS_FLAG_NOFOLLOW, out);
    if (fs_failed(err)) {
        rollback_err = (type == FS_TYPE_DIR) ?
                       lsa_rmdir(parent_fd, name, FS_FLAG_NONE) :
                       lsa_unlink(parent_fd, name, FS_FLAG_NONE);
        if (fs_failed(rollback_err)) {
            FS_LOG_DUMP_ERROR("rollback mknod failed: err=%s (0x%x)",
                              fs_error_str(rollback_err), rollback_err);
        }
    }

out:
    fops_close_object(parent_meta, parent_fd);
    return err;
}

fs_error_t fops_mknod(const fuid_t *parent_fuid,
                      const char *name,
                      fs_type_t type,
                      const fops_create_attr_t *attr,
                      const fops_device_t *device,
                      fs_flags_t flags,
                      fuid_t *out_fuid)
{
    fs_error_t err;
    fops_object_result_t result;

    if (out_fuid != NULL) {
        fuid_set_invalid(out_fuid);
    }
    if (out_fuid == NULL) {
        return fops_error(FS_OP_MKNOD, EINVAL);
    }

    err = fops_mknod_plus(parent_fuid, name, type, attr, device, flags,
                          &result);
    if (fs_failed(err)) {
        return err;
    }

    *out_fuid = result.fuid;
    return FS_OK;
}
