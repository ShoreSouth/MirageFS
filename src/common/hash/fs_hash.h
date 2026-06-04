#pragma once

#include <stdint.h>

#include "common/list/fs_list.h"

/* ============================================================
 * 默认配置
 * ============================================================ */

#define FS_HASH_DEFAULT_BUCKET_NR 1024

/* ============================================================
 * 回调定义
 * ============================================================ */

/*
 * 获取节点 key。
 *
 * 用户数据结构中必须嵌入 fs_list_head_t。
 *
 * hash 模块不关心具体对象类型，
 * 仅通过回调获取 key。
 */
typedef uint64_t (*fs_hash_key_fn)(const fs_list_head_t *node);

/*
 * key 比较函数。
 */
typedef bool (*fs_hash_match_fn)(const fs_list_head_t *node,
                                uint64_t key);

/* ============================================================
 * HashTable
 * ============================================================ */

typedef struct fs_hash {

    uint32_t bucket_nr; /* hash桶数量 */

    uint64_t entry_nr; /* 当前节点数量 */

    fs_list_head_t *buckets; /* bucket数组，每个bucket是一个链表头 */

    fs_hash_key_fn key_fn; /* 从节点获取key */

    fs_hash_match_fn match_fn; /* 判断节点是否匹配指定key */

} fs_hash_t;

/* ============================================================
 * 生命周期
 * ============================================================ */

/*
 * 初始化。
 */
int fs_hash_init(fs_hash_t *hash,
                 uint32_t bucket_nr,
                 fs_hash_key_fn key_fn,
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
int fs_hash_insert(fs_hash_t *hash,
                   fs_list_head_t *node);

/*
 * 删除节点。
 */
void fs_hash_remove(fs_hash_t *hash,
                    fs_list_head_t *node);

/*
 * 查找节点。
 *
 * 返回：
 *      NULL    未找到
 *      node    找到
 */
fs_list_head_t *fs_hash_lookup(fs_hash_t *hash,
                         uint64_t key);

/* ============================================================
 * 统计
 * ============================================================ */

static inline uint64_t fs_hash_count(
                    const fs_hash_t *hash)
{
    return hash->entry_nr;
}