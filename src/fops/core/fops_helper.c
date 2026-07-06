#include "fops/internal/fops_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "fops/internal/fops_error.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

fs_error_t fops_validate_name(const char *name, fs_op_t sub)
{
    fs_error_t err;
    size_t len;

    if (name == NULL) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: name is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    len = strlen(name);
    if ((len == 0U) || (len > FS_MAX_NAME_LEN)) {
        err = fops_error(sub, ENAMETOOLONG);
        FS_LOG_DUMP_ERROR("param check failed: invalid name length, "
                          "len=%lu, err=%s (0x%x)",
                          (unsigned long)len, fs_error_str(err), err);
        return err;
    }

    if ((strcmp(name, ".") == 0) ||
        (strcmp(name, "..") == 0) ||
        (strchr(name, '/') != NULL)) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid name=%s, "
                          "err=%s (0x%x)",
                          name, fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

fs_error_t fops_open_object(const fuid_t *fuid,
                            int flags,
                            obj_meta_t **out_meta,
                            int *out_fd,
                            fs_op_t sub)
{
    fs_error_t err;
    obj_meta_t *meta;
    lsa_file_handle_t lsa_handle;
    int fd;

    if ((fuid == NULL) || (out_meta == NULL) || (out_fd == NULL)) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid open args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    *out_meta = NULL;
    *out_fd = -1;

    if (!fuid_is_valid(fuid)) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid fuid, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    meta = objmgr_acquire(fuid);
    if (meta == NULL) {
        err = fops_error(sub, ENOENT);
        FS_LOG_DUMP_ERROR("objmgr_acquire failed: object not found, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    err = objmeta_handle_to_lsa(&lsa_handle, &meta->handle);
    if (fs_failed(err)) {
        objmgr_release(meta);
        return err;
    }

    fd = -1;
    err = lsa_open_by_handle_id(meta->handle.mount_id,
                                &lsa_handle,
                                flags,
                                &fd);
    if (fs_failed(err)) {
        objmgr_release(meta);
        return err;
    }

    *out_meta = meta;
    *out_fd = fd;
    return FS_OK;
}

void fops_close_object(obj_meta_t *meta, int fd)
{
    if (fd >= 0) {
        (void)lsa_close(fd);
    }

    if (meta != NULL) {
        objmgr_release(meta);
    }
}

fs_type_t fops_type_from_mode(mode_t mode)
{
    return fs_type_from_mode(mode);
}

fuid_type_t fops_fuid_type_from_fs_type(fs_type_t type)
{
    switch (type) {
    case FS_TYPE_REG: return FUID_TYPE_FILE;
    case FS_TYPE_DIR: return FUID_TYPE_DIR;
    case FS_TYPE_LNK: return FUID_TYPE_SYMLINK;
    case FS_TYPE_FIFO: return FUID_TYPE_FIFO;
    case FS_TYPE_SOCK: return FUID_TYPE_SOCK;
    case FS_TYPE_BLK: return FUID_TYPE_BLK;
    case FS_TYPE_CHR: return FUID_TYPE_CHR;
    default: return FUID_TYPE_INVALID;
    }
}

void fops_attr_from_stat(fops_attr_t *attr, const struct stat *st)
{
    if ((attr == NULL) || (st == NULL)) {
        return;
    }

    memset(attr, 0, sizeof(*attr));
    attr->type = fops_type_from_mode(st->st_mode);
    attr->mode = st->st_mode;
    attr->uid = st->st_uid;
    attr->gid = st->st_gid;
    attr->size = (uint64_t)st->st_size;
    attr->nlink = (uint64_t)st->st_nlink;
    attr->atime_sec = (uint64_t)st->st_atim.tv_sec;
    attr->mtime_sec = (uint64_t)st->st_mtim.tv_sec;
    attr->ctime_sec = (uint64_t)st->st_ctim.tv_sec;
}

fuid_t fops_make_child_fuid(const fuid_t *parent_fuid,
                            ObjectId_t objectid,
                            GenId_t gen,
                            fs_type_t type)
{
    fuid_t fuid;

    fuid = fuid_make(parent_fuid->fsid,
                     objectid,
                     gen,
                     fops_fuid_type_from_fs_type(type));
    fuid.qtreeid = parent_fuid->qtreeid;
    fuid.snapid = parent_fuid->snapid;
    fuid.shardid = parent_fuid->shardid;

    return fuid;
}

fs_error_t fops_fuid_from_handle(const fuid_t *parent_fuid,
                                 const obj_handle_t *handle,
                                 fs_type_t type,
                                 fuid_t *out_fuid,
                                 fs_op_t sub)
{
    fs_error_t err;
    obj_meta_t *meta;
    ObjectId_t objectid;
    fuid_t child_fuid;

    if ((parent_fuid == NULL) || (handle == NULL) || (out_fuid == NULL)) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid handle args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    fuid_set_invalid(out_fuid);
    meta = objmgr_acquire_by_handle(handle);
    if (meta != NULL) {
        *out_fuid = fops_make_child_fuid(parent_fuid,
                                         meta->key.objectid,
                                         meta->key.gen,
                                         type);
        objmgr_release(meta);
        return FS_OK;
    }

    err = objmgr_alloc_objectid(&objectid);
    if (fs_failed(err)) {
        return err;
    }

    child_fuid = fops_make_child_fuid(parent_fuid,
                                      objectid,
                                      FOPS_OBJECT_GEN_DEFAULT,
                                      type);
    if (objmgr_create(&child_fuid, handle) == NULL) {
        err = fops_error(sub, EIO);
        FS_LOG_DUMP_ERROR("objmgr_create failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    *out_fuid = child_fuid;
    return FS_OK;
}

fs_error_t fops_handle_from_lsa_checked(obj_handle_t *out,
                                        const lsa_file_handle_t *lsa_handle,
                                        int32_t mount_id,
                                        const obj_meta_t *parent_meta,
                                        fs_op_t sub)
{
    fs_error_t err;

    if ((out == NULL) || (lsa_handle == NULL) || (parent_meta == NULL)) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid handle conversion args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (mount_id != parent_meta->handle.mount_id) {
        err = fops_error(sub, EXDEV);
        FS_LOG_DUMP_ERROR("cross mount object is unsupported: parent=%d, "
                          "child=%d, err=%s (0x%x)",
                          parent_meta->handle.mount_id,
                          mount_id,
                          fs_error_str(err), err);
        return err;
    }

    return objmeta_handle_from_lsa(out, lsa_handle, mount_id);
}
