#include "runtime/internal/runtime_internal.h"

fs_error_t runtime_fs_create(const char *name, fuid_t *root_out)
{
    fs_error_t err;
    fuid_t root_fuid;

    if (root_out != NULL) {
        fuid_set_invalid(root_out);
    }

    err = runtime_require_initialized();
    if (fs_failed(err)) {
        return err;
    }

    err = fsmgr_create(name, &root_fuid);
    if (fs_failed(err)) {
        return err;
    }

    if (root_out != NULL) {
        *root_out = root_fuid;
    }

    return FS_OK;
}

fs_error_t runtime_fs_destroy(const char *name)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    err = runtime_require_initialized();
    if (fs_failed(err)) {
        return err;
    }

    ns = fsmgr_lookup(name);
    if (ns == NULL) {
        return runtime_error(RUNTIME_SUB_NAMESPACE, ENOENT);
    }

    if (g_runtime.ns_active &&
        fuid_equal(&g_runtime.root_fuid, &ns->root_fuid)) {
        err = runtime_fs_leave();
        if (fs_failed(err)) {
            return err;
        }
    }

    return fsmgr_destroy(ns->fsid);
}

fs_error_t runtime_fs_use(const char *name)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    err = runtime_require_initialized();
    if (fs_failed(err)) {
        return err;
    }

    ns = fsmgr_lookup(name);
    if (ns == NULL) {
        return runtime_error(RUNTIME_SUB_NAMESPACE, ENOENT);
    }

    g_runtime.root_fuid = ns->root_fuid;
    g_runtime.cwd_fuid = ns->root_fuid;
    g_runtime.ns_active = true;

    if (snprintf(g_runtime.namespace_name, sizeof(g_runtime.namespace_name),
                 "%s", name) >= (int)sizeof(g_runtime.namespace_name)) {
        (void)runtime_fs_leave();
        return runtime_error(RUNTIME_SUB_NAMESPACE, ENAMETOOLONG);
    }

    if (snprintf(g_runtime.cwd_path, sizeof(g_runtime.cwd_path),
                 "/") >= (int)sizeof(g_runtime.cwd_path)) {
        (void)runtime_fs_leave();
        return runtime_error(RUNTIME_SUB_PATH, ENAMETOOLONG);
    }

    return FS_OK;
}

fs_error_t runtime_fs_leave(void)
{
    fs_error_t err;

    err = runtime_require_initialized();
    if (fs_failed(err)) {
        return err;
    }

    g_runtime.ns_active = false;
    g_runtime.namespace_name[0] = 0;
    g_runtime.cwd_path[0] = 0;
    fuid_set_invalid(&g_runtime.root_fuid);
    fuid_set_invalid(&g_runtime.cwd_fuid);

    return FS_OK;
}

bool runtime_fs_is_active(void)
{
    return g_runtime.initialized && g_runtime.ns_active;
}

const char *runtime_fs_current(void)
{
    if (!runtime_fs_is_active()) {
        return NULL;
    }

    return g_runtime.namespace_name;
}

fs_error_t runtime_get_ctx(namei_ctx_t *out_ctx)
{
    return runtime_make_ctx(out_ctx);
}

fs_error_t runtime_get_root(fuid_t *out_fuid)
{
    fs_error_t err;

    if (out_fuid == NULL) {
        return runtime_error(RUNTIME_SUB_CTX, EINVAL);
    }
    fuid_set_invalid(out_fuid);

    err = runtime_require_session();
    if (fs_failed(err)) {
        return err;
    }

    *out_fuid = g_runtime.root_fuid;
    return FS_OK;
}

fs_error_t runtime_get_cwd(fuid_t *out_fuid)
{
    fs_error_t err;

    if (out_fuid == NULL) {
        return runtime_error(RUNTIME_SUB_CTX, EINVAL);
    }
    fuid_set_invalid(out_fuid);

    err = runtime_require_session();
    if (fs_failed(err)) {
        return err;
    }

    *out_fuid = g_runtime.cwd_fuid;
    return FS_OK;
}

fs_error_t runtime_getcwd(char *buf, size_t size)
{
    fs_error_t err;

    if ((buf == NULL) || (size == 0U)) {
        return runtime_error(RUNTIME_SUB_PATH, EINVAL);
    }
    buf[0] = 0;

    err = runtime_require_session();
    if (fs_failed(err)) {
        return err;
    }

    if (snprintf(buf, size, "%s", g_runtime.cwd_path) >= (int)size) {
        return runtime_error(RUNTIME_SUB_PATH, ENAMETOOLONG);
    }

    return FS_OK;
}

fs_error_t runtime_chdir(const char *path)
{
    fs_error_t err;
    fuid_t target_fuid;

    err = runtime_lookup_fuid(path, FS_FLAG_DIRECTORY, &target_fuid);
    if (fs_failed(err)) {
        return err;
    }

    err = runtime_update_cwd_path(path);
    if (fs_failed(err)) {
        return err;
    }

    g_runtime.cwd_fuid = target_fuid;
    return FS_OK;
}
