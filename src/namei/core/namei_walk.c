#include "namei/internal/namei_internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "fops/include/fops.h"
#include "namei/internal/namei_error.h"

static fs_metric_id_t g_namei_walk_metric_id = FS_METRIC_ID_INVALID;

void namei_metrics_init(void)
{
    g_namei_walk_metric_id = FS_METRIC_ID_INVALID;
    (void)fs_metrics_register("namei", "WALK", true,
                              &g_namei_walk_metric_id);
}

static const char *namei_skip_slashes(const char *path)
{
    while ((path != NULL) && (*path == '/'))
    {
        path++;
    }

    return path;
}

bool namei_path_is_absolute(const char *path)
{
    return (path != NULL) && (path[0] == '/');
}

bool namei_path_has_trailing_slash(const char *path)
{
    size_t len;

    if ((path == NULL) || (path[0] == '\0'))
    {
        return false;
    }

    len = strlen(path);
    while ((len > 1U) && (path[len - 1U] == '/'))
    {
        return true;
    }

    return false;
}

fs_error_t namei_ctx_check(const namei_ctx_t *ctx)
{
    fs_error_t err;

    if ((ctx == NULL) || !fuid_is_valid(&ctx->root_fuid) ||
        !fuid_is_valid(&ctx->cwd_fuid) || !fuid_is_dir(&ctx->root_fuid) ||
        !fuid_is_dir(&ctx->cwd_fuid) ||
        (ctx->root_fuid.fsid != ctx->cwd_fuid.fsid))
    {
        err = namei_error(NAMEI_SUB_CTX, EINVAL);
        FS_LOG_DUMP_ERROR("ctx check failed, err=%s (0x%x)", fs_error_str(err),
                          err);
        return err;
    }

    return FS_OK;
}


static fs_error_t namei_dispatch_lookup_plus(const fuid_t *parent_fuid,
                                             const char *name, fs_flags_t flags,
                                             fops_object_result_t *out)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_LOOKUP;
    args.flags = flags;
    args.parent_fuid = parent_fuid;
    args.name = name;
    args.u.lookup.out = out;

    return fops_dispatch(&args);
}

static fs_error_t namei_dispatch_readlink(const fuid_t *parent_fuid,
                                          const char *name, fs_flags_t flags,
                                          char *buf, size_t size,
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

static fs_error_t namei_next_component(const char **cursor, char *name,
                                       bool *has_more)
{
    const char *p;
    const char *start;
    size_t len;

    p = namei_skip_slashes(*cursor);
    if (*p == '\0')
    {
        name[0] = '\0';
        *cursor = p;
        *has_more = false;
        return FS_OK;
    }

    start = p;
    while ((*p != '\0') && (*p != '/'))
    {
        p++;
    }

    len = (size_t)(p - start);
    if ((len == 0U) || (len > FS_MAX_NAME_LEN))
    {
        return namei_error(NAMEI_SUB_PATH, ENAMETOOLONG);
    }

    memcpy(name, start, len);
    name[len] = '\0';

    *cursor = p;
    *has_more = (*namei_skip_slashes(p) != '\0');
    return FS_OK;
}

static fs_error_t namei_make_remainder(char *dst, size_t size,
                                       const char *target, const char *rest)
{
    int ret;

    rest = namei_skip_slashes(rest);
    if ((target == NULL) || (target[0] == '\0'))
    {
        return namei_error(NAMEI_SUB_WALK, EINVAL);
    }

    if ((rest == NULL) || (rest[0] == '\0'))
    {
        ret = snprintf(dst, size, "%s", target);
    }
    else
    {
        ret = snprintf(dst, size, "%s/%s", target, rest);
    }

    if ((ret < 0) || ((size_t)ret >= size))
    {
        return namei_error(NAMEI_SUB_WALK, ENAMETOOLONG);
    }

    return FS_OK;
}

static fs_error_t namei_walk_impl(const namei_ctx_t *ctx, const char *path,
                                  fs_flags_t flags, namei_walk_result_t *out)
{
    fs_error_t err;
    fuid_t current;
    fops_object_result_t result;
    const char *cursor;
    uint32_t symlink_depth;
    uint32_t symlink_max;
    char todo[FS_MAX_PATH_LEN + 1U];
    char name[FS_MAX_NAME_LEN + 1U];
    char target[FS_MAX_PATH_LEN + 1U];
    char rest_buf[FS_MAX_PATH_LEN + 1U];
    size_t actual;
    bool has_more;
    bool final;

    FS_LOG_DUMP_INFO("enter: ctx=%p, path=%s, flags=0x%x", (void *)ctx,
                     path ? path : "(null)", flags);

    if (out == NULL)
    {
        return namei_error(NAMEI_SUB_WALK, EINVAL);
    }
    memset(out, 0, sizeof(*out));
    fuid_set_invalid(&out->fuid);

    err = namei_ctx_check(ctx);
    if (fs_failed(err))
    {
        return err;
    }
    if ((path == NULL) || (path[0] == '\0') || (strlen(path) > FS_MAX_PATH_LEN))
    {
        return namei_error(NAMEI_SUB_PATH, EINVAL);
    }

    current = namei_path_is_absolute(path) ? ctx->root_fuid : ctx->cwd_fuid;
    symlink_depth = 0U;
    symlink_max = (ctx->max_symlink_depth == 0U) ? NAMEI_SYMLINK_MAX
                                                 : ctx->max_symlink_depth;

    if (snprintf(todo, sizeof(todo), "%s", path) >= (int)sizeof(todo))
    {
        return namei_error(NAMEI_SUB_PATH, ENAMETOOLONG);
    }
    cursor = todo;

    while (1)
    {
        err = namei_next_component(&cursor, name, &has_more);
        if (fs_failed(err))
        {
            return err;
        }
        if (name[0] == '\0')
        {
            out->fuid = current;
            out->has_attr = false;
            FS_LOG_DUMP_INFO("exit: ok");
            return FS_OK;
        }

        if (strcmp(name, ".") == 0)
        {
            continue;
        }
        if ((strcmp(name, "..") == 0) && fuid_equal(&current, &ctx->root_fuid))
        {
            continue;
        }

        final = !has_more;
        memset(&result, 0, sizeof(result));
        fuid_set_invalid(&result.fuid);

        err = namei_dispatch_lookup_plus(&current, name, FS_FLAG_NOFOLLOW,
                                         &result);
        if (fs_failed(err))
        {
            return err;
        }

        if ((result.attr.type == FS_TYPE_LNK) &&
            !(final && fs_flag_test(flags, FS_FLAG_NOFOLLOW)))
        {
            if (++symlink_depth > symlink_max)
            {
                return namei_error(NAMEI_SUB_WALK, ELOOP);
            }

            memset(target, 0, sizeof(target));
            actual = 0U;
            err = namei_dispatch_readlink(&current, name, FS_FLAG_NOFOLLOW,
                                          target, sizeof(target) - 1U, &actual);
            if (fs_failed(err))
            {
                return err;
            }
            target[actual] = '\0';

            if (namei_path_is_absolute(target))
            {
                current = ctx->root_fuid;
            }

            if (snprintf(rest_buf, sizeof(rest_buf), "%s",
                         namei_skip_slashes(cursor)) >= (int)sizeof(rest_buf))
            {
                return namei_error(NAMEI_SUB_WALK, ENAMETOOLONG);
            }

            err = namei_make_remainder(todo, sizeof(todo), target, rest_buf);
            if (fs_failed(err))
            {
                return err;
            }
            cursor = todo;
            continue;
        }

        if (has_more && (result.attr.type != FS_TYPE_DIR))
        {
            return namei_error(NAMEI_SUB_WALK, ENOTDIR);
        }

        current = result.fuid;
        if (final)
        {
            out->fuid = result.fuid;
            out->attr = result.attr;
            out->has_attr = true;
            FS_LOG_DUMP_INFO("exit: ok");
            return FS_OK;
        }
    }
}

fs_error_t namei_walk(const namei_ctx_t *ctx, const char *path,
                      fs_flags_t flags, namei_walk_result_t *out)
{
    fs_metrics_token_t token;
    fs_error_t err;

    token = fs_metrics_begin(g_namei_walk_metric_id);
    err = namei_walk_impl(ctx, path, flags, out);
    fs_metrics_end(token, fs_succeeded(err), 0U);
    return err;
}
