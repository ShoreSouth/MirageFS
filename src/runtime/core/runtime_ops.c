#include "runtime/internal/runtime_internal.h"

fs_error_t runtime_mknod(const char *path,
                         fs_type_t type,
                         const fops_create_attr_t *attr,
                         const fops_device_t *device,
                         fs_flags_t flags,
                         fops_object_result_t *out)
{
    fs_error_t err;
    namei_parent_result_t parent;
    fops_args_t args;

    err = runtime_lookup_parent_path(path, flags, &parent);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_MKNOD;
    args.flags = flags;
    args.parent_fuid = &parent.parent_fuid;
    args.name = parent.name;
    args.u.mknod.type = type;
    args.u.mknod.attr = attr;
    args.u.mknod.device = device;
    args.u.mknod.out = out;

    return runtime_dispatch(&args);
}

fs_error_t runtime_link(const char *old_path,
                        const char *new_path,
                        fs_flags_t flags,
                        fops_object_result_t *out)
{
    fs_error_t err;
    namei_parent_result_t old_parent;
    namei_parent_result_t new_parent;
    fops_args_t args;

    err = runtime_lookup_parent_path(old_path, flags, &old_parent);
    if (fs_failed(err)) {
        return err;
    }

    err = runtime_lookup_parent_path(new_path, flags, &new_parent);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_LINK;
    args.flags = flags;
    args.parent_fuid = &old_parent.parent_fuid;
    args.name = old_parent.name;
    args.u.link.new_parent_fuid = &new_parent.parent_fuid;
    args.u.link.new_name = new_parent.name;
    args.u.link.out = out;

    return runtime_dispatch(&args);
}

fs_error_t runtime_setattr(const char *path,
                           const fops_setattr_t *attr,
                           fs_flags_t flags)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, flags, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_SETATTR;
    args.flags = flags;
    args.fuid = &fuid;
    args.u.setattr.attr = attr;

    return runtime_dispatch(&args);
}

fs_error_t runtime_access(const char *path, int mask, fs_flags_t flags)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, flags, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_ACCESS;
    args.flags = flags;
    args.fuid = &fuid;
    args.u.access.mask = mask;

    return runtime_dispatch(&args);
}

fs_error_t runtime_truncate(const char *path, uint64_t size, fs_flags_t flags)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, flags, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_TRUNCATE;
    args.flags = flags;
    args.fuid = &fuid;
    args.u.truncate.size = size;

    return runtime_dispatch(&args);
}

fs_error_t runtime_close(fops_file_t *file)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_CLOSE;
    args.u.close.file = file;

    return runtime_dispatch(&args);
}

fs_error_t runtime_read(fops_file_t *file,
                        void *buf,
                        size_t size,
                        size_t *actual)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_READ;
    args.u.read.file = file;
    args.u.read.buf = buf;
    args.u.read.size = size;
    args.u.read.actual = actual;

    return runtime_dispatch(&args);
}

fs_error_t runtime_write(fops_file_t *file,
                         const void *buf,
                         size_t size,
                         size_t *actual)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_WRITE;
    args.u.write.file = file;
    args.u.write.buf = buf;
    args.u.write.size = size;
    args.u.write.actual = actual;

    return runtime_dispatch(&args);
}

fs_error_t runtime_getxattr(const char *path,
                            const char *name,
                            void *value,
                            size_t size,
                            size_t *actual)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, FS_FLAG_NONE, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_GETXATTR;
    args.fuid = &fuid;
    args.name = name;
    args.u.getxattr.value = value;
    args.u.getxattr.size = size;
    args.u.getxattr.actual = actual;

    return runtime_dispatch(&args);
}

fs_error_t runtime_setxattr(const char *path,
                            const char *name,
                            const void *value,
                            size_t size,
                            fs_flags_t flags)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, FS_FLAG_NONE, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_SETXATTR;
    args.flags = flags;
    args.fuid = &fuid;
    args.name = name;
    args.u.setxattr.value = value;
    args.u.setxattr.size = size;

    return runtime_dispatch(&args);
}

fs_error_t runtime_listxattr(const char *path,
                             char *list,
                             size_t size,
                             size_t *actual)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, FS_FLAG_NONE, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_LISTXATTR;
    args.fuid = &fuid;
    args.u.listxattr.list = list;
    args.u.listxattr.size = size;
    args.u.listxattr.actual = actual;

    return runtime_dispatch(&args);
}

fs_error_t runtime_removexattr(const char *path, const char *name)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, FS_FLAG_NONE, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_REMOVEXATTR;
    args.fuid = &fuid;
    args.name = name;

    return runtime_dispatch(&args);
}

fs_error_t runtime_statfs(const char *path, fops_statfs_t *out_statfs)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, FS_FLAG_NONE, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_STATFS;
    args.fuid = &fuid;
    args.u.statfs.out_statfs = out_statfs;

    return runtime_dispatch(&args);
}

fs_error_t runtime_syncfs(const char *path)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, FS_FLAG_NONE, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_SYNCFS;
    args.fuid = &fuid;

    return runtime_dispatch(&args);
}

fs_error_t runtime_gethandle(const char *path, obj_handle_t *out_handle)
{
    fs_error_t err;
    fuid_t fuid;
    fops_args_t args;

    err = runtime_lookup_fuid(path, FS_FLAG_NONE, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_GETHANDLE;
    args.fuid = &fuid;
    args.u.gethandle.out_handle = out_handle;

    return runtime_dispatch(&args);
}

fs_error_t runtime_openhandle(const obj_handle_t *handle,
                              fs_flags_t flags,
                              fops_file_t **out_file)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_OPENHANDLE;
    args.flags = flags;
    args.u.openhandle.handle = handle;
    args.u.openhandle.out_file = out_file;

    return runtime_dispatch(&args);
}
