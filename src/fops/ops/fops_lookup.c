#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_lookup_plus(const fuid_t *parent_fuid, const char *name,
                            fs_flags_t flags, fops_object_result_t *out)
{
    fs_error_t err;
    obj_meta_t *parent_meta;
    int parent_fd;
    int child_fd;
    lsa_file_handle_t lsa_handle;
    obj_handle_t handle;
    int32_t mount_id;
    struct stat st;
    fs_type_t type;

    FS_LOG_DUMP_INFO("enter: parent=%p name=%s flags=0x%x out=%p",
                     (const void *)parent_fuid, name ? name : "(null)", flags,
                     (void *)out);

    parent_meta = NULL;
    parent_fd = -1;
    child_fd = -1;

    if (out != NULL)
    {
        memset(out, 0, sizeof(*out));
        fuid_set_invalid(&out->fuid);
    }

    if ((parent_fuid == NULL) || (out == NULL))
    {
        err = fops_error(FS_OP_LOOKUP, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid lookup args, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

    if (!fuid_is_dir(parent_fuid))
    {
        err = fops_error(FS_OP_LOOKUP, ENOTDIR);
        FS_LOG_DUMP_ERROR("lookup failed: parent is not dir, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

    err = fops_validate_lookup_flags(flags, FS_OP_LOOKUP);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_validate_name(name, FS_OP_LOOKUP, true);
    if (fs_failed(err))
    {
        goto out;
    }

    if (strcmp(name, ".") == 0)
    {
        err = fops_getattr(parent_fuid, FS_FLAG_NONE, &out->attr);
        if (fs_failed(err))
        {
            goto out;
        }
        err = fops_check_type_flags(out->attr.type, flags, FS_OP_LOOKUP);
        if (fs_failed(err))
        {
            goto out;
        }
        out->fuid = *parent_fuid;
        err = FS_OK;
        goto out;
    }

    err = fops_open_object(parent_fuid, O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                           &parent_meta, &parent_fd, FS_OP_LOOKUP);
    if (fs_failed(err))
    {
        goto out;
    }

    err = lsa_lookup(parent_fd, name, flags, &child_fd);
    if (fs_failed(err))
    {
        goto out;
    }

    err = lsa_fstat(child_fd, &st);
    if (fs_failed(err))
    {
        goto out;
    }

    type = fops_type_from_mode(st.st_mode);
    err = fops_check_type_flags(type, flags, FS_OP_LOOKUP);
    if (fs_failed(err))
    {
        goto out;
    }

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    mount_id = 0;
    err = lsa_name_to_handle_at(parent_fd, name, &lsa_handle, &mount_id, 0);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_handle_from_lsa_checked(&handle, &lsa_handle, mount_id,
                                       parent_meta, FS_OP_LOOKUP);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_fuid_from_handle(parent_fuid, &handle, type, &out->fuid,
                                FS_OP_LOOKUP);
    if (fs_failed(err))
    {
        goto out;
    }

    fops_attr_from_stat(&out->attr, &st);

out:
    if (child_fd >= 0)
    {
        (void)lsa_close(child_fd);
    }
    fops_close_object(parent_meta, parent_fd);
    FS_LOG_DUMP_INFO("exit: err=%s (0x%x)", fs_error_str(err), err);
    return err;
}

fs_error_t fops_lookup(const fuid_t *parent_fuid, const char *name,
                       fs_flags_t flags, fuid_t *out_fuid)
{
    fs_error_t err;
    fops_object_result_t result;

    if (out_fuid != NULL)
    {
        fuid_set_invalid(out_fuid);
    }

    if (out_fuid == NULL)
    {
        err = fops_error(FS_OP_LOOKUP, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: out_fuid is NULL, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = fops_lookup_plus(parent_fuid, name, flags, &result);
    if (fs_failed(err))
    {
        return err;
    }

    *out_fuid = result.fuid;
    return FS_OK;
}
