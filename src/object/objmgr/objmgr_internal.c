#include "object/objmgr/objmgr_internal.h"

#include <errno.h>

#include "object/obj_error.h"
#include "object/objpool/objpool.h"

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

    FS_LOG_DUMP_INFO("enter: key=%p", (const void *)key);

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
