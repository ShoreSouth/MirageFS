#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/list/fs_list.h"
#include "common/error/fs_common_sub.h"

/* ============================================================
 * 默认配置
 * ============================================================ */

#define FS_HASH_DEFAULT_BUCKET_NR 1024

/* ============================================================
 * 回调定义
 * ============================================================ */

/*
 * 节点哈希函数。
 *
 * 插入节点时使用。
 */
typedef uint64_t (*fs_hash_node_hash_fn)(
                    const fs_list_head_t *node);

/*
 * Key哈希函数。
 *
 * 查找节点时使用。
 */
typedef uint64_t (*fs_hash_key_hash_fn)(
                    const void *key);

/*
 * 节点匹配函数。
 *
 * key 为业务自定义查询条件。
 */
typedef bool (*fs_hash_match_fn)(
                    const fs_list_head_t *node,
                    const void *key);

/* ============================================================
 * HashTable
 * ============================================================ */

typedef struct fs_hash {

    uint32_t bucket_nr; /* hash桶数量 */

    uint64_t entry_nr; /* 当前节点数量 */

    fs_list_head_t *buckets; /* bucket数组 */

    fs_hash_node_hash_fn node_hash_fn; /* 节点哈希函数 */

    fs_hash_key_hash_fn key_hash_fn; /* Key哈希函数 */

    fs_hash_match_fn match_fn; /* 节点匹配函数 */

} fs_hash_t;

/* ============================================================
 * 生命周期
 * ============================================================ */

/*
 * 初始化。
 */
fs_error_t fs_hash_init(fs_hash_t *hash,
                 uint32_t bucket_nr,
                 fs_hash_node_hash_fn hash_fn,
                 fs_hash_key_hash_fn key_hash_fn,
                 fs_hash_match_fn match_fn);

/*
 * 销毁。
 */
void fs_hash_destroy(fs_hash_t *hash);

/* ============================================================
 * 基础操作
 * ============================================================ */

/*
 * 插入节点。
 *
 * node 必须已初始化。
 */
fs_error_t fs_hash_insert(fs_hash_t *hash,
                   fs_list_head_t *node);

/*
 * 删除节点。
 */
void fs_hash_remove(fs_hash_t *hash,
                    fs_list_head_t *node);

/*
 * 查找节点。
 *
 * key 为业务自定义查询条件。
 *
 * 返回：
 *      NULL    未找到
 *      node    找到
 */
fs_list_head_t *fs_hash_lookup(
                    fs_hash_t *hash,
                    const void *key);

/* ============================================================
 * 统计
 * ============================================================ */

static inline uint64_t fs_hash_count(
                    const fs_hash_t *hash)
{
    return hash->entry_nr;
}
