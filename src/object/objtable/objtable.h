#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "object/objruntime/objruntime.h"
#include "object/objkey/objkey.h"

/* ============================================================
 * Object Table Entry
 * ============================================================ */

/*
 * 对象表节点。
 *
 * 持有 obj_runtime_t 指针并附加 hash 链表节点，
 * 供 ObjTable 内部使用。
 *
 * runtime 指向 pool 统一管理的 obj_runtime_t 实例，
 * objtable 不持有数据副本，仅做索引引用。
 */
typedef struct objtable_entry {

    obj_runtime_t  *runtime; /* 运行时对象（pool 统一管理） */

    fs_list_head_t  node;    /* hash 冲突链表节点 */

} objtable_entry_t;

/* ============================================================
 * Object Table
 * ============================================================ */

/*
 * MirageFS 全局对象注册表。
 *
 * 维护映射关系：
 *
 *      (objectid, gen)
 *              ↓
 *         obj_runtime_t
 *
 * 内部基于 fs_hash 实现，
 * 不直接管理 Bucket 和链表。
 */
typedef struct obj_table {

    fs_hash_t table;

} obj_table_t;

/* ============================================================
 * 生命周期
 * ============================================================ */

/*
 * 初始化对象表。
 *
 * 参数：
 *      [OUT] table       : 目标对象表
 *      [IN]  bucket_nr   : hash bucket 数量
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 失败
 */
fs_error_t objtable_init(
            obj_table_t *table,
            uint32_t bucket_nr);

/*
 * 销毁对象表。
 *
 * 释放所有 entry 及内部 hash 资源。
 *
 * 注意：仅释放 entry wrapper（calloc/free），
 * runtime 本身由 objpool 管理。
 *
 * 参数：
 *      [IN/OUT] table  : 对象表（资源将被释放并置零）
 */
void objtable_destroy(
            obj_table_t *table);

/* ============================================================
 * 基础操作
 * ============================================================ */

/*
 * 插入对象。
 *
 * 若 (objectid, gen) 已存在则返回失败。
 *
 * 参数：
 *      [IN/OUT] table   : 对象表
 *      [IN]     runtime : 待索引的运行时对象（由 pool 分配）
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 失败（已存在或参数无效）
 */
fs_error_t objtable_insert(
            obj_table_t *table,
            obj_runtime_t *runtime);

/*
 * 删除对象。
 *
 * 参数：
 *      [IN/OUT] table  : 对象表
 *      [IN]     key    : 对象标识
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 失败（不存在或参数无效）
 */
fs_error_t objtable_remove(
            obj_table_t *table,
            const obj_key_t *key);

/*
 * 查找对象。
 *
 * 参数：
 *      [IN] table  : 对象表
 *      [IN] key    : 对象标识
 *
 * 返回：
 *      NULL        : 未找到
 *      非 NULL     : 运行时对象指针（由 objtable 索引，pool 管理生命周期）
 */
obj_runtime_t *objtable_lookup(
                obj_table_t *table,
                const obj_key_t *key);

/*
 * 判断对象是否存在。
 *
 * 参数：
 *      [IN] table  : 对象表
 *      [IN] key    : 对象标识
 */
bool objtable_exists(
            obj_table_t *table,
            const obj_key_t *key);

/* ============================================================
 * 统计
 * ============================================================ */

/*
 * 当前对象数量。
 *
 * 参数：
 *      [IN] table  : 对象表
 */
uint64_t objtable_count(
            const obj_table_t *table);
