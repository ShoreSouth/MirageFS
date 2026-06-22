#pragma once

#include "common/fs_common.h"
#include "object/fuid/fuid.h"
#include "object/objtable/objtable.h"
#include "object/objmeta/objmeta.h"

/*
 * ============================================================
 * objmgr internal
 *
 * Internal definitions shared by objmgr implementation.
 *
 * This header is private to objmgr and must not be included by
 * other modules.
 * ============================================================
 */

/*
 * ============================================================
 * 对象管理器
 *
 * Object Layer 全局管理器。
 *
 * 整个系统仅维护一个 obj_manager_t 实例，
 * 负责管理所有对象的运行时状态，包括：
 *
 *   1. 对象索引（ObjTable）
 *   2. 生命周期管理
 *   3. 全局同步
 *   4. 对象数量统计
 *
 * 所有 ObjMeta 均由该管理器统一管理。
 * ============================================================
 */
typedef struct obj_manager
{
    obj_table_t table; /* 全局对象索引 */
    fs_mutex_t lock; /* 全局互斥锁 */
    fs_atomic32_t object_count; /* 当前对象数量 */
} obj_manager_t;

/*
 * ============================================================
 * 全局管理器实例
 * ============================================================
 */

extern obj_manager_t g_objmgr;

/*
 * ============================================================
 * internal helpers
 * ============================================================
 */

/*
 * 查找对象
 *
 * 调用者必须持有 objmgr 锁
 */
obj_meta_t *objmgr_lookup_locked(
                const obj_key_t *key);

/*
 * 插入对象
 *
 * 调用者必须持有 objmgr 锁
 */
int32_t objmgr_insert_locked(
                obj_meta_t *meta);

/*
 * 移除对象
 *
 * 调用者必须持有 objmgr 锁
 */
int32_t objmgr_remove_locked(
                const obj_key_t *key);

/*
 * 变更对象状态
 *
 * 调用者必须持有 objmgr 锁
 */
int32_t objmgr_change_state(
                obj_meta_t *meta,
                obj_state_t state);