#include "object/objmgr/objmgr.h"
#include "object/objmgr/objmgr_internal.h"

#include <errno.h>
#include <string.h>

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
 * objectid allocator
 * ============================================================
 */

static void objmgr_key_allocator_reset(void)
{
    uint64_t i;

    memset(g_objmgr.key_free_stack, 0, sizeof(g_objmgr.key_free_stack));
    memset(g_objmgr.key_allocated, 0, sizeof(g_objmgr.key_allocated));

    for (i = 1U; i <= OBJMGR_KEY_MAX_SLOTS; i++)
    {
        g_objmgr.key_generation[i] = OBJMGR_KEY_GENERATION_INIT;
        g_objmgr.key_free_stack[i - 1U] =
                (uint64_t)(OBJMGR_KEY_MAX_SLOTS - i + 1U);
    }

    g_objmgr.key_free_count = OBJMGR_KEY_MAX_SLOTS;
}

static bool objmgr_key_slot_is_valid(ObjectId_t objectid)
{
    return (objectid != 0U) && (objectid <= (ObjectId_t)OBJMGR_KEY_MAX_SLOTS);
}

fs_error_t objmgr_alloc_key(obj_key_t *out_key)
{
    fs_error_t err;
    ObjectId_t objectid;
    GenId_t gen;

    FS_LOG_DUMP_INFO("enter: out_key=%p", (void *)out_key);

    if (out_key == NULL)
    {
        err = obj_error(OBJ_SUB_ALLOC, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: out_key is NULL, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    memset(out_key, 0, sizeof(*out_key));

    fs_mutex_lock(&g_objmgr.key_lock);

    if (g_objmgr.key_free_count == 0U)
    {
        fs_mutex_unlock(&g_objmgr.key_lock);
        err = obj_error(OBJ_SUB_ALLOC, ENOSPC);
        FS_LOG_DUMP_ERROR("allocate key failed: no free slot, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    g_objmgr.key_free_count--;
    objectid = (ObjectId_t)g_objmgr.key_free_stack[g_objmgr.key_free_count];
    gen = (GenId_t)g_objmgr.key_generation[objectid];
    g_objmgr.key_allocated[objectid] = 1U;

    fs_mutex_unlock(&g_objmgr.key_lock);

    *out_key = objkey_make(objectid, gen);

    FS_LOG_DUMP_INFO("exit: objectid=%llu gen=%u",
                     (unsigned long long)out_key->objectid,
                     (unsigned int)out_key->gen);
    return FS_OK;
}

fs_error_t objmgr_free_key(const obj_key_t *key)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter: key=%p", (const void *)key);

    if ((key == NULL) || !objkey_is_valid(key) ||
        !objmgr_key_slot_is_valid(key->objectid))
    {
        err = obj_error(OBJ_SUB_ALLOC, EINVAL);
        FS_LOG_DUMP_ERROR("free key failed: invalid key, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    fs_mutex_lock(&g_objmgr.key_lock);

    if ((g_objmgr.key_allocated[key->objectid] == 0U) ||
        (g_objmgr.key_generation[key->objectid] != key->gen))
    {
        fs_mutex_unlock(&g_objmgr.key_lock);
        err = obj_error(OBJ_SUB_ALLOC, EINVAL);
        FS_LOG_DUMP_ERROR("free key failed: stale or unallocated key, "
                          "objectid=%llu gen=%u, err=%s (0x%x)",
                          (unsigned long long)key->objectid,
                          (unsigned int)key->gen, fs_error_str(err), err);
        return err;
    }

    objmgr_free_key_locked(key);

    fs_mutex_unlock(&g_objmgr.key_lock);

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/*
 * ============================================================
 * 初始化 / 销毁
 * ============================================================
 */

fs_error_t objmgr_init(void)
{
    fs_error_t ret;

    FS_LOG_DUMP_INFO("enter");

    ret = objtable_init(&g_objmgr.table, FS_HASH_DEFAULT_BUCKET_NR);
    if (fs_failed(ret))
    {
        FS_LOG_DUMP_ERROR("objtable_init failed, err=%s (0x%x)",
                          fs_error_str(ret), ret);
        FS_LOG_DUMP_INFO("exit: failed");
        return ret;
    }

    ret = objmgr_handle_index_init(&g_objmgr.handle_table,
                                   FS_HASH_DEFAULT_BUCKET_NR);
    if (fs_failed(ret))
    {
        FS_LOG_DUMP_ERROR("objmgr_handle_index_init failed, err=%s (0x%x)",
                          fs_error_str(ret), ret);
        objtable_destroy(&g_objmgr.table);
        FS_LOG_DUMP_INFO("exit: failed");
        return ret;
    }

    ret = fs_mutex_init(&g_objmgr.key_lock, "objmgr_key", 0);
    if (fs_failed(ret))
    {
        FS_LOG_DUMP_ERROR("fs_mutex_init key_lock failed, err=%s (0x%x)",
                          fs_error_str(ret), ret);
        objmgr_handle_index_deinit(&g_objmgr.handle_table);
        objtable_destroy(&g_objmgr.table);
        FS_LOG_DUMP_INFO("exit: failed");
        return ret;
    }

    objmgr_key_allocator_reset();

    ret = fs_mutex_init(&g_objmgr.lock, "objmgr", 0);
    if (fs_failed(ret))
    {
        FS_LOG_DUMP_ERROR("fs_mutex_init failed, err=%s (0x%x)",
                          fs_error_str(ret), ret);
        fs_mutex_destroy(&g_objmgr.key_lock);
        objmgr_handle_index_deinit(&g_objmgr.handle_table);
        objtable_destroy(&g_objmgr.table);
        FS_LOG_DUMP_INFO("exit: failed");
        return ret;
    }

    fs_atomic32_init(&g_objmgr.object_count, 0);

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void objmgr_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    fs_mutex_destroy(&g_objmgr.lock);
    fs_mutex_destroy(&g_objmgr.key_lock);

    objmgr_handle_index_deinit(&g_objmgr.handle_table);

    objtable_destroy(&g_objmgr.table);

    fs_atomic32_store(&g_objmgr.object_count, 0);
    objmgr_key_allocator_reset();

    FS_LOG_DUMP_INFO("exit: done");
}

/*
 * ============================================================
 * 对象状态查询
 * ============================================================
 */

obj_state_t objmgr_state(const fuid_t *fuid)
{
    obj_key_t key;

    obj_runtime_t *rt;

    FS_LOG_DUMP_INFO("enter: objectid=%llu gen=%u",
                     (unsigned long long)fuid->objectid,
                     (unsigned int)fuid->gen);

    objkey_from_fuid(&key, fuid);

    fs_mutex_lock(&g_objmgr.lock);

    rt = objmgr_lookup_locked(&key);

    if (rt == NULL)
    {
        fs_mutex_unlock(&g_objmgr.lock);

        FS_LOG_DUMP_INFO("exit: INVALID (not found)");

        return OBJ_STATE_INVALID;
    }

    fs_mutex_unlock(&g_objmgr.lock);

    FS_LOG_DUMP_INFO("exit: state=%u", (unsigned int)objruntime_state(rt));

    return objruntime_state(rt);
}

/*
 * ============================================================
 * 对象生命周期
 * ============================================================
 */

obj_meta_t *objmgr_create(const fuid_t *fuid, const obj_handle_t *handle)
{
    fs_error_t err;

    obj_runtime_t *rt;

    FS_LOG_DUMP_INFO("enter: objectid=%llu gen=%u",
                     (unsigned long long)fuid->objectid,
                     (unsigned int)fuid->gen);

    rt = objpool_alloc();
    if (rt == NULL)
    {
        FS_LOG_DUMP_ERROR("objpool_alloc failed");

        return NULL;
    }

    err = objmeta_init(&rt->meta, fuid, handle);

    if (err != FS_OK)
    {
        FS_LOG_DUMP_ERROR("objmeta_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);

        objpool_free(rt);

        return NULL;
    }

    rt->state = OBJ_STATE_INIT;
    rt->refcnt = 0;

    fs_mutex_lock(&g_objmgr.lock);

    err = objmgr_insert_locked(rt);

    if (err != FS_OK)
    {
        fs_mutex_unlock(&g_objmgr.lock);

        FS_LOG_DUMP_ERROR("insert_locked failed, err=%s (0x%x)",
                          fs_error_str(err), err);

        objmeta_reset(&rt->meta);

        objpool_free(rt);

        return NULL;
    }

    err = objmgr_change_state(rt, OBJ_STATE_ACTIVE);

    if (err != FS_OK)
    {
        FS_LOG_DUMP_ERROR("change_state failed, err=%s (0x%x)",
                          fs_error_str(err), err);

        objmgr_reclaim_locked(rt);

        fs_mutex_unlock(&g_objmgr.lock);

        return NULL;
    }

    fs_mutex_unlock(&g_objmgr.lock);

    FS_LOG_DUMP_INFO("exit: rt=%p, meta=%p", (void *)rt, (void *)&rt->meta);

    return &rt->meta;
}

fs_error_t objmgr_delete(const fuid_t *fuid)
{
    fs_error_t err;

    obj_key_t key;

    obj_runtime_t *rt;

    int32_t refcnt;

    FS_LOG_DUMP_INFO("enter: objectid=%llu gen=%u",
                     (unsigned long long)fuid->objectid,
                     (unsigned int)fuid->gen);

    objkey_from_fuid(&key, fuid);

    fs_mutex_lock(&g_objmgr.lock);

    rt = objmgr_lookup_locked(&key);

    if (rt == NULL)
    {
        fs_mutex_unlock(&g_objmgr.lock);

        err = obj_error(OBJ_SUB_DELETE, ENOENT);

        FS_LOG_DUMP_ERROR("lookup failed, err=%s (0x%x)", fs_error_str(err),
                          err);

        return err;
    }

    if (objruntime_state(rt) != OBJ_STATE_ACTIVE)
    {
        fs_mutex_unlock(&g_objmgr.lock);

        err = obj_error(OBJ_SUB_DELETE, EBUSY);

        FS_LOG_DUMP_ERROR("state not ACTIVE: state=%u, err=%s (0x%x)",
                          (unsigned int)objruntime_state(rt), fs_error_str(err),
                          err);

        return err;
    }

    err = objmgr_change_state(rt, OBJ_STATE_DELETING);

    if (err != FS_OK)
    {
        fs_mutex_unlock(&g_objmgr.lock);

        FS_LOG_DUMP_ERROR("change_state failed, err=%s (0x%x)",
                          fs_error_str(err), err);

        return err;
    }

    refcnt = fs_atomic32_load(&rt->refcnt);

    if (refcnt == 0)
    {
        objmgr_reclaim_locked(rt);
    }

    fs_mutex_unlock(&g_objmgr.lock);

    FS_LOG_DUMP_INFO("exit: ok");

    return FS_OK;
}

/*
 * ============================================================
 * 对象查找
 * ============================================================
 */

obj_meta_t *objmgr_lookup(const fuid_t *fuid)
{
    obj_key_t key;

    obj_runtime_t *rt;

    FS_LOG_DUMP_INFO("enter: objectid=%llu gen=%u",
                     (unsigned long long)fuid->objectid,
                     (unsigned int)fuid->gen);

    objkey_from_fuid(&key, fuid);

    fs_mutex_lock(&g_objmgr.lock);

    rt = objmgr_lookup_locked(&key);

    if (rt == NULL)
    {
        fs_mutex_unlock(&g_objmgr.lock);

        FS_LOG_DUMP_INFO("exit: NULL (not found)");

        return NULL;
    }

    if (objruntime_state(rt) != OBJ_STATE_ACTIVE)
    {
        fs_mutex_unlock(&g_objmgr.lock);

        FS_LOG_DUMP_INFO("exit: NULL (state=%u)",
                         (unsigned int)objruntime_state(rt));

        return NULL;
    }

    fs_mutex_unlock(&g_objmgr.lock);

    FS_LOG_DUMP_INFO("exit: rt=%p, meta=%p", (void *)rt, (void *)&rt->meta);

    return &rt->meta;
}

bool objmgr_exists(const fuid_t *fuid)
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
    return (uint32_t)fs_atomic32_load(&g_objmgr.object_count);
}
