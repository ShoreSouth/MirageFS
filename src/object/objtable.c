#include <stdlib.h>
#include <string.h>

#include "common/fs_common.h"
#include "object/objkey.h"
#include "object/objtable.h"

/* ============================================================
 * 内部函数
 * ============================================================ */

static uint64_t objtable_node_hash(
                    const fs_list_head_t *node)
{
    const objtable_entry_t *entry;

    entry = FS_CONTAINER_OF(
                node,
                objtable_entry_t,
                node);

    return ((uint64_t)entry->meta.key.objectid) ^
           ((uint64_t)entry->meta.key.gen);
}

static uint64_t objtable_key_hash(
                    const void *key)
{
    const objkey_t *objkey;

    objkey = key;

    return ((uint64_t)objkey->objectid) ^
           ((uint64_t)objkey->gen);
}

static bool objtable_match(
                    const fs_list_head_t *node,
                    const void *key)
{
    const objtable_entry_t *entry;
    const objkey_t *objkey;

    entry = FS_CONTAINER_OF(
                node,
                objtable_entry_t,
                node);

    objkey = key;

    return (entry->meta.key.objectid ==
            objkey->objectid) &&

           (entry->meta.key.gen ==
            objkey->gen);
}

static objtable_entry_t *objtable_find_entry(
                            objtable_t *table,
                            const objkey_t *key)
{
    fs_list_head_t *node;

    node = fs_hash_lookup(
                &table->table,
                key);

    if (node == NULL) {
        return NULL;
    }

    return FS_CONTAINER_OF(
                node,
                objtable_entry_t,
                node);
}

/* ============================================================
 * 生命周期
 * ============================================================ */

int objtable_init(
            objtable_t *table,
            uint32_t bucket_nr)
{
    if (table == NULL) {
        FS_LOG_DUMP_ERROR("table is NULL");
        return -1;
    }

    memset(table, 0, sizeof(objtable_t));

    return fs_hash_init(
                &table->table,
                bucket_nr,
                objtable_node_hash,
                objtable_key_hash,
                objtable_match);
}

void objtable_destroy(objtable_t *table)
{
    uint32_t i;

    fs_list_head_t *pos;
    fs_list_head_t *next;

    objtable_entry_t *entry;

    if (table == NULL) {
        return;
    }

    for (i = 0;
         i < table->table.bucket_nr;
         i++) {

        FS_LIST_FOR_EACH_SAFE(
                pos,
                next,
                &table->table.buckets[i]) {

            entry = FS_CONTAINER_OF(
                        pos,
                        objtable_entry_t,
                        node);

            fs_hash_remove(
                    &table->table,
                    &entry->node);

            free(entry);
        }
    }

    fs_hash_destroy(
            &table->table);

    memset(table,
           0,
           sizeof(objtable_t));
}

/* ============================================================
 * 基础操作
 * ============================================================ */

int objtable_insert(
            objtable_t *table,
            const ObjMeta_t *meta)
{
    objtable_entry_t *entry;

    objkey_t key;

    if (table == NULL) {
        FS_LOG_DUMP_ERROR("table is NULL");
        return -1;
    }

    if (meta == NULL) {
        FS_LOG_DUMP_ERROR("meta is NULL");
        return -1;
    }

    if (!objmeta_is_valid(meta)) {
        FS_LOG_DUMP_ERROR("invalid meta");
        return -1;
    }

    key = objkey_make(
                meta->key.objectid,
                meta->key.gen);

    if (objtable_exists(
            table,
            &key)) {

        FS_LOG_DUMP_ERROR(
                "object already exists");

        return -1;
    }

    entry = calloc(
                1,
                sizeof(objtable_entry_t));

    if (entry == NULL) {
        FS_LOG_DUMP_ERROR("calloc failed");
        return -1;
    }

    memcpy(&entry->meta,
           meta,
           sizeof(ObjMeta_t));

    fs_list_init(
            &entry->node);

    if (fs_hash_insert(
            &table->table,
            &entry->node) != 0) {

        free(entry);
        return -1;
    }

    return 0;
}

int objtable_remove(
            objtable_t *table,
            const objkey_t *key)
{
    objtable_entry_t *entry;

    if (table == NULL) {
        FS_LOG_DUMP_ERROR("table is NULL");
        return -1;
    }

    if (key == NULL) {
        FS_LOG_DUMP_ERROR("key is NULL");
        return -1;
    }

    entry = objtable_find_entry(
                table,
                key);

    if (entry == NULL) {
        return -1;
    }

    fs_hash_remove(
            &table->table,
            &entry->node);

    free(entry);

    return 0;
}

ObjMeta_t *objtable_lookup(
                objtable_t *table,
                const objkey_t *key)
{
    objtable_entry_t *entry;

    if (table == NULL) {
        return NULL;
    }

    if (key == NULL) {
        return NULL;
    }

    entry = objtable_find_entry(
                table,
                key);

    if (entry == NULL) {
        return NULL;
    }

    return &entry->meta;
}

bool objtable_exists(
            objtable_t *table,
            const objkey_t *key)
{
    return (objtable_lookup(
                table,
                key) != NULL);
}

/* ============================================================
 * 统计
 * ============================================================ */

uint64_t objtable_count(
            const objtable_t *table)
{
    if (table == NULL) {
        return 0;
    }

    return fs_hash_count(
                &table->table);
}