#include "fsc/fsmgr/fsmgr.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "fsc/fsc_error.h"
#include "fsc/fsmgr/fsmgr_internal.h"
#include "fsc/fstable/fstable.h"
#include "fsc/nspool/nspool.h"
#include "fsc/sysroot/sysroot.h"
#include "lsa/include/lsa_api.h"
#include "object/objmgr/objmgr.h"

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
        (void)objmgr_delete(&ns->root_fuid);
        (void)fsid_free(ns->fsid);
        nspool_free(ns);
    }
}

static fs_error_t fsmgr_open_sysroot(
                int *fd_out)
{
    fs_error_t err;
    obj_handle_t root_handle;
    lsa_file_handle_t lsa_handle;

    if (fd_out == NULL) {
        return fsc_error(FSC_SUB_CREATE, FS_ERRNO_EINVAL);
    }

    err = fsc_sysroot_get_handle(&root_handle);
    if (fs_failed(err)) {
        return err;
    }

    err = objmeta_handle_to_lsa(&lsa_handle, &root_handle);
    if (fs_failed(err)) {
        return err;
    }

    return lsa_open_by_handle_id(root_handle.mount_id,
                                 &lsa_handle,
                                 O_PATH | O_DIRECTORY,
                                 fd_out);
}

static fs_error_t fsmgr_create_root_dir(
                const char *name,
                obj_handle_t *handle_out)
{
    fs_error_t err;
    lsa_file_handle_t lsa_handle;
    int32_t mount_id;
    int sysroot_fd;

    sysroot_fd = -1;

    err = fsmgr_open_sysroot(&sysroot_fd);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_mkdir(sysroot_fd,
                    name,
                    FS_FLAG_NONE,
                    FS_MODE_DIR_DEFAULT & FS_PERM_MASK);
    if (fs_failed(err)) {
        goto out;
    }

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    mount_id = 0;

    err = lsa_name_to_handle_at(sysroot_fd,
                                name,
                                &lsa_handle,
                                &mount_id,
                                0);
    if (fs_failed(err)) {
        (void)lsa_rmdir(sysroot_fd, name, FS_FLAG_NONE);
        goto out;
    }

    err = objmeta_handle_from_lsa(handle_out, &lsa_handle, mount_id);
    if (fs_failed(err)) {
        (void)lsa_rmdir(sysroot_fd, name, FS_FLAG_NONE);
        goto out;
    }

out:
    if (sysroot_fd >= 0) {
        (void)lsa_close(sysroot_fd);
    }

    return err;
}

static fs_error_t fsmgr_remove_root_dir(
                const char *name)
{
    fs_error_t err;
    int sysroot_fd;

    sysroot_fd = -1;

    err = fsmgr_open_sysroot(&sysroot_fd);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_rmdir(sysroot_fd, name, FS_FLAG_NONE);

out:
    if (sysroot_fd >= 0) {
        (void)lsa_close(sysroot_fd);
    }

    return err;
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
        goto out;
    }

    err = fs_mutex_init(&g_fsmgr.lock, "fsmgr", 0);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_mutex_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto err_table;
    }

    fs_atomic32_init(&g_fsmgr.namespace_count, 0);

    FS_LOG_DUMP_INFO("exit: ok");
    goto out;

err_table:
    fstable_deinit(&g_fsmgr.table, NULL);

out:
    return err;
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

fs_error_t fsmgr_create(
                const char *name,
                fuid_t *root_out)
{
    fs_error_t err;
    fsc_fsid_t fsid;
    fuid_t root_fuid;
    obj_key_t root_key;
    obj_handle_t root_handle;
    fsc_namespace_t *ns;
    bool dir_created;
    bool object_registered;

    fsid = FSID_INVALID;
    memset(&root_key, 0, sizeof(root_key));
    ns = NULL;
    dir_created = false;
    object_registered = false;

    if (root_out != NULL) {
        fuid_set_invalid(root_out);
    }

    if ((root_out == NULL) || !fsc_namespace_name_is_valid(name)) {
        err = fsc_error(FSC_SUB_CREATE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid create args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    fs_mutex_lock(&g_fsmgr.lock);

    if (fstable_exists_name(&g_fsmgr.table, name)) {
        err = fsc_error(FSC_SUB_CREATE, FS_ERRNO_EEXIST);
        FS_LOG_DUMP_ERROR("create failed: namespace exists, name=%s, "
                          "err=%s (0x%x)", name, fs_error_str(err), err);
        goto unlock;
    }

    err = fsid_alloc(&fsid);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fsid_alloc failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto unlock;
    }

    err = objmgr_alloc_key(&root_key);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("objmgr_alloc_key failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto unlock;
    }

    root_fuid = fuid_make(fsid,
                          root_key.objectid,
                          root_key.gen,
                          FUID_TYPE_DIR);

    err = fsmgr_create_root_dir(name, &root_handle);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("create root dir failed: name=%s, err=%s (0x%x)",
                          name, fs_error_str(err), err);
        goto unlock;
    }
    dir_created = true;

    ns = nspool_alloc();
    if (ns == NULL) {
        err = fsc_error(FSC_SUB_CREATE, FS_ERRNO_ENOMEM);
        FS_LOG_DUMP_ERROR("nspool_alloc failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto unlock;
    }

    if (objmgr_create(&root_fuid, &root_handle) == NULL) {
        err = fsc_error(FSC_SUB_CREATE, FS_ERRNO_EIO);
        FS_LOG_DUMP_ERROR("objmgr_create root failed: name=%s, "
                          "err=%s (0x%x)",
                          name, fs_error_str(err), err);
        goto unlock;
    }
    object_registered = true;

    err = fsc_namespace_init(ns,
                             fsid,
                             name,
                             &root_fuid,
                             &root_handle);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fsc_namespace_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto unlock;
    }

    err = fstable_insert(&g_fsmgr.table, ns);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fstable_insert failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto unlock;
    }

    err = fsc_namespace_change_state(ns, FSC_NAMESPACE_STATE_ACTIVE);
    if (fs_failed(err)) {
        (void)fstable_remove(&g_fsmgr.table, fsid, NULL);
        FS_LOG_DUMP_ERROR("fsc_namespace_change_state failed, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto unlock;
    }

    fs_atomic32_inc(&g_fsmgr.namespace_count);
    *root_out = root_fuid;
    ns = NULL;
    fsid = FSID_INVALID;
    dir_created = false;

unlock:
    if (fs_failed(err)) {
        if (ns != NULL) {
            nspool_free(ns);
        }

        if (object_registered) {
            (void)objmgr_delete(&root_fuid);
        } else if (objkey_is_valid(&root_key)) {
            (void)objmgr_free_key(&root_key);
        }

        if (dir_created) {
            (void)fsmgr_remove_root_dir(name);
        }

        if (fsid != FSID_INVALID) {
            (void)fsid_free(fsid);
        }
    }

    fs_mutex_unlock(&g_fsmgr.lock);

out:
    FS_LOG_DUMP_INFO("exit: err=%s (0x%x)", fs_error_str(err), err);
    return err;
}

fs_error_t fsmgr_destroy(
                fsc_fsid_t fsid)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    fs_mutex_lock(&g_fsmgr.lock);

    ns = fstable_lookup_fsid(&g_fsmgr.table, fsid);
    if (ns == NULL) {
        err = fsc_error(FSC_SUB_DESTROY, FS_ERRNO_ENOENT);
        FS_LOG_DUMP_ERROR("destroy failed: namespace not found, fsid=%llu, "
                          "err=%s (0x%x)",
                          (unsigned long long)fsid,
                          fs_error_str(err), err);
        goto unlock;
    }

    if (fs_atomic32_load(&ns->refcnt) != 0) {
        err = fsc_error(FSC_SUB_DESTROY, FS_ERRNO_EBUSY);
        FS_LOG_DUMP_ERROR("destroy failed: namespace busy, fsid=%llu, "
                          "err=%s (0x%x)",
                          (unsigned long long)fsid,
                          fs_error_str(err), err);
        goto unlock;
    }

    err = fsmgr_remove_root_dir(ns->name);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("remove root dir failed: name=%s, err=%s (0x%x)",
                          ns->name, fs_error_str(err), err);
        goto unlock;
    }

    err = objmgr_delete(&ns->root_fuid);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("objmgr_delete root failed: fsid=%llu, "
                          "err=%s (0x%x)",
                          (unsigned long long)fsid,
                          fs_error_str(err), err);
        goto unlock;
    }

    err = fsc_namespace_change_state(ns, FSC_NAMESPACE_STATE_DELETING);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fsc_namespace_change_state failed, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto unlock;
    }

    err = fstable_remove(&g_fsmgr.table, fsid, &ns);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fstable_remove failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto unlock;
    }

    fs_atomic32_dec(&g_fsmgr.namespace_count);
    (void)fsid_free(fsid);
    nspool_free(ns);

unlock:
    fs_mutex_unlock(&g_fsmgr.lock);

    FS_LOG_DUMP_INFO("exit: err=%s (0x%x)", fs_error_str(err), err);
    return err;
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

fs_error_t fsmgr_get_root_fuid(
                fsc_fsid_t fsid,
                fuid_t *root_out)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    if (root_out == NULL) {
        return fsc_error(FSC_SUB_LOOKUP, FS_ERRNO_EINVAL);
    }

    fuid_set_invalid(root_out);
    err = FS_OK;

    fs_mutex_lock(&g_fsmgr.lock);

    ns = fstable_lookup_fsid(&g_fsmgr.table, fsid);
    if ((ns == NULL) ||
        (fsc_namespace_state(ns) != FSC_NAMESPACE_STATE_ACTIVE)) {
        err = fsc_error(FSC_SUB_LOOKUP, FS_ERRNO_ENOENT);
        goto unlock;
    }

    *root_out = ns->root_fuid;

unlock:
    fs_mutex_unlock(&g_fsmgr.lock);
    return err;
}

fs_error_t fsmgr_get_root_handle(
                fsc_fsid_t fsid,
                obj_handle_t *handle_out)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    if (handle_out == NULL) {
        return fsc_error(FSC_SUB_LOOKUP, FS_ERRNO_EINVAL);
    }

    memset(handle_out, 0, sizeof(*handle_out));
    err = FS_OK;

    fs_mutex_lock(&g_fsmgr.lock);

    ns = fstable_lookup_fsid(&g_fsmgr.table, fsid);
    if ((ns == NULL) ||
        (fsc_namespace_state(ns) != FSC_NAMESPACE_STATE_ACTIVE)) {
        err = fsc_error(FSC_SUB_LOOKUP, FS_ERRNO_ENOENT);
        goto unlock;
    }

    *handle_out = ns->root_handle;

unlock:
    fs_mutex_unlock(&g_fsmgr.lock);
    return err;
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
