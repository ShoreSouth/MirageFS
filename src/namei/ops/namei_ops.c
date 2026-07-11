#include "namei/include/namei.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "fops/include/fops.h"
#include "namei/internal/namei_error.h"
#include "namei/internal/namei_internal.h"

static fs_flags_t namei_type_flags(fs_flags_t flags)
{
    return flags & (FS_FLAG_DIRECTORY | FS_FLAG_REGULAR);
}


static fs_error_t namei_dispatch_getattr(const fuid_t *fuid,
                                         fs_flags_t flags,
                                         fops_attr_t *out_attr)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_GETATTR;
    args.flags = flags;
    args.fuid = fuid;
    args.u.getattr.out_attr = out_attr;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_create_plus(const fuid_t *parent_fuid,
                                             const char *name,
                                             const fops_create_attr_t *attr,
                                             fs_flags_t flags,
                                             fops_object_result_t *out)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_CREATE;
    args.flags = flags;
    args.parent_fuid = parent_fuid;
    args.name = name;
    args.u.create.attr = attr;
    args.u.create.out = out;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_mkdir_plus(const fuid_t *parent_fuid,
                                            const char *name,
                                            const fops_create_attr_t *attr,
                                            fs_flags_t flags,
                                            fops_object_result_t *out)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_MKDIR;
    args.flags = flags;
    args.parent_fuid = parent_fuid;
    args.name = name;
    args.u.mkdir.attr = attr;
    args.u.mkdir.out = out;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_symlink_plus(const fuid_t *parent_fuid,
                                              const char *name,
                                              const char *target,
                                              fs_flags_t flags,
                                              fops_object_result_t *out)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_SYMLINK;
    args.flags = flags;
    args.parent_fuid = parent_fuid;
    args.name = name;
    args.u.symlink.target = target;
    args.u.symlink.out = out;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_readlink(const fuid_t *parent_fuid,
                                          const char *name,
                                          fs_flags_t flags,
                                          char *buf,
                                          size_t size,
                                          size_t *actual)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_READLINK;
    args.flags = flags;
    args.parent_fuid = parent_fuid;
    args.name = name;
    args.u.readlink.buf = buf;
    args.u.readlink.size = size;
    args.u.readlink.actual = actual;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_readdir(const fuid_t *fuid,
                                         fs_flags_t flags,
                                         fops_dirent_t *entries,
                                         uint32_t entry_cap,
                                         uint32_t *out_entry_nr,
                                         bool *out_eof)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_READDIR;
    args.flags = flags;
    args.fuid = fuid;
    args.u.readdir.entries = entries;
    args.u.readdir.entry_cap = entry_cap;
    args.u.readdir.out_entry_nr = out_entry_nr;
    args.u.readdir.out_eof = out_eof;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_readdirplus(const fuid_t *fuid,
                                             fs_flags_t flags,
                                             fops_dirent_plus_t *entries,
                                             uint32_t entry_cap,
                                             uint32_t *out_entry_nr,
                                             bool *out_eof)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_READDIRPLUS;
    args.flags = flags;
    args.fuid = fuid;
    args.u.readdirplus.entries = entries;
    args.u.readdirplus.entry_cap = entry_cap;
    args.u.readdirplus.out_entry_nr = out_entry_nr;
    args.u.readdirplus.out_eof = out_eof;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_unlink(const fuid_t *parent_fuid,
                                        const char *name,
                                        fs_flags_t flags)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_UNLINK;
    args.flags = flags;
    args.parent_fuid = parent_fuid;
    args.name = name;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_rmdir(const fuid_t *parent_fuid,
                                       const char *name,
                                       fs_flags_t flags)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_RMDIR;
    args.flags = flags;
    args.parent_fuid = parent_fuid;
    args.name = name;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_rename(const fuid_t *old_parent_fuid,
                                        const char *old_name,
                                        const fuid_t *new_parent_fuid,
                                        const char *new_name,
                                        fs_flags_t flags)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_RENAME;
    args.flags = flags;
    args.parent_fuid = old_parent_fuid;
    args.name = old_name;
    args.u.rename.new_parent_fuid = new_parent_fuid;
    args.u.rename.new_name = new_name;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_open(const fuid_t *fuid,
                                      fs_flags_t flags,
                                      fops_file_t **out_file)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_OPEN;
    args.flags = flags;
    args.fuid = fuid;
    args.u.open.out_file = out_file;

    return fops_dispatch(&args);
}

fs_error_t namei_lookup(const namei_ctx_t *ctx,
                        const char *path,
                        fs_flags_t flags,
                        fuid_t *out_fuid)
{
    fs_error_t err;
    namei_walk_result_t result;

    if (out_fuid != NULL) {
        fuid_set_invalid(out_fuid);
    }
    if (out_fuid == NULL) {
        return namei_error(NAMEI_SUB_LOOKUP, EINVAL);
    }

    err = namei_walk(ctx, path, flags, &result);
    if (fs_failed(err)) {
        return err;
    }

    if (namei_type_flags(flags) != FS_FLAG_NONE) {
        fops_attr_t attr;

        err = namei_dispatch_getattr(&result.fuid, namei_type_flags(flags), &attr);
        if (fs_failed(err)) {
            return err;
        }
    }

    *out_fuid = result.fuid;
    return FS_OK;
}

fs_error_t namei_lookup_plus(const namei_ctx_t *ctx,
                             const char *path,
                             fs_flags_t flags,
                             fops_object_result_t *out)
{
    fs_error_t err;
    namei_walk_result_t result;

    if (out == NULL) {
        return namei_error(NAMEI_SUB_LOOKUP, EINVAL);
    }
    memset(out, 0, sizeof(*out));
    fuid_set_invalid(&out->fuid);

    err = namei_walk(ctx, path, flags, &result);
    if (fs_failed(err)) {
        return err;
    }

    out->fuid = result.fuid;
    if (result.has_attr && (namei_type_flags(flags) == FS_FLAG_NONE)) {
        out->attr = result.attr;
        return FS_OK;
    }

    return namei_dispatch_getattr(&out->fuid, namei_type_flags(flags), &out->attr);
}

fs_error_t namei_lookup_parent(const namei_ctx_t *ctx,
                               const char *path,
                               fs_flags_t flags,
                               namei_parent_result_t *out)
{
    fs_error_t err;
    char path_buf[FS_MAX_PATH_LEN + 1U];
    char *slash;
    char *name;
    size_t len;
    namei_walk_result_t parent;

    (void)flags;

    if (out == NULL) {
        return namei_error(NAMEI_SUB_PARENT, EINVAL);
    }
    memset(out, 0, sizeof(*out));
    fuid_set_invalid(&out->parent_fuid);

    if ((path == NULL) || (path[0] == '\0') ||
        (strlen(path) > FS_MAX_PATH_LEN) ||
        (strcmp(path, "/") == 0) ||
        namei_path_has_trailing_slash(path)) {
        return namei_error(NAMEI_SUB_PATH, EINVAL);
    }

    if (snprintf(path_buf, sizeof(path_buf), "%s", path) >=
        (int)sizeof(path_buf)) {
        return namei_error(NAMEI_SUB_PATH, ENAMETOOLONG);
    }

    slash = strrchr(path_buf, '/');
    if (slash == NULL) {
        name = path_buf;
        err = namei_walk(ctx, ".", FS_FLAG_DIRECTORY, &parent);
    } else {
        name = slash + 1;
        if (name[0] == '\0') {
            return namei_error(NAMEI_SUB_PATH, EINVAL);
        }
        if (slash == path_buf) {
            slash[1] = '\0';
        } else {
            *slash = '\0';
        }
        err = namei_walk(ctx, path_buf, FS_FLAG_DIRECTORY, &parent);
    }
    if (fs_failed(err)) {
        return err;
    }

    len = strlen(name);
    if ((len == 0U) || (len > FS_MAX_NAME_LEN) ||
        (strchr(name, '/') != NULL) ||
        (strcmp(name, ".") == 0) ||
        (strcmp(name, "..") == 0)) {
        return namei_error(NAMEI_SUB_PATH, EINVAL);
    }

    out->parent_fuid = parent.fuid;
    memcpy(out->name, name, len + 1U);
    return FS_OK;
}

fs_error_t namei_create(const namei_ctx_t *ctx,
                        const char *path,
                        const fops_create_attr_t *attr,
                        fs_flags_t flags,
                        fops_object_result_t *out)
{
    fs_error_t err;
    namei_parent_result_t parent;

    err = namei_lookup_parent(ctx, path, flags, &parent);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_create_plus(&parent.parent_fuid, parent.name, attr, flags, out);
}

fs_error_t namei_mkdir(const namei_ctx_t *ctx,
                       const char *path,
                       const fops_create_attr_t *attr,
                       fs_flags_t flags,
                       fops_object_result_t *out)
{
    fs_error_t err;
    namei_parent_result_t parent;

    err = namei_lookup_parent(ctx, path, flags, &parent);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_mkdir_plus(&parent.parent_fuid, parent.name, attr, flags, out);
}

fs_error_t namei_symlink(const namei_ctx_t *ctx,
                         const char *target,
                         const char *linkpath,
                         fs_flags_t flags,
                         fops_object_result_t *out)
{
    fs_error_t err;
    namei_parent_result_t parent;

    err = namei_lookup_parent(ctx, linkpath, flags, &parent);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_symlink_plus(&parent.parent_fuid, parent.name, target,
                                    flags, out);
}

fs_error_t namei_readlink(const namei_ctx_t *ctx,
                          const char *path,
                          fs_flags_t flags,
                          char *buf,
                          size_t size,
                          size_t *actual)
{
    fs_error_t err;
    namei_parent_result_t parent;

    err = namei_lookup_parent(ctx, path, flags, &parent);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_readlink(&parent.parent_fuid, parent.name,
                            flags | FS_FLAG_NOFOLLOW, buf, size, actual);
}

fs_error_t namei_readdir(const namei_ctx_t *ctx,
                         const char *path,
                         fs_flags_t flags,
                         fops_dirent_t *entries,
                         uint32_t entry_cap,
                         uint32_t *out_entry_nr,
                         bool *out_eof)
{
    fs_error_t err;
    fuid_t fuid;

    err = namei_lookup(ctx, path, flags | FS_FLAG_DIRECTORY, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_readdir(&fuid, flags | FS_FLAG_DIRECTORY, entries,
                          entry_cap, out_entry_nr, out_eof);
}

fs_error_t namei_readdirplus(const namei_ctx_t *ctx,
                             const char *path,
                             fs_flags_t flags,
                             fops_dirent_plus_t *entries,
                             uint32_t entry_cap,
                             uint32_t *out_entry_nr,
                             bool *out_eof)
{
    fs_error_t err;
    fuid_t fuid;

    err = namei_lookup(ctx, path, flags | FS_FLAG_DIRECTORY, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_readdirplus(&fuid, flags | FS_FLAG_DIRECTORY, entries,
                              entry_cap, out_entry_nr, out_eof);
}

fs_error_t namei_unlink(const namei_ctx_t *ctx,
                        const char *path,
                        fs_flags_t flags)
{
    fs_error_t err;
    namei_parent_result_t parent;

    err = namei_lookup_parent(ctx, path, flags, &parent);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_unlink(&parent.parent_fuid, parent.name, flags);
}

fs_error_t namei_rmdir(const namei_ctx_t *ctx,
                       const char *path,
                       fs_flags_t flags)
{
    fs_error_t err;
    namei_parent_result_t parent;

    err = namei_lookup_parent(ctx, path, flags, &parent);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_rmdir(&parent.parent_fuid, parent.name, flags);
}

fs_error_t namei_rename(const namei_ctx_t *ctx,
                        const char *old_path,
                        const char *new_path,
                        fs_flags_t flags)
{
    fs_error_t err;
    namei_parent_result_t old_parent;
    namei_parent_result_t new_parent;

    err = namei_lookup_parent(ctx, old_path, flags, &old_parent);
    if (fs_failed(err)) {
        return err;
    }
    err = namei_lookup_parent(ctx, new_path, flags, &new_parent);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_rename(&old_parent.parent_fuid, old_parent.name,
                          &new_parent.parent_fuid, new_parent.name, flags);
}

fs_error_t namei_getattr(const namei_ctx_t *ctx,
                         const char *path,
                         fs_flags_t flags,
                         fops_attr_t *out_attr)
{
    fs_error_t err;
    fuid_t fuid;

    err = namei_lookup(ctx, path, flags, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_getattr(&fuid, namei_type_flags(flags), out_attr);
}

fs_error_t namei_open(const namei_ctx_t *ctx,
                      const char *path,
                      fs_flags_t flags,
                      fops_file_t **out_file)
{
    fs_error_t err;
    fuid_t fuid;

    err = namei_lookup(ctx, path, flags, &fuid);
    if (fs_failed(err)) {
        return err;
    }

    return namei_dispatch_open(&fuid, flags, out_file);
}

