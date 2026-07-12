#include "fops/internal/fops_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "fops/internal/fops_error.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

static fs_error_t fops_validate_known_flags(fs_flags_t flags,
                                            fs_flags_t known,
                                            fs_op_t sub)
{
    fs_error_t err;

    if ((flags & ~known) != 0U) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("flag check failed: flags=0x%x known=0x%x, "
                          "err=%s (0x%x)",
                          flags, known, fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

static fs_error_t fops_validate_type_flag_pair(fs_flags_t flags,
                                               fs_op_t sub)
{
    fs_error_t err;

    if (fs_flag_test(flags, FS_FLAG_DIRECTORY) &&
        fs_flag_test(flags, FS_FLAG_REGULAR)) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("flag check failed: DIRECTORY conflicts with "
                          "REGULAR, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

fs_error_t fops_validate_name(const char *name,
                              fs_op_t sub,
                              bool allow_dot_names)
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

    if (strchr(name, '/') != NULL) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: slash in name=%s, "
                          "err=%s (0x%x)",
                          name, fs_error_str(err), err);
        return err;
    }

    if (!allow_dot_names &&
        ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: dot name is not allowed, "
                          "name=%s, err=%s (0x%x)",
                          name, fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

fs_error_t fops_validate_lookup_flags(fs_flags_t flags, fs_op_t sub)
{
    return fops_validate_flags(FS_OP_LOOKUP, flags, sub);
}

fs_error_t fops_validate_create_flags(fs_flags_t flags, fs_op_t sub)
{
    return fops_validate_flags(FS_OP_CREATE, flags, sub);
}

fs_error_t fops_validate_mkdir_flags(fs_flags_t flags, fs_op_t sub)
{
    return fops_validate_flags(FS_OP_MKDIR, flags, sub);
}

fs_error_t fops_validate_getattr_flags(fs_flags_t flags, fs_op_t sub)
{
    return fops_validate_flags(FS_OP_GETATTR, flags, sub);
}

fs_error_t fops_validate_readdir_flags(fs_flags_t flags, fs_op_t sub)
{
    if (sub == FS_OP_READDIRPLUS) {
        return fops_validate_flags(FS_OP_READDIRPLUS, flags, sub);
    }

    return fops_validate_flags(FS_OP_READDIR, flags, sub);
}

fs_error_t fops_validate_unlink_flags(fs_flags_t flags, fs_op_t sub)
{
    return fops_validate_flags(FS_OP_UNLINK, flags, sub);
}

fs_error_t fops_validate_rmdir_flags(fs_flags_t flags, fs_op_t sub)
{
    return fops_validate_flags(FS_OP_RMDIR, flags, sub);
}

fs_error_t fops_validate_open_flags(fs_flags_t flags, fs_op_t sub)
{
    if (sub == FS_OP_OPENHANDLE) {
        return fops_validate_flags(FS_OP_OPENHANDLE, flags, sub);
    }

    return fops_validate_flags(FS_OP_OPEN, flags, sub);
}

fs_error_t fops_validate_setattr_flags(fs_flags_t flags, fs_op_t sub)
{
    return fops_validate_flags(FS_OP_SETATTR, flags, sub);
}

fs_error_t fops_validate_xattr_flags(fs_flags_t flags, fs_op_t sub)
{
    return fops_validate_flags(FS_OP_SETXATTR, flags, sub);
}

fs_error_t fops_validate_replace_flags(fs_flags_t flags,
                                       fs_op_t sub)
{
    return fops_validate_flags(sub, flags, sub);
}

fs_error_t fops_validate_new_name_flags(fs_flags_t flags,
                                        fs_op_t sub)
{
    return fops_validate_flags(sub, flags, sub);
}

fs_error_t fops_check_type_flags(fs_type_t type,
                                 fs_flags_t flags,
                                 fs_op_t sub)
{
    fs_error_t err;

    if (fs_flag_test(flags, FS_FLAG_DIRECTORY) && (type != FS_TYPE_DIR)) {
        err = fops_error(sub, ENOTDIR);
        FS_LOG_DUMP_ERROR("type check failed: expected directory, type=%u, "
                          "err=%s (0x%x)",
                          (unsigned int)type, fs_error_str(err), err);
        return err;
    }

    if (fs_flag_test(flags, FS_FLAG_REGULAR) && (type != FS_TYPE_REG)) {
        err = fops_error(sub, (type == FS_TYPE_DIR) ? EISDIR : EINVAL);
        FS_LOG_DUMP_ERROR("type check failed: expected regular, type=%u, "
                          "err=%s (0x%x)",
                          (unsigned int)type, fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

mode_t fops_create_mode(const fops_create_attr_t *attr,
                        mode_t default_mode)
{
    if ((attr != NULL) &&
        ((attr->valid_mask & FOPS_CREATE_ATTR_MODE) != 0U)) {
        return attr->mode & FS_PERM_MASK;
    }

    return default_mode & FS_PERM_MASK;
}

fs_error_t fops_validate_create_attr(const fops_create_attr_t *attr,
                                     uint32_t supported_mask,
                                     fs_op_t sub)
{
    fs_error_t err;

    if (attr == NULL) {
        return FS_OK;
    }

    if ((attr->valid_mask & ~supported_mask) != 0U) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("create attr check failed: valid_mask=0x%x "
                          "supported=0x%x, err=%s (0x%x)",
                          attr->valid_mask,
                          supported_mask,
                          fs_error_str(err),
                          err);
        return err;
    }

    return FS_OK;
}

fs_error_t fops_apply_create_attr(int fd,
                                  const fops_create_attr_t *attr,
                                  bool allow_size,
                                  fs_op_t sub)
{
    fs_error_t err;
    uid_t uid;
    gid_t gid;

    if (attr == NULL) {
        return FS_OK;
    }

    if (((attr->valid_mask & FOPS_CREATE_ATTR_SIZE) != 0U) &&
        !allow_size) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("create attr check failed: size is unsupported, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (((attr->valid_mask & FOPS_CREATE_ATTR_UID) != 0U) ||
        ((attr->valid_mask & FOPS_CREATE_ATTR_GID) != 0U)) {
        uid = ((attr->valid_mask & FOPS_CREATE_ATTR_UID) != 0U) ?
                attr->uid : (uid_t)-1;
        gid = ((attr->valid_mask & FOPS_CREATE_ATTR_GID) != 0U) ?
                attr->gid : (gid_t)-1;
        err = lsa_fchown(fd, uid, gid);
        if (fs_failed(err)) {
            return err;
        }
    }

    if ((attr->valid_mask & FOPS_CREATE_ATTR_SIZE) != 0U) {
        err = lsa_ftruncate(fd, (off_t)attr->size);
        if (fs_failed(err)) {
            return err;
        }
    }

    (void)sub;
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

fs_error_t fops_open_parent_dir(const fuid_t *fuid,
                                obj_meta_t **out_meta,
                                int *out_fd,
                                fs_op_t sub)
{
    fs_error_t err;

    if ((fuid == NULL) || !fuid_is_dir(fuid)) {
        err = fops_error(sub, ENOTDIR);
        FS_LOG_DUMP_ERROR("parent check failed: not dir, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    return fops_open_object(fuid,
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                            out_meta,
                            out_fd,
                            sub);
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
    obj_key_t key;
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

    err = objmgr_alloc_key(&key);
    if (fs_failed(err)) {
        return err;
    }

    child_fuid = fops_make_child_fuid(parent_fuid,
                                      key.objectid,
                                      key.gen,
                                      type);
    if (objmgr_create(&child_fuid, handle) == NULL) {
        (void)objmgr_free_key(&key);
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

int fops_linux_open_flags(fs_flags_t flags)
{
    int linux_flags;

    if (fs_flag_test(flags, FS_FLAG_WRITE) &&
        fs_flag_test(flags, FS_FLAG_READ)) {
        linux_flags = O_RDWR;
    } else if (fs_flag_test(flags, FS_FLAG_WRITE)) {
        linux_flags = O_WRONLY;
    } else {
        linux_flags = O_RDONLY;
    }

    linux_flags |= O_CLOEXEC;

    if (fs_flag_test(flags, FS_FLAG_APPEND)) {
        linux_flags |= O_APPEND;
    }
    if (fs_flag_test(flags, FS_FLAG_TRUNCATE)) {
        linux_flags |= O_TRUNC;
    }
    if (fs_flag_test(flags, FS_FLAG_SYNC)) {
        linux_flags |= O_SYNC;
    }
    if (fs_flag_test(flags, FS_FLAG_DIRECT)) {
        linux_flags |= O_DIRECT;
    }
    if (fs_flag_test(flags, FS_FLAG_DIRECTORY)) {
        linux_flags |= O_DIRECTORY;
    }

    return linux_flags;
}

fs_error_t fops_file_check(const fops_file_t *file, fs_op_t sub)
{
    fs_error_t err;

    if ((file == NULL) || (file->meta == NULL) || (file->fd < 0)) {
        err = fops_error(sub, EBADF);
        FS_LOG_DUMP_ERROR("file check failed: invalid file, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if ((sub == FS_OP_READ) &&
        fs_flag_test(file->flags, FS_FLAG_WRITE) &&
        !fs_flag_test(file->flags, FS_FLAG_READ)) {
        err = fops_error(sub, EBADF);
        FS_LOG_DUMP_ERROR("file check failed: write-only, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if ((sub == FS_OP_WRITE) &&
        !fs_flag_test(file->flags, FS_FLAG_WRITE)) {
        err = fops_error(sub, EBADF);
        FS_LOG_DUMP_ERROR("file check failed: not writable, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}
