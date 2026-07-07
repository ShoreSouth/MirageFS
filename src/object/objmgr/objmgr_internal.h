#pragma once

#include "common/fs_common.h"

#include "object/objruntime/objruntime.h"
#include "object/objkey/objkey.h"
#include "object/objtable/objtable.h"

#define OBJMGR_KEY_MAX_SLOTS 65536U
#define OBJMGR_KEY_GENERATION_INIT 1U

/*
 * ============================================================
 * objmgr internal
 *
 * ObjMgr 私有内部定义。
 *
 * 本文件仅供 ObjMgr 内部实现使用，包括：
 *
 *      objmgr.c
 *      objmgr_ref.c
 *      objmgr_internal.c
 *
 * 外部模块禁止包含本头文件。
 * ============================================================
 */

/*
 * ============================================================
 * 对象管理器
 *
 * Object Layer 全局运行时管理器。
 *
 * 整个系统仅维护一个 obj_manager_t 实例，负责：
 *
 *   1. 管理全局对象索引（ObjTable）
 *   2. 维护对象生命周期
 *   3. 提供全局同步保护
 *   4. 统计当前对象数量
 *
 * 所有运行时对象均由 ObjMgr 统一管理。
 * ============================================================
 */
typedef struct obj_manager
{

    obj_table_t table; /* 全局对象索引 */
    fs_hash_t handle_table; /* backend handle index */

    /*
     * 当前采用一级全局锁保护所有对象。
     * 后续如需提升并发性能，可演进为：
     *
     *      - RWLock
     *      - Bucket Lock
     *      - Object Lock
     */
    fs_mutex_t lock; /* 全局互斥锁 */

    fs_atomic32_t object_count; /* 当前对象数量 */
    fs_mutex_t key_lock; /* protects key allocator */
    uint32_t key_free_count; /* free key slots */
    uint64_t key_free_stack[OBJMGR_KEY_MAX_SLOTS]; /* free objectid stack */
    uint32_t key_generation[OBJMGR_KEY_MAX_SLOTS + 1U]; /* slot generation */
    uint8_t key_allocated[OBJMGR_KEY_MAX_SLOTS + 1U]; /* allocation bitmap */

} obj_manager_t;

/*
 * ============================================================
 * 全局管理器
 * ============================================================
 */

extern obj_manager_t g_objmgr;

/*
 * ============================================================
 * ObjTable Helper
 *
 * 调用者必须已经持有 objmgr 全局锁。
 * ============================================================
 */

/*
 * 查找对象。
 *
 * 参数：
 *      [IN] key    : 对象标识
 */
obj_runtime_t *objmgr_lookup_locked(
                const obj_key_t *key);

fs_error_t objmgr_handle_index_init(
                fs_hash_t *table,
                uint32_t bucket_nr);

void objmgr_handle_index_deinit(
                fs_hash_t *table);

obj_runtime_t *objmgr_lookup_handle_locked(
                const obj_handle_t *handle);

fs_error_t objmgr_insert_handle_locked(
                obj_runtime_t *rt);

void objmgr_remove_handle_locked(
                obj_runtime_t *rt);

void objmgr_free_key_locked(
                const obj_key_t *key);

/*
 * 插入对象。
 *
 * 参数：
 *      [IN] rt     : 待插入的运行时对象（由 pool 分配）
 */
fs_error_t objmgr_insert_locked(
                obj_runtime_t *rt);

/*
 * 删除对象。
 *
 * 参数：
 *      [IN] key    : 对象标识
 */
fs_error_t objmgr_remove_locked(
                const obj_key_t *key);

/*
 * ============================================================
 * 生命周期 Helper
 *
 * 调用者必须已经持有 objmgr 全局锁。
 * ============================================================
 */

/*
 * 判断状态迁移是否合法。
 *
 * 参数：
 *      [IN] from   : 当前状态
 *      [IN] to     : 目标状态
 */
bool objmgr_state_can_transit(
                obj_state_t from,
                obj_state_t to);

/*
 * 修改对象生命周期状态。
 *
 * 参数：
 *      [IN/OUT] rt     : 运行时对象
 *      [IN]     state  : 目标状态
 */
fs_error_t objmgr_change_state(
                obj_runtime_t *rt,
                obj_state_t state);

/*
 * ============================================================
 * 引用计数 Helper
 *
 * 调用者必须已经持有 objmgr 全局锁。
 * ============================================================
 */

/*
 * 增加对象引用。
 *
 * 内部完成状态检查及引用计数增加。
 *
 * 参数：
 *      [IN/OUT] rt : 运行时对象（refcnt 将被递增）
 */
fs_error_t objmgr_ref_get_locked(
                obj_runtime_t *rt);

/*
 * 释放对象引用。
 *
 * 当引用计数降为 0 时，
 * 根据对象状态决定是否执行最终释放。
 *
 * 参数：
 *      [IN/OUT] rt : 运行时对象（refcnt 将被递减，可能回收）
 */
fs_error_t objmgr_ref_put_locked(
                obj_runtime_t *rt);

/*
 * ============================================================
 * 回收 Helper
 *
 * 从表中移除对象，重置元数据，归还内存池。
 * 仅在 refcnt==0 && state==DELETING 时调用。
 *
 * 调用者必须已经持有 objmgr 全局锁。
 * ============================================================
 */

/*
 * 从表中移除对象，重置元数据，归还内存池。
 *
 * 参数：
 *      [IN] rt     : 待回收的运行时对象（将被重置并归还 pool）
 */
void objmgr_reclaim_locked(
                obj_runtime_t *rt);
