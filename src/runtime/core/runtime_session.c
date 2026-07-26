#include "runtime/internal/runtime_internal.h"

fs_error_t runtime_fs_create(const char *name, fuid_t *root_out)
{
    fs_error_t err;
    fuid_t root_fuid;

    if (root_out != NULL)
    {
        fuid_set_invalid(root_out);
    }

    err = runtime_require_initialized();
    if (fs_failed(err))
    {
        return err;
    }

    err = fsmgr_create(name, &root_fuid);
    if (fs_failed(err))
    {
        return err;
    }

    if (root_out != NULL)
    {
        *root_out = root_fuid;
    }

    return FS_OK;
}

fs_error_t runtime_fs_destroy(const char *name)
{
    fs_error_t err;
    fsc_namespace_t *ns;
    bool destroying_current;
    bool had_session;
    fuid_t saved_root;
    fuid_t saved_cwd;
    char saved_name[FSC_NAMESPACE_NAME_MAX];
    char saved_cwd_path[FS_MAX_PATH_LEN + 1U];

    err = runtime_require_initialized();
    if (fs_failed(err))
    {
        return err;
    }

    ns = fsmgr_lookup(name);
    if (ns == NULL)
    {
        return runtime_error(RUNTIME_SUB_NAMESPACE, ENOENT);
    }

    had_session = g_runtime.ns_active;
    destroying_current =
            had_session && (strcmp(g_runtime.namespace_name, name) == 0);
    saved_root = g_runtime.root_fuid;
    saved_cwd = g_runtime.cwd_fuid;
    (void)snprintf(saved_name, sizeof(saved_name), "%s",
                   g_runtime.namespace_name);
    (void)snprintf(saved_cwd_path, sizeof(saved_cwd_path), "%s",
                   g_runtime.cwd_path);

    err = fsmgr_destroy(ns->fsid);
    if (fs_failed(err))
    {
        if (had_session)
        {
            g_runtime.initialized = true;
            g_runtime.ns_active = true;
            g_runtime.root_fuid = saved_root;
            g_runtime.cwd_fuid = saved_cwd;
            (void)snprintf(g_runtime.namespace_name,
                           sizeof(g_runtime.namespace_name), "%s", saved_name);
            (void)snprintf(g_runtime.cwd_path, sizeof(g_runtime.cwd_path), "%s",
                           saved_cwd_path);
        }
        return err;
    }

    if (destroying_current)
    {
        return runtime_fs_leave();
    }

    return FS_OK;
}

static bool runtime_fs_tree_skip_name(const char *name)
{
    return (name == NULL) || (strcmp(name, ".") == 0) ||
           (strcmp(name, "..") == 0);
}

static fs_error_t runtime_fs_join_child_path(char *out, size_t out_size,
                                             const char *parent,
                                             const char *name)
{
    if ((out == NULL) || (parent == NULL) || (name == NULL))
    {
        return runtime_error(RUNTIME_SUB_PATH, EINVAL);
    }

    if (strcmp(parent, "/") == 0)
    {
        if (snprintf(out, out_size, "/%s", name) >= (int)out_size)
        {
            return runtime_error(RUNTIME_SUB_PATH, ENAMETOOLONG);
        }
    }
    else if (snprintf(out, out_size, "%s/%s", parent, name) >= (int)out_size)
    {
        return runtime_error(RUNTIME_SUB_PATH, ENAMETOOLONG);
    }

    return FS_OK;
}

static fs_error_t runtime_fs_remove_children(const char *path)
{
    fops_dirent_plus_t entries[64];
    uint32_t entry_nr;
    bool eof;
    fs_error_t err;
    char child_path[FS_MAX_PATH_LEN + 1U];

    eof = false;
    while (!eof)
    {
        err = runtime_readdirplus(path, FS_FLAG_DIRECTORY, entries, 64U,
                                  &entry_nr, &eof);
        if (fs_failed(err))
        {
            return err;
        }

        if (entry_nr == 0U)
        {
            break;
        }

        for (uint32_t index = 0U; index < entry_nr; index++)
        {
            if (runtime_fs_tree_skip_name(entries[index].entry.name))
            {
                continue;
            }

            err = runtime_fs_join_child_path(child_path, sizeof(child_path),
                                             path, entries[index].entry.name);
            if (fs_failed(err))
            {
                return err;
            }

            if (entries[index].attr.type == FS_TYPE_DIR)
            {
                err = runtime_fs_remove_children(child_path);
                if (fs_failed(err))
                {
                    return err;
                }

                err = runtime_rmdir(child_path, FS_FLAG_DIRECTORY);
            }
            else
            {
                err = runtime_unlink(child_path, FS_FLAG_NOFOLLOW);
            }

            if (fs_failed(err))
            {
                return err;
            }
        }
    }

    return FS_OK;
}

fs_error_t runtime_fs_destroy_tree(const char *name)
{
    fs_error_t err;
    bool had_session;
    bool destroying_current;
    char previous_name[FSC_NAMESPACE_NAME_MAX];

    err = runtime_require_initialized();
    if (fs_failed(err))
    {
        return err;
    }

    if (!fsc_namespace_name_is_valid(name))
    {
        return runtime_error(RUNTIME_SUB_NAMESPACE, EINVAL);
    }

    had_session = runtime_fs_is_active();
    destroying_current =
            had_session && (strcmp(g_runtime.namespace_name, name) == 0);
    previous_name[0] = 0;
    if (had_session)
    {
        (void)snprintf(previous_name, sizeof(previous_name), "%s",
                       g_runtime.namespace_name);
    }

    if (!destroying_current)
    {
        err = runtime_fs_use(name);
        if (fs_failed(err))
        {
            return err;
        }
    }

    err = runtime_fs_remove_children("/");
    if (fs_failed(err))
    {
        if (had_session && !destroying_current)
        {
            (void)runtime_fs_use(previous_name);
        }
        return err;
    }

    (void)runtime_fs_leave();
    err = runtime_fs_destroy(name);
    if (fs_failed(err))
    {
        if (had_session && !destroying_current)
        {
            (void)runtime_fs_use(previous_name);
        }
        return err;
    }

    if (had_session && !destroying_current)
    {
        err = runtime_fs_use(previous_name);
    }

    return err;
}

fs_error_t runtime_fs_rename(const char *old_name, const char *new_name)
{
    fs_error_t err;
    bool renaming_current;

    err = runtime_require_initialized();
    if (fs_failed(err))
    {
        return err;
    }

    renaming_current = runtime_fs_is_active() &&
                       (strcmp(g_runtime.namespace_name, old_name) == 0);

    err = fsmgr_rename(old_name, new_name);
    if (fs_failed(err))
    {
        return err;
    }

    if (renaming_current)
    {
        if (snprintf(g_runtime.namespace_name, sizeof(g_runtime.namespace_name),
                     "%s", new_name) >= (int)sizeof(g_runtime.namespace_name))
        {
            (void)runtime_fs_leave();
            return runtime_error(RUNTIME_SUB_NAMESPACE, ENAMETOOLONG);
        }
    }

    return FS_OK;
}

fs_error_t runtime_fs_use(const char *name)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    err = runtime_require_initialized();
    if (fs_failed(err))
    {
        return err;
    }

    ns = fsmgr_lookup(name);
    if (ns == NULL)
    {
        return runtime_error(RUNTIME_SUB_NAMESPACE, ENOENT);
    }

    if (g_runtime.ns_active)
    {
        runtime_monitoring_session_end();
    }

    g_runtime.root_fuid = ns->root_fuid;
    g_runtime.cwd_fuid = ns->root_fuid;
    g_runtime.ns_active = true;

    if (snprintf(g_runtime.namespace_name, sizeof(g_runtime.namespace_name),
                 "%s", name) >= (int)sizeof(g_runtime.namespace_name))
    {
        (void)runtime_fs_leave();
        return runtime_error(RUNTIME_SUB_NAMESPACE, ENAMETOOLONG);
    }

    if (snprintf(g_runtime.cwd_path, sizeof(g_runtime.cwd_path), "/") >=
        (int)sizeof(g_runtime.cwd_path))
    {
        (void)runtime_fs_leave();
        return runtime_error(RUNTIME_SUB_PATH, ENAMETOOLONG);
    }

    runtime_monitoring_session_begin(ns->fsid, name);
    return FS_OK;
}

fs_error_t runtime_fs_enter(const char *name)
{
    return runtime_fs_use(name);
}

fs_error_t runtime_fs_leave(void)
{
    fs_error_t err;

    err = runtime_require_initialized();
    if (fs_failed(err))
    {
        return err;
    }

    runtime_monitoring_session_end();
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
    if (!runtime_fs_is_active())
    {
        return NULL;
    }

    return g_runtime.namespace_name;
}

fs_error_t runtime_fs_list(char names[][FSC_NAMESPACE_NAME_MAX], uint32_t cap,
                           uint32_t *actual_out)
{
    fs_error_t err;

    err = runtime_require_initialized();
    if (fs_failed(err))
    {
        return err;
    }

    return fsmgr_list(names, cap, actual_out);
}

fs_error_t runtime_get_ctx(namei_ctx_t *out_ctx)
{
    return runtime_make_ctx(out_ctx);
}

fs_error_t runtime_get_root(fuid_t *out_fuid)
{
    fs_error_t err;

    if (out_fuid == NULL)
    {
        return runtime_error(RUNTIME_SUB_CTX, EINVAL);
    }
    fuid_set_invalid(out_fuid);

    err = runtime_require_session();
    if (fs_failed(err))
    {
        return err;
    }

    *out_fuid = g_runtime.root_fuid;
    return FS_OK;
}

fs_error_t runtime_get_cwd(fuid_t *out_fuid)
{
    fs_error_t err;

    if (out_fuid == NULL)
    {
        return runtime_error(RUNTIME_SUB_CTX, EINVAL);
    }
    fuid_set_invalid(out_fuid);

    err = runtime_require_session();
    if (fs_failed(err))
    {
        return err;
    }

    *out_fuid = g_runtime.cwd_fuid;
    return FS_OK;
}

fs_error_t runtime_getcwd(char *buf, size_t size)
{
    fs_error_t err;

    if ((buf == NULL) || (size == 0U))
    {
        return runtime_error(RUNTIME_SUB_PATH, EINVAL);
    }
    buf[0] = 0;

    err = runtime_require_session();
    if (fs_failed(err))
    {
        return err;
    }

    if (snprintf(buf, size, "%s", g_runtime.cwd_path) >= (int)size)
    {
        return runtime_error(RUNTIME_SUB_PATH, ENAMETOOLONG);
    }

    return FS_OK;
}

fs_error_t runtime_chdir(const char *path)
{
    fs_error_t err;
    fuid_t target_fuid;

    err = runtime_lookup_fuid(path, FS_FLAG_DIRECTORY, &target_fuid);
    if (fs_failed(err))
    {
        return err;
    }

    err = runtime_update_cwd_path(path);
    if (fs_failed(err))
    {
        return err;
    }

    g_runtime.cwd_fuid = target_fuid;
    return FS_OK;
}
