#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "meta/objmeta.h"
#include "object/objkey.h"

/* ============================================================
 * Object Table Entry
 * ============================================================ */

/*
 * (objectid, gen) -> ObjMeta
 */
typedef struct objtable_entry {

    ObjMeta_t meta; /* 对象元数据 */

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

} objtable_t;

/* ============================================================
 * 生命周期
 * ============================================================ */

/*
 * 初始化对象表。
 */
int objtable_init(
            objtable_t *table,
            uint32_t bucket_nr);

/*
 * 销毁对象表。
 */
void objtable_destroy(
            objtable_t *table);

/* ============================================================
 * 基础操作
 * ============================================================ */

/*
 * 插入对象。
 *
 * 若(objid, gen)已存在返回失败。
 */
int objtable_insert(
            objtable_t *table,
            const ObjMeta_t *meta);

/*
 * 删除对象。
 */
int objtable_remove(
            objtable_t *table,
            const objkey_t *key);

/*
 * 查找对象。
 *
 * 返回：
 *      NULL    未找到
 *      meta    找到
 */
ObjMeta_t *objtable_lookup(
                objtable_t *table,
                const objkey_t *key);

/*
 * 判断对象是否存在。
 */
bool objtable_exists(
            objtable_t *table,
            const objkey_t *key);

/* ============================================================
 * 统计
 * ============================================================ */

/*
 * 当前对象数量。
 */
uint64_t objtable_count(
            const objtable_t *table);