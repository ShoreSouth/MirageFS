#include <string.h>

#include "fsc/namespace/namespace.h"
#include "fsc/fsc_error.h"

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

fs_error_t fsc_namespace_init(
                fsc_namespace_t *ns,
                fsc_fsid_t fsid,
                const char *name,
                const obj_handle_t *root)
{
    fs_error_t err;
    size_t len;

    FS_LOG_DUMP_INFO("enter: ns=%p, fsid=%llu, name=%p, root=%p",
                     (void *)ns,
                     (unsigned long long)fsid,
                     (const void *)name,
                     (const void *)root);

    if (ns == NULL) {
        err = fsc_error(FSC_SUB_NAMESPACE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: ns is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (!fsid_is_valid(fsid)) {
        err = fsc_error(FSC_SUB_NAMESPACE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid fsid, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (!fsc_namespace_name_is_valid(name)) {
        err = fsc_error(FSC_SUB_NAMESPACE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid name, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    memset(ns, 0, sizeof(*ns));

    ns->fsid = fsid;

    len = strlen(name);
    memcpy(ns->name, name, len);
    ns->name[len] = '\0';

    if (root != NULL) {
        memcpy(&ns->root, root, sizeof(ns->root));
    }

    fs_atomic32_init(&ns->refcnt, 0);
    ns->state = FSC_NAMESPACE_STATE_INIT;

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void fsc_namespace_deinit(
                fsc_namespace_t *ns)
{
    FS_LOG_DUMP_INFO("enter: ns=%p", (void *)ns);

    if (ns == NULL) {
        FS_LOG_DUMP_INFO("exit: ns is NULL");
        return;
    }

    memset(ns, 0, sizeof(*ns));

    FS_LOG_DUMP_INFO("exit: done");
}

/*
 * ============================================================
 * value ops
 * ============================================================
 */

bool fsc_namespace_is_valid(
                const fsc_namespace_t *ns)
{
    bool valid;

    FS_LOG_DUMP_INFO("enter: ns=%p", (const void *)ns);

    valid = (ns != NULL) &&
            fsid_is_valid(ns->fsid) &&
            fsc_namespace_name_is_valid(ns->name) &&
            (fsc_namespace_state(ns) != FSC_NAMESPACE_STATE_INVALID);

    FS_LOG_DUMP_INFO("exit: %s", valid ? "true" : "false");
    return valid;
}

bool fsc_namespace_name_is_valid(
                const char *name)
{
    size_t len;

    FS_LOG_DUMP_INFO("enter: name=%p", (const void *)name);

    if (name == NULL) {
        FS_LOG_DUMP_INFO("exit: false");
        return false;
    }

    len = strlen(name);
    if ((len == 0) || (len >= FSC_NAMESPACE_NAME_MAX)) {
        FS_LOG_DUMP_INFO("exit: false");
        return false;
    }

    FS_LOG_DUMP_INFO("exit: true");
    return true;
}

fsc_namespace_state_t fsc_namespace_state(
                const fsc_namespace_t *ns)
{
    if (ns == NULL) {
        return FSC_NAMESPACE_STATE_INVALID;
    }

    return (fsc_namespace_state_t)ns->state;
}

bool fsc_namespace_state_can_transit(
                fsc_namespace_state_t from,
                fsc_namespace_state_t to)
{
    bool allowed;

    FS_LOG_DUMP_INFO("enter: from=%u, to=%u",
                     (unsigned int)from, (unsigned int)to);

    allowed = false;

    switch (from) {
    case FSC_NAMESPACE_STATE_INIT:
        allowed = (to == FSC_NAMESPACE_STATE_ACTIVE);
        break;
    case FSC_NAMESPACE_STATE_ACTIVE:
        allowed = (to == FSC_NAMESPACE_STATE_DELETING);
        break;
    default:
        allowed = false;
        break;
    }

    FS_LOG_DUMP_INFO("exit: %s", allowed ? "true" : "false");
    return allowed;
}

fs_error_t fsc_namespace_change_state(
                fsc_namespace_t *ns,
                fsc_namespace_state_t state)
{
    fs_error_t err;
    fsc_namespace_state_t old_state;

    FS_LOG_DUMP_INFO("enter: ns=%p, to=%u",
                     (void *)ns, (unsigned int)state);

    if (ns == NULL) {
        err = fsc_error(FSC_SUB_NAMESPACE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: ns is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    old_state = fsc_namespace_state(ns);
    if (!fsc_namespace_state_can_transit(old_state, state)) {
        err = fsc_error(FSC_SUB_NAMESPACE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("state change failed: invalid transit, "
                          "from=%u, to=%u, err=%s (0x%x)",
                          (unsigned int)old_state,
                          (unsigned int)state,
                          fs_error_str(err), err);
        return err;
    }

    ns->state = (uint32_t)state;

    FS_LOG_DUMP_INFO("exit: ok, from=%u, to=%u",
                     (unsigned int)old_state, (unsigned int)state);
    return FS_OK;
}

/*
 * ============================================================
 * debug
 * ============================================================
 */

void fsc_namespace_dump(
                const fsc_namespace_t *ns)
{
    if (ns == NULL) {
        FS_LOG_DUMP_INFO("namespace: null");
        return;
    }

    FS_LOG_DUMP_INFO("namespace: fsid=%llu, name=%s, state=%u, refcnt=%d",
                     (unsigned long long)ns->fsid,
                     ns->name,
                     (unsigned int)fsc_namespace_state(ns),
                     fs_atomic32_load(&ns->refcnt));

}
