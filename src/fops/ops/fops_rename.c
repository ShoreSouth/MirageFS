#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

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
