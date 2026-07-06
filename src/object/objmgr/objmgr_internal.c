#include "object/objmgr/objmgr_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "object/obj_error.h"
#include "object/objpool/objpool.h"


/*
 * ============================================================
 * Handle Index Helper
 * ============================================================
 */

typedef struct objmgr_handle_entry {

    obj_runtime_t *runtime;
    fs_list_head_t node;

} objmgr_handle_entry_t;

static uint64_t objmgr_handle_hash(
                const obj_handle_t *handle)
{
    uint64_t hash;
    uint16_t i;

    hash = 1469598103934665603ULL;
    hash ^= (uint64_t)(uint32_t)handle->mount_id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)handle->type;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)handle->len;
    hash *= 1099511628211ULL;

    for (i = 0; i < handle->len; i++) {
        hash ^= (uint64_t)handle->data[i];
        hash *= 1099511628211ULL;
    }

    return hash;
}

static bool objmgr_handle_equal(
                const obj_handle_t *lhs,
                const obj_handle_t *rhs)
{
    if ((lhs == NULL) || (rhs == NULL)) {
        return false;
    }

    if ((lhs->mount_id != rhs->mount_id) ||
        (lhs->type != rhs->type) ||
        (lhs->len != rhs->len)) {
        return false;
    }

    return memcmp(lhs->data, rhs->data, lhs->len) == 0;
}

static uint64_t objmgr_handle_node_hash(
                const fs_list_head_t *node)
{
    const objmgr_handle_entry_t *entry;

    entry = FS_CONTAINER_OF(node, objmgr_handle_entry_t, node);
    return objmgr_handle_hash(&entry->runtime->meta.handle);
}

static uint64_t objmgr_handle_key_hash(
                const void *key)
{
    return objmgr_handle_hash((const obj_handle_t *)key);
}

static bool objmgr_handle_match(
                const fs_list_head_t *node,
                const void *key)
{
    const objmgr_handle_entry_t *entry;

    entry = FS_CONTAINER_OF(node, objmgr_handle_entry_t, node);
    return objmgr_handle_equal(&entry->runtime->meta.handle,
                               (const obj_handle_t *)key);
}

static objmgr_handle_entry_t *objmgr_find_handle_entry(
                fs_hash_t *table,
                const obj_handle_t *handle)
{
    fs_list_head_t *node;

    node = fs_hash_lookup(table, handle);
    if (node == NULL) {
        return NULL;
    }

    return FS_CONTAINER_OF(node, objmgr_handle_entry_t, node);
}

fs_error_t objmgr_handle_index_init(
                fs_hash_t *table,
                uint32_t bucket_nr)
{
    return fs_hash_init(table,
                        bucket_nr,
                        objmgr_handle_node_hash,
                        objmgr_handle_key_hash,
                        objmgr_handle_match);
}

void objmgr_handle_index_deinit(
                fs_hash_t *table)
{
    uint32_t i;
    fs_list_head_t *pos;
    fs_list_head_t *next;
    objmgr_handle_entry_t *entry;

    if (table == NULL) {
        return;
    }

    for (i = 0; i < table->bucket_nr; i++) {
        FS_LIST_FOR_EACH_SAFE(pos, next, &table->buckets[i]) {
            entry = FS_CONTAINER_OF(pos, objmgr_handle_entry_t, node);
            fs_hash_remove(table, &entry->node);
            free(entry);
        }
    }

    fs_hash_destroy(table);
}

obj_runtime_t *objmgr_lookup_handle_locked(
                const obj_handle_t *handle)
{
    objmgr_handle_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: handle=%p", (const void *)handle);

    entry = objmgr_find_handle_entry(&g_objmgr.handle_table, handle);
    if (entry == NULL) {
        FS_LOG_DUMP_INFO("exit: not found");
        return NULL;
    }

    FS_LOG_DUMP_INFO("exit: rt=%p", (void *)entry->runtime);
    return entry->runtime;
}

fs_error_t objmgr_insert_handle_locked(
                obj_runtime_t *rt)
{
    fs_error_t err;
    objmgr_handle_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: rt=%p", (void *)rt);

    if (rt == NULL) {
        err = obj_error(OBJ_SUB_HANDLE, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: rt is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (objmgr_lookup_handle_locked(&rt->meta.handle) != NULL) {
        err = obj_error(OBJ_SUB_HANDLE, EEXIST);
        FS_LOG_DUMP_ERROR("handle insert failed: handle exists, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        err = obj_error(OBJ_SUB_HANDLE, ENOMEM);
        FS_LOG_DUMP_ERROR("calloc failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    entry->runtime = rt;
    fs_list_init(&entry->node);

    err = fs_hash_insert(&g_objmgr.handle_table, &entry->node);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_hash_insert failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        free(entry);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void objmgr_remove_handle_locked(
                obj_runtime_t *rt)
{
    objmgr_handle_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: rt=%p", (void *)rt);

    if (rt == NULL) {
        FS_LOG_DUMP_INFO("exit: rt is NULL");
        return;
    }

    entry = objmgr_find_handle_entry(&g_objmgr.handle_table,
                                     &rt->meta.handle);
    if (entry == NULL) {
        FS_LOG_DUMP_INFO("exit: not found");
        return;
    }

    fs_hash_remove(&g_objmgr.handle_table, &entry->node);
    free(entry);

    FS_LOG_DUMP_INFO("exit: done");
}

/*
 * ============================================================
 * ObjTable Helper
 * ============================================================
 */

obj_runtime_t *objmgr_lookup_locked(
                const obj_key_t *key)
{
    obj_runtime_t *rt;

    FS_LOG_DUMP_INFO("enter: key=%p", (const void *)key);

    rt = objtable_lookup(
                &g_objmgr.table,
                key);

    FS_LOG_DUMP_INFO("exit: rt=%p", (void *)rt);
    return rt;
}

fs_error_t objmgr_insert_locked(
                obj_runtime_t *rt)
{
    fs_error_t ret;

    FS_LOG_DUMP_INFO("enter: rt=%p", (void *)rt);

    ret = objtable_insert(
                &g_objmgr.table,
                rt);

    if (fs_failed(ret)) {
        FS_LOG_DUMP_INFO("exit: failed, err=%s (0x%x)",
                         fs_error_str(ret), ret);
        return ret;
    }

    ret = objmgr_insert_handle_locked(rt);
    if (fs_failed(ret)) {
        (void)objtable_remove(&g_objmgr.table, &rt->meta.key);
        FS_LOG_DUMP_INFO("exit: failed, err=%s (0x%x)",
                         fs_error_str(ret), ret);
        return ret;
    }

    fs_atomic32_inc(
                &g_objmgr.object_count);

    FS_LOG_DUMP_INFO("exit: ok, count=%d",
                     (int)fs_atomic32_load(&g_objmgr.object_count));
    return FS_OK;
}

fs_error_t objmgr_remove_locked(
                const obj_key_t *key)
{
    fs_error_t ret;
    obj_runtime_t *rt;

    FS_LOG_DUMP_INFO("enter: key=%p", (const void *)key);

    rt = objmgr_lookup_locked(key);
    if (rt != NULL) {
        objmgr_remove_handle_locked(rt);
    }

    ret = objtable_remove(
                &g_objmgr.table,
                key);

    if (fs_failed(ret)) {
        FS_LOG_DUMP_INFO("exit: failed, err=%s (0x%x)",
                         fs_error_str(ret), ret);
        return ret;
    }

    fs_atomic32_dec(
                &g_objmgr.object_count);

    FS_LOG_DUMP_INFO("exit: ok, count=%d",
                     (int)fs_atomic32_load(&g_objmgr.object_count));
    return FS_OK;
}

/*
 * ============================================================
 * 生命周期 Helper
 * ============================================================
 */

bool objmgr_state_can_transit(
                obj_state_t from,
                obj_state_t to)
{
    bool can;

    FS_LOG_DUMP_INFO("enter: from=%u, to=%u",
                     (unsigned int)from, (unsigned int)to);

    switch (from)
    {
    case OBJ_STATE_INVALID:
        can = false;
        break;

    case OBJ_STATE_INIT:
        can = (to == OBJ_STATE_ACTIVE);
        break;

    case OBJ_STATE_ACTIVE:
        can = (to == OBJ_STATE_DELETING);
        break;

    case OBJ_STATE_DELETING:
    default:
        can = false;
        break;
    }

    FS_LOG_DUMP_INFO("exit: %s",
                     can ? "true" : "false");
    return can;
}

fs_error_t objmgr_change_state(
                obj_runtime_t *rt,
                obj_state_t state)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter: rt=%p, state=%u",
                     (void *)rt, (unsigned int)state);

    if (rt == NULL) {

        err = obj_error(
                    OBJ_SUB_STATE,
                    EINVAL);

        FS_LOG_DUMP_ERROR(
                "param check failed: rt is NULL, err=%s (0x%x)",
                fs_error_str(err), err);

        return err;
    }

    if (!objmgr_state_can_transit(
                    objruntime_state(rt),
                    state)) {

        err = obj_error(
                    OBJ_SUB_STATE,
                    EPERM);

        FS_LOG_DUMP_ERROR(
                "state transition failed: from=%u to=%u, err=%s (0x%x)",
                (unsigned int)objruntime_state(rt),
                (unsigned int)state,
                fs_error_str(err), err);

        return err;
    }

    rt->state = state;

    FS_LOG_DUMP_INFO("exit: ok, new_state=%u",
                     (unsigned int)state);
    return FS_OK;
}

/*
 * ============================================================
 * 引用计数 Helper
 * ============================================================
 */

fs_error_t objmgr_ref_get_locked(
                obj_runtime_t *rt)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter: rt=%p", (void *)rt);

    if (rt == NULL) {

        err = obj_error(
                    OBJ_SUB_GET,
                    EINVAL);

        FS_LOG_DUMP_ERROR(
                "param check failed: rt is NULL, err=%s (0x%x)",
                fs_error_str(err), err);

        return err;
    }

    if (objruntime_state(rt) != OBJ_STATE_ACTIVE) {

        err = obj_error(
                    OBJ_SUB_GET,
                    EBUSY);

        FS_LOG_DUMP_ERROR(
                "ref get failed: state=%u not ACTIVE, err=%s (0x%x)",
                (unsigned int)objruntime_state(rt),
                fs_error_str(err), err);

        return err;
    }

    fs_atomic32_inc(
                &rt->refcnt);

    FS_LOG_DUMP_INFO("exit: ok, refcnt=%d",
                     (int)fs_atomic32_load(&rt->refcnt));
    return FS_OK;
}

/*
 * 释放对象引用。
 *
 * 调用者必须已经持有 objmgr 全局锁。
 */
fs_error_t objmgr_ref_put_locked(
                obj_runtime_t *rt)
{
    fs_error_t err;

    int32_t refcnt;

    FS_LOG_DUMP_INFO(
            "enter: rt=%p",
            (void *)rt);

    if (rt == NULL) {

        err = obj_error(
                    OBJ_SUB_PUT,
                    EINVAL);

        FS_LOG_DUMP_ERROR(
                "param check failed: rt is NULL, err=%s (0x%x)",
                fs_error_str(err),
                err);

        return err;
    }

    refcnt = fs_atomic32_load(
                    &rt->refcnt);

    if (refcnt <= 0) {

        err = obj_error(
                    OBJ_SUB_PUT,
                    EINVAL);

        FS_LOG_DUMP_ERROR(
                "invalid refcnt=%d, err=%s (0x%x)",
                refcnt,
                fs_error_str(err),
                err);

        return err;
    }

    refcnt = fs_atomic32_dec(
                    &rt->refcnt);

    FS_LOG_DUMP_INFO(
            "ref released: refcnt=%d",
            refcnt);

    /*
     * 对象仍被引用。
     */
    if (refcnt > 0) {

        FS_LOG_DUMP_INFO(
                "exit: object still referenced");

        return FS_OK;
    }

    /*
     * refcnt == 0
     *
     * 仅当对象已进入 DELETING 状态时，
     * 才执行最终回收。
     */
    if (objruntime_state(rt) != OBJ_STATE_DELETING) {

        FS_LOG_DUMP_INFO(
                "exit: refcnt=0 but state=%u",
                (unsigned int)objruntime_state(rt));

        return FS_OK;
    }

    FS_LOG_DUMP_INFO(
            "final reclaim begin");

    objmgr_reclaim_locked(rt);

    FS_LOG_DUMP_INFO(
            "exit: ok");

    return FS_OK;
}

/*
 * ============================================================
 * 回收 Helper
 * ============================================================
 */

void objmgr_reclaim_locked(
                obj_runtime_t *rt)
{
    uint64_t objectid;
    uint32_t gen;

    FS_LOG_DUMP_INFO(
            "enter: rt=%p",
            (void *)rt);

    objectid = rt->meta.key.objectid;
    gen      = rt->meta.key.gen;

    (void)objmgr_remove_locked(
                &rt->meta.key);

    objmeta_reset(&rt->meta);

    objpool_free(rt);

    FS_LOG_DUMP_INFO(
            "object reclaimed: objectid=%llu gen=%u",
            (unsigned long long)objectid,
            (unsigned int)gen);

    FS_LOG_DUMP_INFO("exit");
}
