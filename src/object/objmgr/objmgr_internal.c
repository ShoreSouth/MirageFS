#include "object/objmgr/objmgr_internal.h"
#include "object/fuid/fuid.h"
#include "object/obj_error.h"

/*
 * ============================================================
 * 内部辅助函数
 * ============================================================
 */

/*
 * 查找对象。
 *
 * 调用者必须已经持有 objmgr 全局锁。
 */
obj_meta_t *objmgr_lookup_locked(
                const obj_key_t *key)
{
    return objtable_lookup(
                &g_objmgr.table,
                key);
}

/*
 * 插入对象。
 *
 * 调用者必须已经持有 objmgr 全局锁。
 */
int32_t objmgr_insert_locked(
                obj_meta_t *meta)
{
    int32_t ret;

    ret = objtable_insert(
                    &g_objmgr.table,
                    meta);

    if (ret != FS_OK)
    {
        return ret;
    }

    fs_atomic32_inc(
            &g_objmgr.object_count);

    return FS_OK;
}

/*
 * 删除对象。
 *
 * 调用者必须已经持有 objmgr 全局锁。
 */
int32_t objmgr_remove_locked(
                const obj_key_t *key)
{
    int32_t ret;

    ret = objtable_remove(
                    &g_objmgr.table,
                    key);

    if (ret != FS_OK)
    {
        return ret;
    }

    fs_atomic32_dec(
            &g_objmgr.object_count);

    return FS_OK;
}

/*
 * 修改对象状态。
 *
 * 调用者必须已经持有 objmgr 全局锁。
 */
int32_t objmgr_change_state(
                obj_meta_t *meta,
                obj_state_t state)
{
    if (meta == NULL)
    {
        FS_LOG_DUMP_ERROR(
                "meta is NULL");

        return obj_error(OBJ_SUB_STATE, FS_ERRNO_EINVAL);
    }

    /*
     * TODO:
     * 后续可增加状态迁移合法性检查，例如：
     *
     * INIT      -> ACTIVE
     * ACTIVE    -> DELETING
     * DELETING  -> DELETED
     */

    meta->state = state;

    return FS_OK;
}