#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

typedef struct fops_readdir_resolve_ctx {
    const fuid_t *dir_fuid;     /* 目录对象 FUID */
    obj_meta_t *dir_meta;       /* 目录对象元数据 */
    int dir_fd;                 /* 已打开的目录 fd */
    fs_op_t sub;                /* 当前操作字 */
} fops_readdir_resolve_ctx_t;

typedef struct fops_readdir_ctx {
    const fuid_t *dir_fuid;         /* 待读取目录 */
    fs_flags_t flags;               /* READDIR/READDIRPLUS 标志 */
    fops_dirent_t *entries;         /* readdir 输出数组 */
    fops_dirent_plus_t *plus_entries; /* readdirplus 输出数组 */
    uint32_t entry_cap;             /* 输出数组容量 */
    uint32_t *out_entry_nr;         /* 实际返回条目数 */
    bool *out_eof;                  /* 是否已经读到目录末尾 */
    bool need_attr;                 /* 是否需要返回 stat 属性 */
    fs_op_t sub;                    /* 当前操作字 */
} fops_readdir_ctx_t;

static bool fops_readdir_skip_name(const char *name)
{
    return (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0);
}

static fs_error_t fops_readdir_validate_ctx(const fops_readdir_ctx_t *ctx)
{
    fs_error_t err;

    if ((ctx == NULL) ||
        (ctx->dir_fuid == NULL) ||
        (ctx->entry_cap == 0U) ||
        (ctx->out_entry_nr == NULL) ||
        (ctx->out_eof == NULL) ||
        (ctx->need_attr ? (ctx->plus_entries == NULL) :
                          (ctx->entries == NULL))) {
        err = fops_error((ctx == NULL) ? FS_OP_READDIR : ctx->sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid readdir args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (!fuid_is_dir(ctx->dir_fuid)) {
        err = fops_error(ctx->sub, ENOTDIR);
        FS_LOG_DUMP_ERROR("readdir failed: object is not dir, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    return fops_validate_readdir_flags(ctx->flags, ctx->sub);
}

static fs_error_t fops_readdir_resolve_entry(
                                    const fops_readdir_resolve_ctx_t *ctx,
                                    const char *name,
                                    fs_type_t type_hint,
                                    fuid_t *out_fuid)
{
    fs_error_t err;
    lsa_file_handle_t lsa_handle;
    obj_handle_t handle;
    int32_t mount_id;
    struct stat st;
    fs_type_t type;

    type = type_hint;
    if (type == FS_TYPE_UNKNOWN) {
        err = lsa_fstatat(ctx->dir_fd, name, FS_FLAG_NOFOLLOW, &st);
        if (fs_failed(err)) {
            return err;
        }
        type = fops_type_from_mode(st.st_mode);
    }

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    mount_id = 0;
    err = lsa_name_to_handle_at(ctx->dir_fd, name, &lsa_handle, &mount_id, 0);
    if (fs_failed(err)) {
        return err;
    }

    err = fops_handle_from_lsa_checked(&handle,
                                       &lsa_handle,
                                       mount_id,
                                       ctx->dir_meta,
                                       ctx->sub);
    if (fs_failed(err)) {
        return err;
    }

    return fops_fuid_from_handle(ctx->dir_fuid, &handle, type,
                                 out_fuid, ctx->sub);
}

static fs_error_t fops_readdir_copy_plain(
                                    const fops_readdir_resolve_ctx_t *rctx,
                                    const lsa_dirent_t *src,
                                    fops_dirent_t *dst)
{
    fs_error_t err;

    err = fops_readdir_resolve_entry(rctx, src->name, src->type, &dst->fuid);
    if (fs_failed(err)) {
        return err;
    }

    strncpy(dst->name, src->name, FS_MAX_NAME_LEN);
    dst->name[FS_MAX_NAME_LEN] = '\0';
    return FS_OK;
}

static fs_error_t fops_readdir_copy_plus(
                                    const fops_readdir_resolve_ctx_t *rctx,
                                    const lsa_dirent_plus_t *src,
                                    fops_dirent_plus_t *dst)
{
    fs_error_t err;
    fs_type_t type;

    type = fops_type_from_mode(src->st.st_mode);
    err = fops_readdir_resolve_entry(rctx, src->entry.name, type,
                                     &dst->entry.fuid);
    if (fs_failed(err)) {
        return err;
    }

    strncpy(dst->entry.name, src->entry.name, FS_MAX_NAME_LEN);
    dst->entry.name[FS_MAX_NAME_LEN] = '\0';
    fops_attr_from_stat(&dst->attr, &src->st);
    return FS_OK;
}

static fs_error_t fops_readdir_common(const fops_readdir_ctx_t *ctx)
{
    fs_error_t err;
    obj_meta_t *dir_meta;
    int dir_fd;
    lsa_dir_iter_t *iter;
    lsa_dirent_t entry;
    lsa_dirent_plus_t plus_entry;
    fops_readdir_resolve_ctx_t rctx;
    uint32_t copied;

    dir_meta = NULL;
    dir_fd = -1;
    iter = NULL;
    copied = 0U;
    err = FS_OK;

    if ((ctx != NULL) && (ctx->out_entry_nr != NULL)) {
        *ctx->out_entry_nr = 0U;
    }
    if ((ctx != NULL) && (ctx->out_eof != NULL)) {
        *ctx->out_eof = false;
    }

    err = fops_readdir_validate_ctx(ctx);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_open_object(ctx->dir_fuid,
                           O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                           &dir_meta,
                           &dir_fd,
                           ctx->sub);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_dir_iter_open(dir_fd, 0U, &iter);
    if (fs_failed(err)) {
        goto out;
    }

    rctx.dir_fuid = ctx->dir_fuid;
    rctx.dir_meta = dir_meta;
    rctx.dir_fd = dir_fd;
    rctx.sub = ctx->sub;

    while (copied < ctx->entry_cap) {
        if (ctx->need_attr) {
            memset(&plus_entry, 0, sizeof(plus_entry));
            err = lsa_dir_iter_next_plus(iter, &plus_entry);
            if (fs_failed(err)) {
                if (fs_err_errno(err) == FS_ERRNO_ENOENT) {
                    *ctx->out_eof = true;
                    err = FS_OK;
                }
                break;
            }
            if (fops_readdir_skip_name(plus_entry.entry.name)) {
                continue;
            }
            err = fops_readdir_copy_plus(&rctx, &plus_entry,
                                         &ctx->plus_entries[copied]);
        } else {
            memset(&entry, 0, sizeof(entry));
            err = lsa_dir_iter_next(iter, &entry);
            if (fs_failed(err)) {
                if (fs_err_errno(err) == FS_ERRNO_ENOENT) {
                    *ctx->out_eof = true;
                    err = FS_OK;
                }
                break;
            }
            if (fops_readdir_skip_name(entry.name)) {
                continue;
            }
            err = fops_readdir_copy_plain(&rctx, &entry,
                                          &ctx->entries[copied]);
        }

        if (fs_failed(err)) {
            break;
        }
        copied++;
    }

    *ctx->out_entry_nr = copied;

out:
    if (iter != NULL) {
        (void)lsa_dir_iter_close(iter);
    }
    fops_close_object(dir_meta, dir_fd);
    return err;
}

fs_error_t fops_readdir(const fuid_t *dir_fuid,
                        fs_flags_t flags,
                        fops_dirent_t *entries,
                        uint32_t entry_cap,
                        uint32_t *out_entry_nr,
                        bool *out_eof)
{
    fops_readdir_ctx_t ctx;

    ctx.dir_fuid = dir_fuid;
    ctx.flags = flags;
    ctx.entries = entries;
    ctx.plus_entries = NULL;
    ctx.entry_cap = entry_cap;
    ctx.out_entry_nr = out_entry_nr;
    ctx.out_eof = out_eof;
    ctx.need_attr = false;
    ctx.sub = FS_OP_READDIR;

    return fops_readdir_common(&ctx);
}

fs_error_t fops_readdirplus(const fuid_t *dir_fuid,
                            fs_flags_t flags,
                            fops_dirent_plus_t *entries,
                            uint32_t entry_cap,
                            uint32_t *out_entry_nr,
                            bool *out_eof)
{
    fops_readdir_ctx_t ctx;

    ctx.dir_fuid = dir_fuid;
    ctx.flags = flags;
    ctx.entries = NULL;
    ctx.plus_entries = entries;
    ctx.entry_cap = entry_cap;
    ctx.out_entry_nr = out_entry_nr;
    ctx.out_eof = out_eof;
    ctx.need_attr = true;
    ctx.sub = FS_OP_READDIRPLUS;

    return fops_readdir_common(&ctx);
}
