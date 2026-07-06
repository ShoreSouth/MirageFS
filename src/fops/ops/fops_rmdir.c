#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

fs_error_t fops_rmdir(const fuid_t *parent_fuid, const char *name)
{
    fs_error_t err;
    fuid_t child_fuid;
    obj_meta_t *parent_meta;
    int parent_fd;

    parent_meta = NULL;
    parent_fd = -1;
    fuid_set_invalid(&child_fuid);

    if (parent_fuid == NULL) {
        err = fops_error(FS_OP_RMDIR, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: parent is NULL, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    err = fops_lookup(parent_fuid, name, &child_fuid);
    if (fs_failed(err)) {
        goto out;
    }

    if (!fuid_is_dir(&child_fuid)) {
        err = fops_error(FS_OP_RMDIR, ENOTDIR);
        FS_LOG_DUMP_ERROR("rmdir failed: target is not dir, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

    err = fops_open_object(parent_fuid,
                           O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                           &parent_meta,
                           &parent_fd,
                           FS_OP_RMDIR);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_rmdir(parent_fd, name, FS_FLAG_NONE);
    if (fs_failed(err)) {
        goto out;
    }

    err = objmgr_delete(&child_fuid);

out:
    fops_close_object(parent_meta, parent_fd);
    return err;
}
