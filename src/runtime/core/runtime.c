#include "runtime/internal/runtime_internal.h"
runtime_state_t g_runtime;


fs_error_t runtime_require_initialized(void)
{
    if (!g_runtime.initialized)
    {
        return runtime_error(RUNTIME_SUB_INIT, EINVAL);
    }

    return FS_OK;
}

fs_error_t runtime_require_session(void)
{
    fs_error_t err;

    err = runtime_require_initialized();
    if (fs_failed(err))
    {
        return err;
    }

    if (!g_runtime.ns_active || !fuid_is_valid(&g_runtime.root_fuid) ||
        !fuid_is_valid(&g_runtime.cwd_fuid))
    {
        return runtime_error(RUNTIME_SUB_SESSION, ENOENT);
    }

    return FS_OK;
}

fs_error_t runtime_make_ctx(namei_ctx_t *ctx)
{
    fs_error_t err;

    if (ctx == NULL)
    {
        return runtime_error(RUNTIME_SUB_CTX, EINVAL);
    }

    err = runtime_require_session();
    if (fs_failed(err))
    {
        return err;
    }

    namei_ctx_make(ctx, &g_runtime.root_fuid, &g_runtime.cwd_fuid);
    return FS_OK;
}

fs_error_t runtime_dispatch(fops_args_t *args)
{
    if (args == NULL)
    {
        return runtime_error(RUNTIME_SUB_OP, EINVAL);
    }

    return fops_dispatch(args);
}

fs_error_t runtime_lookup_fuid(const char *path, fs_flags_t flags,
                               fuid_t *out_fuid)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err))
    {
        return err;
    }

    return namei_lookup(&ctx, path, flags, out_fuid);
}

fs_error_t runtime_lookup_parent_path(const char *path, fs_flags_t flags,
                                      namei_parent_result_t *out)
{
    fs_error_t err;
    namei_ctx_t ctx;

    err = runtime_make_ctx(&ctx);
    if (fs_failed(err))
    {
        return err;
    }

    return namei_lookup_parent(&ctx, path, flags, out);
}

fs_error_t runtime_update_cwd_path(const char *path)
{
    fs_error_t err;
    char joined[FS_MAX_PATH_LEN + 1U];
    char normalized[FS_MAX_PATH_LEN + 1U];

    if ((path == NULL) || (path[0] == 0))
    {
        return runtime_error(RUNTIME_SUB_PATH, EINVAL);
    }

    if (fs_path_is_absolute(path))
    {
        err = fs_path_normalize(normalized, sizeof(normalized), path);
    }
    else
    {
        err = fs_path_join_safe(joined, sizeof(joined), g_runtime.cwd_path,
                                path);
        if (fs_failed(err))
        {
            return err;
        }
        err = fs_path_normalize(normalized, sizeof(normalized), joined);
    }
    if (fs_failed(err))
    {
        return err;
    }

    if (normalized[0] != 47)
    {
        return runtime_error(RUNTIME_SUB_PATH, EINVAL);
    }

    if (snprintf(g_runtime.cwd_path, sizeof(g_runtime.cwd_path), "%s",
                 normalized) >= (int)sizeof(g_runtime.cwd_path))
    {
        return runtime_error(RUNTIME_SUB_PATH, ENAMETOOLONG);
    }

    return FS_OK;
}

fs_error_t runtime_init(const runtime_config_t *cfg)
{
    fs_error_t err;

    if (g_runtime.initialized)
    {
        return runtime_error(RUNTIME_SUB_INIT, EALREADY);
    }

    memset(&g_runtime, 0, sizeof(g_runtime));
    fuid_set_invalid(&g_runtime.root_fuid);
    fuid_set_invalid(&g_runtime.cwd_fuid);

    fs_config_init();
    fs_metrics_init(g_fs_config.metrics.mode);
    FS_TRACE_BEGIN(&g_runtime.trace_ctx);
    fs_log_init(NULL, FS_LOG_INFO);
    fs_sub_register(FS_MODULE_RUNTIME, (fs_sub_name_fn)runtime_sub_name);

    lsa_init();

    err = object_init();
    if (fs_failed(err))
    {
        goto err_trace;
    }

    err = fsc_init();
    if (fs_failed(err))
    {
        goto err_object;
    }

    err = fops_init();
    if (fs_failed(err))
    {
        goto err_fsc;
    }

    err = namei_init();
    if (fs_failed(err))
    {
        goto err_fops;
    }

    fs_metrics_freeze();
    runtime_monitoring_init(&g_fs_config.metrics);
    g_runtime.initialized = true;

    if ((cfg != NULL) && (cfg->default_namespace != NULL))
    {
        if (cfg->auto_create)
        {
            err = runtime_fs_create(cfg->default_namespace, NULL);
            if (fs_failed(err) && !fsmgr_exists(cfg->default_namespace))
            {
                goto err_namei;
            }
        }
        if (cfg->auto_use)
        {
            err = runtime_fs_use(cfg->default_namespace);
            if (fs_failed(err))
            {
                goto err_namei;
            }
        }
    }

    return FS_OK;

err_namei:
    g_runtime.initialized = false;
    runtime_monitoring_deinit();
    namei_deinit();

err_fops:
    fops_deinit();

err_fsc:
    fsc_deinit();

err_object:
    object_deinit();

err_trace:
    FS_TRACE_END();
    fs_metrics_deinit();
    memset(&g_runtime, 0, sizeof(g_runtime));
    return err;
}

void runtime_deinit(void)
{
    if (!g_runtime.initialized)
    {
        return;
    }

    (void)runtime_fs_leave();

    namei_deinit();
    fops_deinit();
    fsc_deinit();
    object_deinit();
    FS_TRACE_END();
    runtime_monitoring_deinit();
    fs_metrics_deinit();

    memset(&g_runtime, 0, sizeof(g_runtime));
}

bool runtime_is_initialized(void)
{
    return g_runtime.initialized;
}
