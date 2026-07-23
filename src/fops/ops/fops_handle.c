#include "fops/include/fops.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

static fs_error_t fops_open_from_meta(obj_meta_t *meta, fs_flags_t flags,
                                      fops_file_t **out_file, fs_op_t sub)
{
    fs_error_t err;
    lsa_file_handle_t lsa_handle;
    fops_file_t *file;
    struct stat st;
    int fd;

    if ((meta == NULL) || (out_file == NULL))
    {
        return fops_error(sub, EINVAL);
    }

    *out_file = NULL;
    fd = -1;

    err = objmeta_handle_to_lsa(&lsa_handle, &meta->handle);
    if (fs_failed(err))
    {
        return err;
    }

    err = lsa_open_by_handle_id(meta->handle.mount_id, &lsa_handle,
                                fops_linux_open_flags(flags), &fd);
    if (fs_failed(err))
    {
        return err;
    }

    err = lsa_fstat(fd, &st);
    if (fs_failed(err))
    {
        (void)lsa_close(fd);
        return err;
    }

    err = fops_check_type_flags(fops_type_from_mode(st.st_mode), flags, sub);
    if (fs_failed(err))
    {
        (void)lsa_close(fd);
        return err;
    }

    file = calloc(1, sizeof(*file));
    if (file == NULL)
    {
        (void)lsa_close(fd);
        return fops_error(sub, ENOMEM);
    }

    file->meta = meta;
    file->fd = fd;
    fuid_set_invalid(&file->fuid);
    file->flags = flags;
    *out_file = file;
    return FS_OK;
}

fs_error_t fops_gethandle(const fuid_t *fuid, obj_handle_t *out_handle)
{
    fs_error_t err;
    obj_meta_t *meta;

    if ((fuid == NULL) || (out_handle == NULL))
    {
        return fops_error(FS_OP_GETHANDLE, EINVAL);
    }

    meta = objmgr_acquire(fuid);
    if (meta == NULL)
    {
        return fops_error(FS_OP_GETHANDLE, ENOENT);
    }

    *out_handle = meta->handle;
    objmgr_release(meta);
    err = FS_OK;
    return err;
}

fs_error_t fops_open(const fuid_t *fuid, fs_flags_t flags,
                     fops_file_t **out_file)
{
    fs_error_t err;
    obj_meta_t *meta;

    if (out_file != NULL)
    {
        *out_file = NULL;
    }
    if ((fuid == NULL) || (out_file == NULL))
    {
        return fops_error(FS_OP_OPEN, EINVAL);
    }

    err = fops_validate_open_flags(flags, FS_OP_OPEN);
    if (fs_failed(err))
    {
        return err;
    }

    meta = objmgr_acquire(fuid);
    if (meta == NULL)
    {
        return fops_error(FS_OP_OPEN, ENOENT);
    }

    err = fops_open_from_meta(meta, flags, out_file, FS_OP_OPEN);
    if (fs_failed(err))
    {
        objmgr_release(meta);
        return err;
    }

    (*out_file)->fuid = *fuid;
    return FS_OK;
}

fs_error_t fops_openhandle(const obj_handle_t *handle, fs_flags_t flags,
                           fops_file_t **out_file)
{
    fs_error_t err;
    obj_meta_t *meta;

    if (out_file != NULL)
    {
        *out_file = NULL;
    }
    if ((handle == NULL) || (out_file == NULL))
    {
        return fops_error(FS_OP_OPENHANDLE, EINVAL);
    }

    err = fops_validate_open_flags(flags, FS_OP_OPENHANDLE);
    if (fs_failed(err))
    {
        return err;
    }

    meta = objmgr_acquire_by_handle(handle);
    if (meta == NULL)
    {
        return fops_error(FS_OP_OPENHANDLE, ENOENT);
    }

    err = fops_open_from_meta(meta, flags, out_file, FS_OP_OPENHANDLE);
    if (fs_failed(err))
    {
        objmgr_release(meta);
    }

    return err;
}

fs_error_t fops_close(fops_file_t *file)
{
    fs_error_t err;

    err = fops_file_check(file, FS_OP_CLOSE);
    if (fs_failed(err))
    {
        return err;
    }

    err = lsa_close(file->fd);
    file->fd = -1;
    objmgr_release(file->meta);
    file->meta = NULL;
    free(file);

    return err;
}
