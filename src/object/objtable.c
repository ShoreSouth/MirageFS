#include "object/objtable.h"

#include "common/fs_common.h"

#include <stdlib.h>
#include <string.h>

/* ============================================================
 * 内部函数
 * ============================================================ */

static uint64_t objtable_key(
                const fs_list_head_t *node)
{
    const objtable_entry_t *entry;

    entry = FS_CONTAINER_OF(
                node,
                objtable_entry_t,
                node);

    return entry->meta.objectid;
}

static bool objtable_match(
                const fs_list_head_t *node,
                uint64_t objectid)
{
    const objtable_entry_t *entry;

    entry = FS_CONTAINER_OF(
                node,
                objtable_entry_t,
                node);

    return (entry->meta.objectid == objectid);
}

static objtable_entry_t *objtable_find_entry(
                        objtable_t *table,
                        uint64_t objectid)
{
    fs_list_head_t *node;

    node = fs_hash_lookup(
                &table->table,
                objectid);

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

int objtable_init(objtable_t *table,
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
                objtable_key,
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

    for (i = 0; i < table->table.bucket_nr; i++) {

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

    fs_hash_destroy(&table->table);

    memset(table, 0, sizeof(objtable_t));
}

/* ============================================================
 * 基础操作
 * ============================================================ */

int objtable_insert(objtable_t *table,
                    const ObjMeta_t *meta)
{
    objtable_entry_t *entry;

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

    if (objtable_exists(table,
                        meta->objectid)) {

        FS_LOG_DUMP_ERROR(
                "objectid=%lu already exists",
                meta->objectid);

        return -1;
    }

    entry = calloc(1,
                   sizeof(objtable_entry_t));

    if (entry == NULL) {
        FS_LOG_DUMP_ERROR("calloc failed");
        return -1;
    }

    memcpy(&entry->meta,
           meta,
           sizeof(ObjMeta_t));

    fs_list_init(&entry->node);

    if (fs_hash_insert(
            &table->table,
            &entry->node) != 0) {

        free(entry);
        return -1;
    }

    return 0;
}

int objtable_remove(objtable_t *table,
                    uint64_t objectid)
{
    objtable_entry_t *entry;

    if (table == NULL) {
        FS_LOG_DUMP_ERROR("table is NULL");
        return -1;
    }

    entry = objtable_find_entry(
                table,
                objectid);

    if (entry == NULL) {
        return -1;
    }

    fs_hash_remove(
            &table->table,
            &entry->node);

    free(entry);

    return 0;
}

ObjMeta_t *objtable_lookup(objtable_t *table,
                           uint64_t objectid)
{
    objtable_entry_t *entry;

    if (table == NULL) {
        return NULL;
    }

    entry = objtable_find_entry(
                table,
                objectid);

    if (entry == NULL) {
        return NULL;
    }

    return &entry->meta;
}

bool objtable_exists(objtable_t *table,
                     uint64_t objectid)
{
    return (objtable_lookup(
                table,
                objectid) != NULL);
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