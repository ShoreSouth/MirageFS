#include <errno.h>

#include "object/objmgr/objmgr.h"
#include "object/objmgr/objmgr_internal.h"
#include "object/obj_error.h"

/*
 * ============================================================
 * 获取引用
 * ============================================================
 */

int32_t objmgr_get(
                const fuid_t *fuid)
{
    fs_error_t err;

    obj_key_t key;

    obj_meta_t *meta;

    FS_LOG_DUMP_INFO(
            "enter: objectid=%llu gen=%llu",
            (unsigned long long)fuid->objectid,
            (unsigned long long)fuid->gen);

    objkey_from_fuid(
                &key,
                fuid);

    fs_mutex_lock(
            &g_objmgr.lock);

    meta = objmgr_lookup_locked(
                    &key);

    if (meta == NULL) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        err = obj_error(
                    OBJ_SUB_GET,
                    ENOENT);

        FS_LOG_DUMP_ERROR(
                "lookup object failed: object not found, err=%s (0x%x)",
                fs_error_str(err),
                err);

        return err;
    }

    err = objmgr_ref_get_locked(
                    meta);

    fs_mutex_unlock(
            &g_objmgr.lock);

    if (err != FS_OK) {

        return err;
    }

    FS_LOG_DUMP_INFO(
            "exit: ok");

    return FS_OK;
}

/*
 * ============================================================
 * 释放引用
 * ============================================================
 */

int32_t objmgr_put(
                const fuid_t *fuid)
{
    fs_error_t err;

    obj_key_t key;

    obj_meta_t *meta;

    FS_LOG_DUMP_INFO(
            "enter: objectid=%llu gen=%llu",
            (unsigned long long)fuid->objectid,
            (unsigned long long)fuid->gen);

    objkey_from_fuid(
                &key,
                fuid);

    fs_mutex_lock(
            &g_objmgr.lock);

    meta = objmgr_lookup_locked(
                    &key);

    if (meta == NULL) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        err = obj_error(
                    OBJ_SUB_PUT,
                    ENOENT);

        FS_LOG_DUMP_ERROR(
                "lookup object failed: object not found, err=%s (0x%x)",
                fs_error_str(err),
                err);

        return err;
    }

    err = objmgr_ref_put_locked(
                    meta);

    fs_mutex_unlock(
            &g_objmgr.lock);

    if (err != FS_OK) {

        return err;
    }

    FS_LOG_DUMP_INFO(
            "exit: ok");

    return FS_OK;
}

/*
 * ============================================================
 * 获取引用计数
 * ============================================================
 */

int32_t objmgr_refcnt(
                const fuid_t *fuid)
{
    obj_key_t key;

    obj_meta_t *meta;

    int32_t refcnt;

    FS_LOG_DUMP_INFO(
            "enter: objectid=%llu gen=%llu",
            (unsigned long long)fuid->objectid,
            (unsigned long long)fuid->gen);

    objkey_from_fuid(
                &key,
                fuid);

    fs_mutex_lock(
            &g_objmgr.lock);

    meta = objmgr_lookup_locked(
                    &key);

    if (meta == NULL) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        FS_LOG_DUMP_INFO(
                "exit: not found");

        return 0;
    }

    refcnt = fs_atomic32_load(
                    &meta->refcnt);

    fs_mutex_unlock(
            &g_objmgr.lock);

    FS_LOG_DUMP_INFO(
            "exit: refcnt=%d",
            refcnt);

    return refcnt;
}

/*
 * ============================================================
 * 获取对象（推荐接口）
 * ============================================================
 */

obj_meta_t *objmgr_acquire(
                const fuid_t *fuid)
{
    obj_key_t key;

    obj_meta_t *meta;

    int32_t err;

    FS_LOG_DUMP_INFO(
            "enter: objectid=%llu gen=%llu",
            (unsigned long long)fuid->objectid,
            (unsigned long long)fuid->gen);

    objkey_from_fuid(
                &key,
                fuid);

    fs_mutex_lock(
            &g_objmgr.lock);

    meta = objmgr_lookup_locked(
                    &key);

    if (meta == NULL) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        FS_LOG_DUMP_INFO(
                "exit: object not found");

        return NULL;
    }

    err = objmgr_ref_get_locked(
                    meta);

    if (err != FS_OK) {

        fs_mutex_unlock(
                &g_objmgr.lock);

        FS_LOG_DUMP_INFO(
                "exit: acquire failed");

        return NULL;
    }

    fs_mutex_unlock(
            &g_objmgr.lock);

    FS_LOG_DUMP_INFO(
            "exit: meta=%p",
            (void *)meta);

    return meta;
}

/*
 * ============================================================
 * 释放对象（推荐接口）
 * ============================================================
 */

void objmgr_release(
                obj_meta_t *meta)
{
    if (meta == NULL) {

        FS_LOG_DUMP_INFO(
                "release ignored: meta is NULL");

        return;
    }

    FS_LOG_DUMP_INFO(
            "enter: meta=%p",
            (void *)meta);

    fs_mutex_lock(
            &g_objmgr.lock);

    (void)objmgr_ref_put_locked(
                    meta);

    fs_mutex_unlock(
            &g_objmgr.lock);

    FS_LOG_DUMP_INFO(
            "exit");
}
