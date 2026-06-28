#include "object/objmgr/objmgr.h"
#include "object/objmgr/objmgr_internal.h"

#include <errno.h>

#include "object/obj_error.h"
#include "object/objpool/objpool.h"

/*
 * ============================================================
 * 全局管理器实例
 * ============================================================
 */

obj_manager_t g_objmgr;

/*
 * ============================================================
 * 初始化 / 销毁
 * ============================================================
 */

int32_t objmgr_init(void)
{
    int ret;

    FS_LOG_DUMP_INFO("enter");

    ret = objtable_init(&g_objmgr.table,
                        FS_HASH_DEFAULT_BUCKET_NR);
    if (ret != 0) {
        FS_LOG_DUMP_ERROR("objtable_init failed, ret=%d", (int)ret);
        FS_LOG_DUMP_INFO("exit: failed, ret=%d", (int)ret);
        return ret;
    }

    ret = fs_mutex_init(&g_objmgr.lock,
                        "objmgr",
                        0);
    if (ret != 0) {
        FS_LOG_DUMP_ERROR("fs_mutex_init failed, ret=%d", (int)ret);
        objtable_destroy(&g_objmgr.table);
        FS_LOG_DUMP_INFO("exit: failed, ret=%d", (int)ret);
        return ret;
    }

    fs_atomic32_init(&g_objmgr.object_count, 0);

    FS_LOG_DUMP_INFO("exit: ok");
    return 0;
}

void objmgr_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    fs_mutex_destroy(&g_objmgr.lock);

    objtable_destroy(&g_objmgr.table);

    fs_atomic32_store(&g_objmgr.object_count, 0);

    FS_LOG_DUMP_INFO("exit: done");
}

/*
 * ============================================================
 * 对象状态查询
 * ============================================================
 */

obj_state_t objmgr_state(
                const fuid_t *fuid)
{
    obj_key_t key;

    obj_runtime_t *rt;

    FS_LOG_DUMP_INFO(
            "enter: objectid=%llu gen=%u",
            (unsigned long long)fuid->objectid,
            (unsigned int)fuid->gen);

    objkey_from_fuid(
                &key,
                fuid);

    fs_mutex_lock(
            &g_objmgr.lock);

    rt = objmgr_lookup_locked(
                    &key);

    if (rt == NULL) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        FS_LOG_DUMP_INFO(
                "exit: INVALID (not found)");

        return OBJ_STATE_INVALID;
    }

    fs_mutex_unlock(
            &g_objmgr.lock);

    FS_LOG_DUMP_INFO(
            "exit: state=%u",
            (unsigned int)objruntime_state(rt));

    return objruntime_state(rt);
}

/*
 * ============================================================
 * 对象生命周期
 * ============================================================
 */

obj_meta_t *objmgr_create(
                const fuid_t *fuid,
                const obj_handle_t *handle)
{
    fs_error_t err;

    obj_runtime_t *rt;

    FS_LOG_DUMP_INFO(
            "enter: objectid=%llu gen=%u",
            (unsigned long long)fuid->objectid,
            (unsigned int)fuid->gen);

    rt = objpool_alloc();
    if (rt == NULL) {

        FS_LOG_DUMP_ERROR(
                "objpool_alloc failed");

        return NULL;
    }

    err = objmeta_init(
                &rt->meta,
                fuid,
                handle);

    if (err != FS_OK) {

        FS_LOG_DUMP_ERROR(
                "objmeta_init failed, err=%s (0x%x)",
                fs_error_str(err),
                err);

        objpool_free(rt);

        return NULL;
    }

    rt->state  = OBJ_STATE_INIT;
    rt->refcnt = 0;

    fs_mutex_lock(
            &g_objmgr.lock);

    err = objmgr_insert_locked(
                rt);

    if (err != FS_OK) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        FS_LOG_DUMP_ERROR(
                "insert_locked failed, err=%s (0x%x)",
                fs_error_str(err),
                err);

        objmeta_reset(&rt->meta);

        objpool_free(rt);

        return NULL;
    }

    err = objmgr_change_state(
                rt,
                OBJ_STATE_ACTIVE);

    if (err != FS_OK) {

        FS_LOG_DUMP_ERROR(
                "change_state failed, err=%s (0x%x)",
                fs_error_str(err),
                err);

        objmgr_reclaim_locked(rt);

        fs_mutex_unlock(
                &g_objmgr.lock);

        return NULL;
    }

    fs_mutex_unlock(
            &g_objmgr.lock);

    FS_LOG_DUMP_INFO(
            "exit: rt=%p, meta=%p",
            (void *)rt,
            (void *)&rt->meta);

    return &rt->meta;
}

int32_t objmgr_delete(
                const fuid_t *fuid)
{
    fs_error_t err;

    obj_key_t key;

    obj_runtime_t *rt;

    int32_t refcnt;

    FS_LOG_DUMP_INFO(
            "enter: objectid=%llu gen=%u",
            (unsigned long long)fuid->objectid,
            (unsigned int)fuid->gen);

    objkey_from_fuid(
                &key,
                fuid);

    fs_mutex_lock(
            &g_objmgr.lock);

    rt = objmgr_lookup_locked(
                    &key);

    if (rt == NULL) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        err = obj_error(
                    OBJ_SUB_DELETE,
                    ENOENT);

        FS_LOG_DUMP_ERROR(
                "lookup failed, err=%s (0x%x)",
                fs_error_str(err),
                err);

        return err;
    }

    if (objruntime_state(rt) != OBJ_STATE_ACTIVE) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        err = obj_error(
                    OBJ_SUB_DELETE,
                    EBUSY);

        FS_LOG_DUMP_ERROR(
                "state not ACTIVE: state=%u, err=%s (0x%x)",
                (unsigned int)objruntime_state(rt),
                fs_error_str(err),
                err);

        return err;
    }

    err = objmgr_change_state(
                rt,
                OBJ_STATE_DELETING);

    if (err != FS_OK) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        FS_LOG_DUMP_ERROR(
                "change_state failed, err=%s (0x%x)",
                fs_error_str(err),
                err);

        return err;
    }

    refcnt = fs_atomic32_load(
                    &rt->refcnt);

    if (refcnt == 0) {

        objmgr_reclaim_locked(rt);
    }

    fs_mutex_unlock(
            &g_objmgr.lock);

    FS_LOG_DUMP_INFO(
            "exit: ok");

    return FS_OK;
}

/*
 * ============================================================
 * 对象查找
 * ============================================================
 */

obj_meta_t *objmgr_lookup(
                const fuid_t *fuid)
{
    obj_key_t key;

    obj_runtime_t *rt;

    FS_LOG_DUMP_INFO(
            "enter: objectid=%llu gen=%u",
            (unsigned long long)fuid->objectid,
            (unsigned int)fuid->gen);

    objkey_from_fuid(
                &key,
                fuid);

    fs_mutex_lock(
            &g_objmgr.lock);

    rt = objmgr_lookup_locked(
                    &key);

    if (rt == NULL) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        FS_LOG_DUMP_INFO(
                "exit: NULL (not found)");

        return NULL;
    }

    if (objruntime_state(rt) != OBJ_STATE_ACTIVE) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        FS_LOG_DUMP_INFO(
                "exit: NULL (state=%u)",
                (unsigned int)objruntime_state(rt));

        return NULL;
    }

    fs_mutex_unlock(
            &g_objmgr.lock);

    FS_LOG_DUMP_INFO(
            "exit: rt=%p, meta=%p",
            (void *)rt,
            (void *)&rt->meta);

    return &rt->meta;
}

bool objmgr_exists(
                const fuid_t *fuid)
{
    return objmgr_lookup(fuid) != NULL;
}

/*
 * ============================================================
 * 统计
 * ============================================================
 */

uint32_t objmgr_count(void)
{
    return (uint32_t)fs_atomic32_load(
                    &g_objmgr.object_count);
}
