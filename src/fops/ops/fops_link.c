#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

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
