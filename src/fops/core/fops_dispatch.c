#include "fops/include/fops.h"

#include <errno.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"

typedef fs_error_t (*fops_dispatch_fn)(fops_args_t *args);

static fs_error_t fops_dispatch_lookup(fops_args_t *args)
{
    return fops_lookup_plus(args->parent_fuid, args->name, args->flags,
                            args->u.lookup.out);
}

static fs_error_t fops_dispatch_create(fops_args_t *args)
{
    return fops_create_plus(args->parent_fuid, args->name, args->u.create.attr,
                            args->flags, args->u.create.out);
}

static fs_error_t fops_dispatch_mkdir(fops_args_t *args)
{
    return fops_mkdir_plus(args->parent_fuid, args->name, args->u.mkdir.attr,
                           args->flags, args->u.mkdir.out);
}

static fs_error_t fops_dispatch_mknod(fops_args_t *args)
{
    fops_mknod_req_t req;

    req.parent_fuid = args->parent_fuid;
    req.name = args->name;
    req.type = args->u.mknod.type;
    req.attr = args->u.mknod.attr;
    req.device = args->u.mknod.device;
    req.flags = args->flags;

    return fops_mknod_plus(&req, args->u.mknod.out);
}

static fs_error_t fops_dispatch_unlink(fops_args_t *args)
{
    return fops_unlink(args->parent_fuid, args->name, args->flags);
}

static fs_error_t fops_dispatch_rmdir(fops_args_t *args)
{
    return fops_rmdir(args->parent_fuid, args->name, args->flags);
}

static fs_error_t fops_dispatch_rename(fops_args_t *args)
{
    return fops_rename(args->parent_fuid, args->name,
                       args->u.rename.new_parent_fuid, args->u.rename.new_name,
                       args->flags);
}

static fs_error_t fops_dispatch_link(fops_args_t *args)
{
    return fops_link_plus(args->parent_fuid, args->name,
                          args->u.link.new_parent_fuid, args->u.link.new_name,
                          args->flags, args->u.link.out);
}

static fs_error_t fops_dispatch_symlink(fops_args_t *args)
{
    return fops_symlink_plus(args->parent_fuid, args->name,
                             args->u.symlink.target, args->flags,
                             args->u.symlink.out);
}

static fs_error_t fops_dispatch_readlink(fops_args_t *args)
{
    return fops_readlink(args->parent_fuid, args->name, args->flags,
                         args->u.readlink.buf, args->u.readlink.size,
                         args->u.readlink.actual);
}

static fs_error_t fops_dispatch_open(fops_args_t *args)
{
    return fops_open(args->fuid, args->flags, args->u.open.out_file);
}

static fs_error_t fops_dispatch_close(fops_args_t *args)
{
    return fops_close(args->u.close.file);
}

static fs_error_t fops_dispatch_gethandle(fops_args_t *args)
{
    return fops_gethandle(args->fuid, args->u.gethandle.out_handle);
}

static fs_error_t fops_dispatch_openhandle(fops_args_t *args)
{
    return fops_openhandle(args->u.openhandle.handle, args->flags,
                           args->u.openhandle.out_file);
}

static fs_error_t fops_dispatch_readdir(fops_args_t *args)
{
    return fops_readdir(args->fuid, args->flags, args->u.readdir.entries,
                        args->u.readdir.entry_cap, args->u.readdir.out_entry_nr,
                        args->u.readdir.out_eof);
}

static fs_error_t fops_dispatch_readdirplus(fops_args_t *args)
{
    return fops_readdirplus(
            args->fuid, args->flags, args->u.readdirplus.entries,
            args->u.readdirplus.entry_cap, args->u.readdirplus.out_entry_nr,
            args->u.readdirplus.out_eof);
}

static fs_error_t fops_dispatch_getattr(fops_args_t *args)
{
    return fops_getattr(args->fuid, args->flags, args->u.getattr.out_attr);
}

static fs_error_t fops_dispatch_setattr(fops_args_t *args)
{
    return fops_setattr(args->fuid, args->u.setattr.attr, args->flags);
}

static fs_error_t fops_dispatch_access(fops_args_t *args)
{
    return fops_access(args->fuid, args->u.access.mask, args->flags);
}

static fs_error_t fops_dispatch_read(fops_args_t *args)
{
    return fops_read(args->u.read.file, args->u.read.buf, args->u.read.size,
                     args->u.read.actual);
}

static fs_error_t fops_dispatch_write(fops_args_t *args)
{
    return fops_write(args->u.write.file, args->u.write.buf, args->u.write.size,
                      args->u.write.actual);
}

static fs_error_t fops_dispatch_truncate(fops_args_t *args)
{
    return fops_truncate(args->fuid, args->u.truncate.size, args->flags);
}

static fs_error_t fops_dispatch_getxattr(fops_args_t *args)
{
    return fops_getxattr(args->fuid, args->name, args->u.getxattr.value,
                         args->u.getxattr.size, args->u.getxattr.actual);
}

static fs_error_t fops_dispatch_setxattr(fops_args_t *args)
{
    return fops_setxattr(args->fuid, args->name, args->u.setxattr.value,
                         args->u.setxattr.size, args->flags);
}

static fs_error_t fops_dispatch_listxattr(fops_args_t *args)
{
    return fops_listxattr(args->fuid, args->u.listxattr.list,
                          args->u.listxattr.size, args->u.listxattr.actual);
}

static fs_error_t fops_dispatch_removexattr(fops_args_t *args)
{
    return fops_removexattr(args->fuid, args->name);
}

static fs_error_t fops_dispatch_statfs(fops_args_t *args)
{
    return fops_statfs(args->fuid, args->u.statfs.out_statfs);
}

static fs_error_t fops_dispatch_syncfs(fops_args_t *args)
{
    return fops_syncfs(args->fuid);
}

static fops_dispatch_fn g_fops_ops[FS_OP_MAX] = {
        [FS_OP_LOOKUP] = fops_dispatch_lookup,
        [FS_OP_CREATE] = fops_dispatch_create,
        [FS_OP_MKDIR] = fops_dispatch_mkdir,
        [FS_OP_MKNOD] = fops_dispatch_mknod,
        [FS_OP_UNLINK] = fops_dispatch_unlink,
        [FS_OP_RMDIR] = fops_dispatch_rmdir,
        [FS_OP_RENAME] = fops_dispatch_rename,
        [FS_OP_LINK] = fops_dispatch_link,
        [FS_OP_SYMLINK] = fops_dispatch_symlink,
        [FS_OP_READLINK] = fops_dispatch_readlink,
        [FS_OP_OPEN] = fops_dispatch_open,
        [FS_OP_CLOSE] = fops_dispatch_close,
        [FS_OP_GETHANDLE] = fops_dispatch_gethandle,
        [FS_OP_OPENHANDLE] = fops_dispatch_openhandle,
        [FS_OP_READDIR] = fops_dispatch_readdir,
        [FS_OP_READDIRPLUS] = fops_dispatch_readdirplus,
        [FS_OP_GETATTR] = fops_dispatch_getattr,
        [FS_OP_SETATTR] = fops_dispatch_setattr,
        [FS_OP_ACCESS] = fops_dispatch_access,
        [FS_OP_READ] = fops_dispatch_read,
        [FS_OP_WRITE] = fops_dispatch_write,
        [FS_OP_TRUNCATE] = fops_dispatch_truncate,
        [FS_OP_GETXATTR] = fops_dispatch_getxattr,
        [FS_OP_SETXATTR] = fops_dispatch_setxattr,
        [FS_OP_LISTXATTR] = fops_dispatch_listxattr,
        [FS_OP_REMOVEXATTR] = fops_dispatch_removexattr,
        [FS_OP_STATFS] = fops_dispatch_statfs,
        [FS_OP_SYNCFS] = fops_dispatch_syncfs,
};

static uint64_t fops_dispatch_actual_bytes(const fops_args_t *args)
{
    if ((args->op == FS_OP_READ) && (args->u.read.actual != NULL))
    {
        return *args->u.read.actual;
    }
    if ((args->op == FS_OP_WRITE) && (args->u.write.actual != NULL))
    {
        return *args->u.write.actual;
    }

    return 0U;
}

fs_error_t fops_dispatch(fops_args_t *args)
{
    fs_error_t err;
    fops_dispatch_fn fn;
    fs_metrics_token_t token;

    err = fops_validate_args(args);
    if (fs_failed(err))
    {
        return err;
    }

    fn = g_fops_ops[args->op];
    if (fn == NULL)
    {
        err = fops_error(args->op, EINVAL);
        FS_LOG_DUMP_ERROR("FOPS dispatch failed: unsupported op=%u, "
                          "err=%s (0x%x)",
                          (unsigned int)args->op, fs_error_str(err), err);
        return err;
    }

    token = fs_metrics_begin(g_fops_metric_ids[args->op]);
    err = fn(args);
    fs_metrics_end(token, fs_succeeded(err),
                   fs_succeeded(err) ? fops_dispatch_actual_bytes(args) : 0U);
    return err;
}
