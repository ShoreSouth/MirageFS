#include "fsc/fsmgr/fsmgr.h"
#include "fsc/fsc_error.h"
#include "fsc/fsmgr/fsmgr_internal.h"
#include "fsc/fstable/fstable.h"
#include "fsc/nspool/nspool.h"

/*
 * ============================================================
 * global manager
 * ============================================================
 */

fsc_manager_t g_fsmgr;

/*
 * ============================================================
 * private helper
 * ============================================================
 */

static void fsmgr_reclaim_namespace(
                fsc_namespace_t *ns)
{
    if (ns != NULL) {
        (void)fsid_free(ns->fsid);
        nspool_free(ns);
    }
}

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

fs_error_t fsmgr_init(void)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter");

    err = fstable_init(&g_fsmgr.table, FS_HASH_DEFAULT_BUCKET_NR);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fstable_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = fs_mutex_init(&g_fsmgr.lock, "fsmgr", 0);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_mutex_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        fstable_deinit(&g_fsmgr.table, NULL);
        return err;
    }

    fs_atomic32_init(&g_fsmgr.namespace_count, 0);

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void fsmgr_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    fs_mutex_destroy(&g_fsmgr.lock);
    fstable_deinit(&g_fsmgr.table, fsmgr_reclaim_namespace);
    fs_atomic32_store(&g_fsmgr.namespace_count, 0);

    FS_LOG_DUMP_INFO("exit: done");
}

/*
 * ============================================================
 * namespace lifecycle
 * ============================================================
 */

fsc_namespace_t *fsmgr_create(
                const char *name,
                const obj_handle_t *root)
{
    fs_error_t err;
    fsc_fsid_t fsid;
    fsc_namespace_t *ns;

    FS_LOG_DUMP_INFO("enter: name=%p, root=%p",
                     (const void *)name, (const void *)root);

    if (!fsc_namespace_name_is_valid(name)) {
        err = fsc_error(FSC_SUB_CREATE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid name, err=%s (0x%x)",
                          fs_error_str(err), err);
        return NULL;
    }

    fs_mutex_lock(&g_fsmgr.lock);

    if (fstable_exists_name(&g_fsmgr.table, name)) {
        fs_mutex_unlock(&g_fsmgr.lock);
        err = fsc_error(FSC_SUB_CREATE, FS_ERRNO_EEXIST);
        FS_LOG_DUMP_ERROR("create failed: namespace already exists, "
                          "name=%s, err=%s (0x%x)",
                          name, fs_error_str(err), err);
        return NULL;
    }

    err = fsid_alloc(&fsid);
    if (fs_failed(err)) {
        fs_mutex_unlock(&g_fsmgr.lock);
        FS_LOG_DUMP_ERROR("fsid_alloc failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return NULL;
    }

    ns = nspool_alloc();
    if (ns == NULL) {
        fs_mutex_unlock(&g_fsmgr.lock);
        (void)fsid_free(fsid);
        FS_LOG_DUMP_ERROR("nspool_alloc failed");
        return NULL;
    }

    err = fsc_namespace_init(ns, fsid, name, root);
    if (fs_failed(err)) {
        fs_mutex_unlock(&g_fsmgr.lock);
        nspool_free(ns);
        (void)fsid_free(fsid);
        FS_LOG_DUMP_ERROR("fsc_namespace_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return NULL;
    }

    err = fstable_insert(&g_fsmgr.table, ns);
    if (fs_failed(err)) {
        fs_mutex_unlock(&g_fsmgr.lock);
        nspool_free(ns);
        (void)fsid_free(fsid);
        FS_LOG_DUMP_ERROR("fstable_insert failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return NULL;
    }

    err = fsc_namespace_change_state(ns, FSC_NAMESPACE_STATE_ACTIVE);
    if (fs_failed(err)) {
        (void)fstable_remove(&g_fsmgr.table, fsid, NULL);
        fs_mutex_unlock(&g_fsmgr.lock);
        nspool_free(ns);
        (void)fsid_free(fsid);
        FS_LOG_DUMP_ERROR("fsc_namespace_change_state failed, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return NULL;
    }

    fs_atomic32_inc(&g_fsmgr.namespace_count);

    fs_mutex_unlock(&g_fsmgr.lock);

    FS_LOG_DUMP_INFO("exit: ns=%p, fsid=%llu",
                     (void *)ns, (unsigned long long)fsid);
    return ns;
}

fs_error_t fsmgr_destroy(
                fsc_fsid_t fsid)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    FS_LOG_DUMP_INFO("enter: fsid=%llu",
                     (unsigned long long)fsid);

    fs_mutex_lock(&g_fsmgr.lock);

    ns = fstable_lookup_fsid(&g_fsmgr.table, fsid);
    if (ns == NULL) {
        fs_mutex_unlock(&g_fsmgr.lock);
        err = fsc_error(FSC_SUB_DESTROY, FS_ERRNO_ENOENT);
        FS_LOG_DUMP_ERROR("destroy failed: namespace not found, "
                          "fsid=%llu, err=%s (0x%x)",
                          (unsigned long long)fsid,
                          fs_error_str(err), err);
        return err;
    }

    if (fs_atomic32_load(&ns->refcnt) != 0) {
        fs_mutex_unlock(&g_fsmgr.lock);
        err = fsc_error(FSC_SUB_DESTROY, FS_ERRNO_EBUSY);
        FS_LOG_DUMP_ERROR("destroy failed: namespace is busy, "
                          "fsid=%llu, err=%s (0x%x)",
                          (unsigned long long)fsid,
                          fs_error_str(err), err);
        return err;
    }

    err = fsc_namespace_change_state(ns, FSC_NAMESPACE_STATE_DELETING);
    if (fs_failed(err)) {
        fs_mutex_unlock(&g_fsmgr.lock);
        FS_LOG_DUMP_ERROR("fsc_namespace_change_state failed, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = fstable_remove(&g_fsmgr.table, fsid, &ns);
    if (fs_failed(err)) {
        fs_mutex_unlock(&g_fsmgr.lock);
        FS_LOG_DUMP_ERROR("fstable_remove failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    fs_atomic32_dec(&g_fsmgr.namespace_count);

    fs_mutex_unlock(&g_fsmgr.lock);

    (void)fsid_free(fsid);
    nspool_free(ns);

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/*
 * ============================================================
 * lookup
 * ============================================================
 */

fsc_namespace_t *fsmgr_lookup(
                const char *name)
{
    fsc_namespace_t *ns;

    FS_LOG_DUMP_INFO("enter: name=%p", (const void *)name);

    fs_mutex_lock(&g_fsmgr.lock);

    ns = fstable_lookup_name(&g_fsmgr.table, name);
    if ((ns != NULL) &&
        (fsc_namespace_state(ns) != FSC_NAMESPACE_STATE_ACTIVE)) {
        ns = NULL;
    }

    fs_mutex_unlock(&g_fsmgr.lock);

    FS_LOG_DUMP_INFO("exit: ns=%p", (void *)ns);
    return ns;
}

fsc_namespace_t *fsmgr_lookup_fsid(
                fsc_fsid_t fsid)
{
    fsc_namespace_t *ns;

    FS_LOG_DUMP_INFO("enter: fsid=%llu",
                     (unsigned long long)fsid);

    fs_mutex_lock(&g_fsmgr.lock);

    ns = fstable_lookup_fsid(&g_fsmgr.table, fsid);
    if ((ns != NULL) &&
        (fsc_namespace_state(ns) != FSC_NAMESPACE_STATE_ACTIVE)) {
        ns = NULL;
    }

    fs_mutex_unlock(&g_fsmgr.lock);

    FS_LOG_DUMP_INFO("exit: ns=%p", (void *)ns);
    return ns;
}

bool fsmgr_exists(
                const char *name)
{
    bool exists;

    exists = (fsmgr_lookup(name) != NULL);

    FS_LOG_DUMP_INFO("exit: %s", exists ? "true" : "false");
    return exists;
}

const obj_handle_t *fsmgr_get_root(
                fsc_fsid_t fsid)
{
    fsc_namespace_t *ns;

    FS_LOG_DUMP_INFO("enter: fsid=%llu",
                     (unsigned long long)fsid);

    ns = fsmgr_lookup_fsid(fsid);
    if (ns == NULL) {
        FS_LOG_DUMP_INFO("exit: not found");
        return NULL;
    }

    FS_LOG_DUMP_INFO("exit: root=%p", (const void *)&ns->root);
    return &ns->root;
}

/*
 * ============================================================
 * stats
 * ============================================================
 */

uint32_t fsmgr_count(void)
{
    return (uint32_t)fs_atomic32_load(&g_fsmgr.namespace_count);
}
