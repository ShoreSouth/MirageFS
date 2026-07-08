#include "common/hash/fs_hash.h"

#include <stdlib.h>
#include <string.h>

#include "common/list/fs_list.h"
#include "common/log/fs_log.h"

/* ============================================================
 * 内部函数
 * ============================================================ */

static uint32_t fs_hash_index(
                    const fs_hash_t *hash,
                    uint64_t hash_value)
{
    return (uint32_t)(hash_value % hash->bucket_nr);
}

/* ============================================================
 * 生命周期
 * ============================================================ */

fs_error_t fs_hash_init(
            fs_hash_t *hash,
            uint32_t bucket_nr,
            fs_hash_node_hash_fn node_hash_fn,
            fs_hash_key_hash_fn key_hash_fn,
            fs_hash_match_fn match_fn)
{
    uint32_t i;

    if (hash == NULL) {
        FS_LOG_DUMP_ERROR("hash is NULL");
        return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
    }

    if (bucket_nr == 0) {
        FS_LOG_DUMP_ERROR("invalid bucket_nr");
        return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
    }

    if (node_hash_fn == NULL) {
        FS_LOG_DUMP_ERROR("node_hash_fn is NULL");
        return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
    }

    if (key_hash_fn == NULL) {
        FS_LOG_DUMP_ERROR("key_hash_fn is NULL");
        return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
    }

    if (match_fn == NULL) {
        FS_LOG_DUMP_ERROR("match_fn is NULL");
        return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
    }

    memset(hash, 0, sizeof(fs_hash_t));

    hash->buckets = calloc(bucket_nr,
                           sizeof(fs_list_head_t));
    if (hash->buckets == NULL) {
        FS_LOG_DUMP_ERROR("calloc buckets failed");
        return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_ENOMEM);
    }

    for (i = 0; i < bucket_nr; i++) {
        fs_list_init(&hash->buckets[i]);
    }

    hash->bucket_nr = bucket_nr;

    hash->node_hash_fn = node_hash_fn;
    hash->key_hash_fn = key_hash_fn;
    hash->match_fn = match_fn;

    return FS_OK;
}

void fs_hash_destroy(fs_hash_t *hash)
{
    if (hash == NULL) {
        return;
    }

    free(hash->buckets);

    memset(hash, 0, sizeof(fs_hash_t));
}

/* ============================================================
 * 基础操作
 * ============================================================ */

fs_error_t fs_hash_insert(
            fs_hash_t *hash,
            fs_list_head_t *node)
{
    uint64_t hash_value;
    uint32_t index;

    if ((hash == NULL) ||
        (node == NULL)) {
        return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
    }

    hash_value = hash->node_hash_fn(node);

    index = fs_hash_index(hash,
                          hash_value);

    fs_list_add_tail(node,
                     &hash->buckets[index]);

    hash->entry_nr++;

    return FS_OK;
}

void fs_hash_remove(
            fs_hash_t *hash,
            fs_list_head_t *node)
{
    if ((hash == NULL) ||
        (node == NULL)) {
        return;
    }

    fs_list_del(node);

    if (hash->entry_nr > 0) {
        hash->entry_nr--;
    }
}

fs_list_head_t *fs_hash_lookup(
                    fs_hash_t *hash,
                    const void *key)
{
    uint64_t hash_value;
    uint32_t index;

    fs_list_head_t *pos;

    if ((hash == NULL) ||
        (key == NULL)) {
        return NULL;
    }

    hash_value = hash->key_hash_fn(key);

    index = fs_hash_index(hash,
                          hash_value);

    FS_LIST_FOR_EACH(pos,
                     &hash->buckets[index]) {

        if (hash->match_fn(pos,
                           key)) {
            return pos;
        }
    }

    return NULL;
}
