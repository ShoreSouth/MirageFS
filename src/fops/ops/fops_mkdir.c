#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_mkdir(const fuid_t *parent_fuid,
                      const char *name,
                      mode_t mode,
                      fuid_t *out_fuid)
{
    fs_error_t err;
    fs_error_t rollback_err;
    obj_meta_t *parent_meta;
    int parent_fd;
    lsa_file_handle_t lsa_handle;
    obj_handle_t handle;
    int32_t mount_id;
    bool linux_created;

    parent_meta = NULL;
    parent_fd = -1;
    linux_created = false;

    if (out_fuid != NULL) {
        fuid_set_invalid(out_fuid);
    }

    if ((parent_fuid == NULL) || (out_fuid == NULL)) {
        err = fops_error(FS_OP_MKDIR, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid mkdir args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    if (!fuid_is_dir(parent_fuid)) {
        err = fops_error(FS_OP_MKDIR, ENOTDIR);
        FS_LOG_DUMP_ERROR("mkdir failed: parent is not dir, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

    err = fops_validate_name(name, FS_OP_MKDIR);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_open_object(parent_fuid,
                           O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                           &parent_meta,
                           &parent_fd,
                           FS_OP_MKDIR);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_mkdir(parent_fd, name, FS_FLAG_NONE, 
                mode & FS_PERM_MASK);
    if (fs_failed(err)) {
        goto out;
    }
    linux_created = true;

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    mount_id = 0;
    err = lsa_name_to_handle_at(parent_fd, name, 
        &lsa_handle, &mount_id, 0);
    if (fs_failed(err)) {
        goto rollback;
    }

    err = fops_handle_from_lsa_checked(&handle, &lsa_handle, 
        mount_id, parent_meta, FS_OP_MKDIR);
    if (fs_failed(err)) {
        goto rollback;
    }

    err = fops_fuid_from_handle(parent_fuid, &handle, 
        FS_TYPE_DIR, out_fuid, FS_OP_MKDIR);
    if (fs_failed(err)) {
        goto rollback;
    }

    goto out;

rollback:
    if (linux_created) {
        rollback_err = lsa_rmdir(parent_fd, name, FS_FLAG_NONE);
        if (fs_failed(rollback_err)) {
            FS_LOG_DUMP_ERROR("rollback rmdir failed: name=%s, err=%s (0x%x)",
                              name, fs_error_str(rollback_err), rollback_err);
        }
    }

out:
    fops_close_object(parent_meta, parent_fd);
    return err;
}
