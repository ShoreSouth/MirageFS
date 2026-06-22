#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "object/objmeta/objmeta.h"
#include "object/objkey/objkey.h"

/* ============================================================
 * Object Table Entry
 * ============================================================ */

/*
 * 对象表节点。
 *
 * 包装 ObjMeta 并附加 hash 链表节点，
 * 供 ObjTable 内部使用。
 */
typedef struct objtable_entry {

    obj_meta_t meta; /* 对象元数据 */

    fs_list_head_t node; /* hash 冲突链表节点 */

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
 *           ObjMeta
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
 *      table       : 目标对象表
 *      bucket_nr   : hash bucket 数量
 *
 * 返回：
 *      0           : 成功
 *      -1          : 失败
 */
int objtable_init(
            obj_table_t *table,
            uint32_t bucket_nr);

/*
 * 销毁对象表。
 *
 * 释放所有 entry 及内部 hash 资源。
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
 *      table       : 对象表
 *      meta        : 待插入的元数据（深拷贝）
 *
 * 返回：
 *      0           : 成功
 *      -1          : 失败（已存在或参数无效）
 */
int objtable_insert(
            obj_table_t *table,
            const obj_meta_t *meta);

/*
 * 删除对象。
 *
 * 参数：
 *      table       : 对象表
 *      key         : 对象标识
 *
 * 返回：
 *      0           : 成功
 *      -1          : 失败（不存在或参数无效）
 */
int objtable_remove(
            obj_table_t *table,
            const obj_key_t *key);

/*
 * 查找对象。
 *
 * 返回：
 *      NULL        : 未找到
 *      非 NULL     : 对象元数据指针（由 objtable 管理生命周期）
 */
obj_meta_t *objtable_lookup(
                obj_table_t *table,
                const obj_key_t *key);

/*
 * 判断对象是否存在。
 */
bool objtable_exists(
            obj_table_t *table,
            const obj_key_t *key);

/* ============================================================
 * 统计
 * ============================================================ */

/*
 * 当前对象数量。
 */
uint64_t objtable_count(
            const obj_table_t *table);
