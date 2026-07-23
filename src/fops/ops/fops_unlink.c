#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

fs_error_t fops_unlink(const fuid_t *parent_fuid, const char *name,
                       fs_flags_t flags)
{
    fs_error_t err;
    fops_object_result_t child;
    obj_meta_t *parent_meta;
    int parent_fd;

    FS_LOG_DUMP_INFO("enter: parent=%p name=%s flags=0x%x",
                     (const void *)parent_fuid, name ? name : "(null)", flags);

    parent_meta = NULL;
    parent_fd = -1;

    err = fops_validate_unlink_flags(flags, FS_OP_UNLINK);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_validate_name(name, FS_OP_UNLINK, false);
    if (fs_failed(err))
    {
        goto out;
    }

    err = fops_lookup_plus(parent_fuid, name, flags, &child);
    if (fs_failed(err))
    {
        goto out;
    }
    if (child.attr.type == FS_TYPE_DIR)
    {
        err = fops_error(FS_OP_UNLINK, EISDIR);
        FS_LOG_DUMP_ERROR("unlink type check failed: name=%s, "
                          "err=%s (0x%x)",
                          name, fs_error_str(err), err);
        goto out;
    }

    err = fops_open_object(parent_fuid, O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                           &parent_meta, &parent_fd, FS_OP_UNLINK);
    if (fs_failed(err))
    {
        goto out;
    }

    err = lsa_unlink(parent_fd, name, flags);
    if (fs_failed(err))
    {
        goto out;
    }

    err = objmgr_delete(&child.fuid);

out:
    fops_close_object(parent_meta, parent_fd);
    FS_LOG_DUMP_INFO("exit: err=%s (0x%x)", fs_error_str(err), err);
    return err;
}
