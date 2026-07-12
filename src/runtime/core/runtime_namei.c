#include "runtime/internal/runtime_internal.h"

fs_error_t runtime_lookup(const char *path, fs_flags_t flags, fuid_t *out_fuid)
{
    return runtime_lookup_fuid(path, flags, out_fuid);
}

fs_error_t runtime_lookup_plus(const char *path,
                               fs_flags_t flags,
                               fops_object_result_t *out)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_lookup_plus(&ctx, path, flags, out);
}

fs_error_t runtime_lookup_parent(const char *path,
                                 fs_flags_t flags,
                                 namei_parent_result_t *out)
{
    return runtime_lookup_parent_path(path, flags, out);
}

fs_error_t runtime_create(const char *path,
                          const fops_create_attr_t *attr,
                          fs_flags_t flags,
                          fops_object_result_t *out)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_create(&ctx, path, attr, flags, out);
}

fs_error_t runtime_mkdir(const char *path,
                         const fops_create_attr_t *attr,
                         fs_flags_t flags,
                         fops_object_result_t *out)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_mkdir(&ctx, path, attr, flags, out);
}

fs_error_t runtime_unlink(const char *path, fs_flags_t flags)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_unlink(&ctx, path, flags);
}

fs_error_t runtime_rmdir(const char *path, fs_flags_t flags)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_rmdir(&ctx, path, flags);
}

fs_error_t runtime_rename(const char *old_path,
                          const char *new_path,
                          fs_flags_t flags)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_rename(&ctx, old_path, new_path, flags);
}

fs_error_t runtime_symlink(const char *target,
                           const char *linkpath,
                           fs_flags_t flags,
                           fops_object_result_t *out)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_symlink(&ctx, target, linkpath, flags, out);
}

fs_error_t runtime_readlink(const char *path,
                            fs_flags_t flags,
                            char *buf,
                            size_t size,
                            size_t *actual)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_readlink(&ctx, path, flags, buf, size, actual);
}

fs_error_t runtime_readdir(const char *path,
                           fs_flags_t flags,
                           fops_dirent_t *entries,
                           uint32_t entry_cap,
                           uint32_t *out_entry_nr,
                           bool *out_eof)
{
    fs_error_t err;
    namei_ctx_t ctx;
    namei_readdir_args_t args;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    args.entries = entries;
    args.entry_cap = entry_cap;
    args.out_entry_nr = out_entry_nr;
    args.out_eof = out_eof;

    return namei_readdir(&ctx, path, flags, &args);
}

fs_error_t runtime_readdirplus(const char *path,
                               fs_flags_t flags,
                               fops_dirent_plus_t *entries,
                               uint32_t entry_cap,
                               uint32_t *out_entry_nr,
                               bool *out_eof)
{
    fs_error_t err;
    namei_ctx_t ctx;
    namei_readdirplus_args_t args;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    args.entries = entries;
    args.entry_cap = entry_cap;
    args.out_entry_nr = out_entry_nr;
    args.out_eof = out_eof;

    return namei_readdirplus(&ctx, path, flags, &args);
}

fs_error_t runtime_getattr(const char *path,
                           fs_flags_t flags,
                           fops_attr_t *out_attr)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_getattr(&ctx, path, flags, out_attr);
}

fs_error_t runtime_open(const char *path,
                        fs_flags_t flags,
                        fops_file_t **out_file)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err)) {
        return err;
    }

    return namei_open(&ctx, path, flags, out_file);
}
