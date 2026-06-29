#include <stdlib.h>
#include <string.h>

#include "common/fs_common.h"
#include "object/obj_error.h"
#include "object/objkey/objkey.h"
#include "object/objtable/objtable.h"

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

    return objkey_hash(&entry->runtime->meta.key);
}

static uint64_t objtable_key_hash(
                    const void *key)
{
    return objkey_hash(key);
}

static bool objtable_match(
                    const fs_list_head_t *node,
                    const void *key)
{
    const objtable_entry_t *entry;

    entry = FS_CONTAINER_OF(
                node,
                objtable_entry_t,
                node);

    return objkey_equal(&entry->runtime->meta.key, key);
}

static objtable_entry_t *objtable_find_entry(
                            obj_table_t *table,
                            const obj_key_t *key)
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

fs_error_t objtable_init(
            obj_table_t *table,
            uint32_t bucket_nr)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter: table=%p, bucket_nr=%u",
                     (void *)table, bucket_nr);

    if (table == NULL) {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: table is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    memset(table, 0, sizeof(obj_table_t));

    err = fs_hash_init(
                &table->table,
                bucket_nr,
                objtable_node_hash,
                objtable_key_hash,
                objtable_match);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_hash_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void objtable_destroy(obj_table_t *table)
{
    uint32_t i;

    fs_list_head_t *pos;
    fs_list_head_t *next;

    objtable_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: table=%p", (void *)table);

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
           sizeof(obj_table_t));

    FS_LOG_DUMP_INFO("exit: done");
}

/* ============================================================
 * 基础操作
 * ============================================================ */

fs_error_t objtable_insert(
            obj_table_t *table,
            obj_runtime_t *runtime)
{
    fs_error_t err;

    objtable_entry_t *entry;

    obj_key_t key;

    FS_LOG_DUMP_INFO("enter: table=%p, runtime=%p",
                     (void *)table, (void *)runtime);

    if (table == NULL) {
        err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: table is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (runtime == NULL) {
        err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: runtime is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (!objmeta_is_valid(&runtime->meta)) {
        err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid meta, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    key = objkey_make(
                runtime->meta.key.objectid,
                runtime->meta.key.gen);

    if (objtable_exists(
            table,
            &key)) {

        err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_EEXIST);
        FS_LOG_DUMP_ERROR(
                "insert failed: object already exists, "
                "key=(%lu,%u), err=%s (0x%x)",
                (unsigned long)key.objectid,
                (unsigned int)key.gen,
                fs_error_str(err), err);
        return err;
    }

    entry = calloc(
                1,
                sizeof(objtable_entry_t));

    if (entry == NULL) {
        err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_ENOMEM);
        FS_LOG_DUMP_ERROR("calloc failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    entry->runtime = runtime;

    fs_list_init(
            &entry->node);

    err = fs_hash_insert(
            &table->table,
            &entry->node);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_hash_insert failed, err=%s (0x%x)",
                          fs_error_str(err), err);

        free(entry);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

fs_error_t objtable_remove(
            obj_table_t *table,
            const obj_key_t *key)
{
    fs_error_t err;

    objtable_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: table=%p, key=%p",
                     (void *)table, (void *)key);

    if (table == NULL) {
        err = obj_error(OBJ_SUB_REMOVE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: table is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (key == NULL) {
        err = obj_error(OBJ_SUB_REMOVE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: key is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    entry = objtable_find_entry(
                table,
                key);

    if (entry == NULL) {

        err = obj_error(OBJ_SUB_REMOVE, FS_ERRNO_ENOENT);
        FS_LOG_DUMP_ERROR(
                "remove failed: object not found, "
                "key=(%lu,%u), err=%s (0x%x)",
                (unsigned long)key->objectid,
                (unsigned int)key->gen,
                fs_error_str(err), err);
        return err;
    }

    fs_hash_remove(
            &table->table,
            &entry->node);

    free(entry);

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

obj_runtime_t *objtable_lookup(
                obj_table_t *table,
                const obj_key_t *key)
{
    objtable_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: table=%p, key=%p",
                     (void *)table, (void *)key);

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
        FS_LOG_DUMP_INFO("exit: not found");
        return NULL;
    }

    FS_LOG_DUMP_INFO("exit: found, runtime=%p",
                     (void *)entry->runtime);
    return entry->runtime;
}

bool objtable_exists(
            obj_table_t *table,
            const obj_key_t *key)
{
    bool exists;

    FS_LOG_DUMP_INFO("enter: table=%p, key=%p",
                     (void *)table, (void *)key);

    exists = (objtable_lookup(
                table,
                key) != NULL);

    FS_LOG_DUMP_INFO("exit: %s",
                     exists ? "true" : "false");
    return exists;
}

/* ============================================================
 * 统计
 * ============================================================ */

uint64_t objtable_count(
            const obj_table_t *table)
{
    uint64_t count;

    FS_LOG_DUMP_INFO("enter: table=%p", (void *)table);

    if (table == NULL) {
        count = 0;
    } else {
        count = fs_hash_count(
                    &table->table);
    }

    FS_LOG_DUMP_INFO("exit: count=%lu",
                     (unsigned long)count);
    return count;
}
