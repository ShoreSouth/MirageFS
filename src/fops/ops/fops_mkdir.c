#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_mkdir_plus(const fuid_t *parent_fuid, const char *name,
                           const fops_create_attr_t *attr, fs_flags_t flags,
                           fops_object_result_t *out)
{
    fs_error_t err;
    fs_error_t rollback_err;
    obj_meta_t *parent_meta;
    int parent_fd;
    int child_fd;
    lsa_file_handle_t lsa_handle;
    obj_handle_t handle;
    int32_t mount_id;
    struct stat st;
    bool linux_created;
    mode_t mode;

    FS_LOG_DUMP_INFO("enter: parent=%p name=%s flags=0x%x attr=%p out=%p",
                     (const void *)parent_fuid, name ? name : "(null)", flags,
                     (const void *)attr, (void *)out);

    parent_meta = NULL;
    parent_fd = -1;
    child_fd = -1;
    linux_created = false;

    if (out != NULL)
    {
        memset(out, 0, sizeof(*out));
        fuid_set_invalid(&out->fuid);
    }

    if ((parent_fuid == NULL) || (out == NULL))
    {
        err = fops_error(FS_OP_MKDIR, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid mkdir args, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

    if (!fuid_is_dir(parent_fuid))
    {
        err = fops_error(FS_OP_MKDIR, ENOTDIR);
        FS_LOG_DUMP_ERROR("mkdir failed: parent is not dir, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

    err = fops_validate_mkdir_flags(flags, FS_OP_MKDIR);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_validate_create_attr(
            attr,
            FOPS_CREATE_ATTR_MODE | FOPS_CREATE_ATTR_UID | FOPS_CREATE_ATTR_GID,
            FS_OP_MKDIR);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_validate_name(name, FS_OP_MKDIR, false);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_open_object(parent_fuid, O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                           &parent_meta, &parent_fd, FS_OP_MKDIR);
    if (fs_failed(err))
    {
        goto out;
    }

    mode = fops_create_mode(attr, FS_MODE_DIR_DEFAULT);
    err = lsa_mkdir(parent_fd, name, flags, mode);
    if (fs_failed(err))
    {
        goto out;
    }
    linux_created = true;

    err = lsa_lookup(parent_fd, name, FS_FLAG_DIRECTORY, &child_fd);
    if (fs_failed(err))
    {
        goto rollback;
    }

    err = fops_apply_create_attr(child_fd, attr, false, FS_OP_MKDIR);
    if (fs_failed(err))
    {
        goto rollback;
    }

    err = lsa_fstat(child_fd, &st);
    if (fs_failed(err))
    {
        goto rollback;
    }

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    mount_id = 0;
    err = lsa_name_to_handle_at(parent_fd, name, &lsa_handle, &mount_id, 0);
    if (fs_failed(err))
    {
        goto rollback;
    }

    err = fops_handle_from_lsa_checked(&handle, &lsa_handle, mount_id,
                                       parent_meta, FS_OP_MKDIR);
    if (fs_failed(err))
    {
        goto rollback;
    }

    err = fops_fuid_from_handle(parent_fuid, &handle, FS_TYPE_DIR, &out->fuid,
                                FS_OP_MKDIR);
    if (fs_failed(err))
    {
        goto rollback;
    }

    fops_attr_from_stat(&out->attr, &st);
    goto out;

rollback:
    if (linux_created)
    {
        rollback_err = lsa_rmdir(parent_fd, name, FS_FLAG_NONE);
        if (fs_failed(rollback_err))
        {
            FS_LOG_DUMP_ERROR("rollback rmdir failed: name=%s, "
                              "err=%s (0x%x)",
                              name, fs_error_str(rollback_err), rollback_err);
        }
    }

out:
    if (child_fd >= 0)
    {
        (void)lsa_close(child_fd);
    }
    fops_close_object(parent_meta, parent_fd);
    FS_LOG_DUMP_INFO("exit: err=%s (0x%x)", fs_error_str(err), err);
    return err;
}

fs_error_t fops_mkdir(const fuid_t *parent_fuid, const char *name,
                      const fops_create_attr_t *attr, fs_flags_t flags,
                      fuid_t *out_fuid)
{
    fs_error_t err;
    fops_object_result_t result;

    if (out_fuid != NULL)
    {
        fuid_set_invalid(out_fuid);
    }

    if (out_fuid == NULL)
    {
        err = fops_error(FS_OP_MKDIR, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: out_fuid is NULL, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = fops_mkdir_plus(parent_fuid, name, attr, flags, &result);
    if (fs_failed(err))
    {
        return err;
    }

    *out_fuid = result.fuid;
    return FS_OK;
}
