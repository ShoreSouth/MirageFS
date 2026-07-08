#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

static fs_error_t fops_mknod_validate_req(const fops_mknod_req_t *req)
{
    fs_error_t err;

    if (req == NULL) {
        err = fops_error(FS_OP_MKNOD, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: req is NULL, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    err = fops_validate_new_name_flags(req->flags, FS_OP_MKNOD);
    if (fs_failed(err)) {
        return err;
    }

    err = fops_validate_create_attr(req->attr, FOPS_CREATE_ATTR_MODE,
                                    FS_OP_MKNOD);
    if (fs_failed(err)) {
        return err;
    }

    err = fops_validate_name(req->name, FS_OP_MKNOD, false);
    if (fs_failed(err)) {
        return err;
    }

    if (((req->type == FS_TYPE_BLK) || (req->type == FS_TYPE_CHR)) &&
        (req->device == NULL)) {
        err = fops_error(FS_OP_MKNOD, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: device is NULL, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

static lsa_device_t *fops_mknod_prepare_device(const fops_mknod_req_t *req,
                                               lsa_device_t *device)
{
    if ((req->type != FS_TYPE_BLK) && (req->type != FS_TYPE_CHR)) {
        return NULL;
    }

    device->major_id = req->device->major_id;
    device->minor_id = req->device->minor_id;
    return device;
}

fs_error_t fops_mknod_plus(const fops_mknod_req_t *req,
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

    err = fops_mknod_validate_req(req);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_lookup_plus(req->parent_fuid, req->name,
                           FS_FLAG_NOFOLLOW, out);
    if (fs_succeeded(err)) {
        err = fops_error(FS_OP_MKNOD, EEXIST);
        goto out;
    }
    if (fs_err_errno(err) != FS_ERRNO_ENOENT) {
        goto out;
    }

    lsa_device_ptr = fops_mknod_prepare_device(req, &lsa_device);

    err = fops_open_parent_dir(req->parent_fuid, &parent_meta, &parent_fd,
                               FS_OP_MKNOD);
    if (fs_failed(err)) {
        goto out;
    }

    mode = fops_create_mode(req->attr, FS_MODE_FILE_DEFAULT);
    err = lsa_mknod(parent_fd, req->name, req->type, mode, lsa_device_ptr);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_lookup_plus(req->parent_fuid, req->name,
                           FS_FLAG_NOFOLLOW, out);
    if (fs_failed(err)) {
        rollback_err = (req->type == FS_TYPE_DIR) ?
                       lsa_rmdir(parent_fd, req->name, FS_FLAG_NONE) :
                       lsa_unlink(parent_fd, req->name, FS_FLAG_NONE);
        if (fs_failed(rollback_err)) {
            FS_LOG_DUMP_ERROR("rollback mknod failed: err=%s (0x%x)",
                              fs_error_str(rollback_err), rollback_err);
        }
    }

out:
    fops_close_object(parent_meta, parent_fd);
    return err;
}

fs_error_t fops_mknod(const fops_mknod_req_t *req,
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

    err = fops_mknod_plus(req, &result);
    if (fs_failed(err)) {
        return err;
    }

    *out_fuid = result.fuid;
    return FS_OK;
}
