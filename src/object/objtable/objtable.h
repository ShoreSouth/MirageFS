#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "object/objmeta/objmeta.h"
#include "object/objtable/objkey.h"

/* ============================================================
 * Object Table Entry
 * ============================================================ */

/*
 * (objectid, gen) -> ObjMeta
 */
typedef struct objtable_entry {

    obj_meta_t meta; /* 对象元数据 */

    fs_list_head_t node; /* hash节点 */

} objtable_entry_t;

/* ============================================================
 * Object Table
 * ============================================================ */

/*
 * MirageFS 全局对象注册表。
 *
 * 维护：
 *
 *      (objectid, gen)
 *              ↓
 *           ObjMeta
 *
 * 映射关系。
 */
typedef struct objtable {

    fs_hash_t table;

} obj_table_t;

/* ============================================================
 * 生命周期
 * ============================================================ */

/*
 * 初始化对象表。
 */
int objtable_init(
            obj_table_t *table,
            uint32_t bucket_nr);

/*
 * 销毁对象表。
 */
void objtable_destroy(
            obj_table_t *table);

/* ============================================================
 * 基础操作
 * ============================================================ */

/*
 * 插入对象。
 *
 * 若(objid, gen)已存在返回失败。
 */
int objtable_insert(
            obj_table_t *table,
            const obj_meta_t *meta);

/*
 * 删除对象。
 */
int objtable_remove(
            obj_table_t *table,
            const obj_key_t *key);

/*
 * 查找对象。
 *
 * 返回：
 *      NULL    未找到
 *      meta    找到
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