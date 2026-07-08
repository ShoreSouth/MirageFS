#include "fsc/fstable/fstable.h"

#include <stdlib.h>
#include <string.h>

#include "fsc/fsc_error.h"

/*
 * ============================================================
 * private helper
 * ============================================================
 */

static uint64_t fstable_name_hash(
                const char *name)
{
    uint64_t hash;
    const unsigned char *p;

    hash = 1469598103934665603ULL;
    p = (const unsigned char *)name;

    while (*p != '\0') {
        hash ^= (uint64_t)*p;
        hash *= 1099511628211ULL;
        p++;
    }

    return hash;
}

static uint64_t fstable_fsid_node_hash(
                    const fs_list_head_t *node)
{
    const fsc_table_entry_t *entry;

    entry = FS_CONTAINER_OF(node, fsc_table_entry_t, fsid_node);

    return fsid_hash(entry->ns->fsid);
}

static uint64_t fstable_fsid_key_hash(
                    const void *key)
{
    const fsc_fsid_t *fsid;

    fsid = (const fsc_fsid_t *)key;

    return fsid_hash(*fsid);
}

static bool fstable_fsid_match(
                    const fs_list_head_t *node,
                    const void *key)
{
    const fsc_table_entry_t *entry;
    const fsc_fsid_t *fsid;

    entry = FS_CONTAINER_OF(node, fsc_table_entry_t, fsid_node);
    fsid = (const fsc_fsid_t *)key;

    return entry->ns->fsid == *fsid;
}

static uint64_t fstable_name_node_hash(
                    const fs_list_head_t *node)
{
    const fsc_table_entry_t *entry;

    entry = FS_CONTAINER_OF(node, fsc_table_entry_t, name_node);

    return fstable_name_hash(entry->ns->name);
}

static uint64_t fstable_name_key_hash(
                    const void *key)
{
    return fstable_name_hash((const char *)key);
}

static bool fstable_name_match(
                    const fs_list_head_t *node,
                    const void *key)
{
    const fsc_table_entry_t *entry;

    entry = FS_CONTAINER_OF(node, fsc_table_entry_t, name_node);

    return strcmp(entry->ns->name, (const char *)key) == 0;
}

static fsc_table_entry_t *fstable_find_fsid_entry(
                            fsc_table_t *table,
                            fsc_fsid_t fsid)
{
    fs_list_head_t *node;

    node = fs_hash_lookup(&table->fsid_index, &fsid);
    if (node == NULL) {
        return NULL;
    }

    return FS_CONTAINER_OF(node, fsc_table_entry_t, fsid_node);
}

static fsc_table_entry_t *fstable_find_name_entry(
                            fsc_table_t *table,
                            const char *name)
{
    fs_list_head_t *node;

    node = fs_hash_lookup(&table->name_index, name);
    if (node == NULL) {
        return NULL;
    }

    return FS_CONTAINER_OF(node, fsc_table_entry_t, name_node);
}

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

fs_error_t fstable_init(
                fsc_table_t *table,
                uint32_t bucket_nr)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter: table=%p, bucket_nr=%u",
                     (void *)table, bucket_nr);

    if (table == NULL) {
        err = fsc_error(FSC_SUB_FSTABLE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: table is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    memset(table, 0, sizeof(*table));

    err = fs_hash_init(&table->fsid_index,
                       bucket_nr,
                       fstable_fsid_node_hash,
                       fstable_fsid_key_hash,
                       fstable_fsid_match);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_hash_init fsid_index failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = fs_hash_init(&table->name_index,
                       bucket_nr,
                       fstable_name_node_hash,
                       fstable_name_key_hash,
                       fstable_name_match);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_hash_init name_index failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        fs_hash_destroy(&table->fsid_index);
        memset(table, 0, sizeof(*table));
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void fstable_deinit(
                fsc_table_t *table,
                fstable_reclaim_fn reclaim)
{
    uint32_t i;
    fs_list_head_t *pos;
    fs_list_head_t *next;
    fsc_table_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: table=%p", (void *)table);

    if (table == NULL) {
        FS_LOG_DUMP_INFO("exit: table is NULL");
        return;
    }

    for (i = 0; i < table->fsid_index.bucket_nr; i++) {
        FS_LIST_FOR_EACH_SAFE(pos, next, &table->fsid_index.buckets[i]) {
            entry = FS_CONTAINER_OF(pos, fsc_table_entry_t, fsid_node);

            fs_hash_remove(&table->fsid_index, &entry->fsid_node);
            fs_hash_remove(&table->name_index, &entry->name_node);

            if (reclaim != NULL) {
                reclaim(entry->ns);
            }

            free(entry);
        }
    }

    fs_hash_destroy(&table->name_index);
    fs_hash_destroy(&table->fsid_index);
    memset(table, 0, sizeof(*table));

    FS_LOG_DUMP_INFO("exit: done");
}

/*
 * ============================================================
 * operations
 * ============================================================
 */

fs_error_t fstable_insert(
                fsc_table_t *table,
                fsc_namespace_t *ns)
{
    fs_error_t err;
    fsc_table_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: table=%p, ns=%p",
                     (void *)table, (void *)ns);

    if ((table == NULL) || (ns == NULL)) {
        err = fsc_error(FSC_SUB_INSERT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: table or ns is NULL, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (!fsc_namespace_is_valid(ns)) {
        err = fsc_error(FSC_SUB_INSERT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid namespace, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (fstable_exists_fsid(table, ns->fsid) ||
        fstable_exists_name(table, ns->name)) {
        err = fsc_error(FSC_SUB_INSERT, FS_ERRNO_EEXIST);
        FS_LOG_DUMP_ERROR("insert failed: namespace already exists, "
                          "fsid=%llu, name=%s, err=%s (0x%x)",
                          (unsigned long long)ns->fsid,
                          ns->name,
                          fs_error_str(err), err);
        return err;
    }

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        err = fsc_error(FSC_SUB_INSERT, FS_ERRNO_ENOMEM);
        FS_LOG_DUMP_ERROR("calloc failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    entry->ns = ns;
    fs_list_init(&entry->fsid_node);
    fs_list_init(&entry->name_node);

    err = fs_hash_insert(&table->fsid_index, &entry->fsid_node);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_hash_insert fsid_index failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        free(entry);
        return err;
    }

    err = fs_hash_insert(&table->name_index, &entry->name_node);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fs_hash_insert name_index failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        fs_hash_remove(&table->fsid_index, &entry->fsid_node);
        free(entry);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

fs_error_t fstable_remove(
                fsc_table_t *table,
                fsc_fsid_t fsid,
                fsc_namespace_t **ns_out)
{
    fs_error_t err;
    fsc_table_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: table=%p, fsid=%llu, ns_out=%p",
                     (void *)table,
                     (unsigned long long)fsid,
                     (void *)ns_out);

    if (table == NULL) {
        err = fsc_error(FSC_SUB_REMOVE, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: table is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    entry = fstable_find_fsid_entry(table, fsid);
    if (entry == NULL) {
        err = fsc_error(FSC_SUB_REMOVE, FS_ERRNO_ENOENT);
        FS_LOG_DUMP_ERROR("remove failed: namespace not found, "
                          "fsid=%llu, err=%s (0x%x)",
                          (unsigned long long)fsid,
                          fs_error_str(err), err);
        return err;
    }

    fs_hash_remove(&table->fsid_index, &entry->fsid_node);
    fs_hash_remove(&table->name_index, &entry->name_node);

    if (ns_out != NULL) {
        *ns_out = entry->ns;
    }

    free(entry);

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

fsc_namespace_t *fstable_lookup_fsid(
                fsc_table_t *table,
                fsc_fsid_t fsid)
{
    fsc_table_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: table=%p, fsid=%llu",
                     (void *)table, (unsigned long long)fsid);

    if (table == NULL) {
        FS_LOG_DUMP_INFO("exit: table is NULL");
        return NULL;
    }

    entry = fstable_find_fsid_entry(table, fsid);
    if (entry == NULL) {
        FS_LOG_DUMP_INFO("exit: not found");
        return NULL;
    }

    FS_LOG_DUMP_INFO("exit: ns=%p", (void *)entry->ns);
    return entry->ns;
}

fsc_namespace_t *fstable_lookup_name(
                fsc_table_t *table,
                const char *name)
{
    fsc_table_entry_t *entry;

    FS_LOG_DUMP_INFO("enter: table=%p, name=%p",
                     (void *)table, (const void *)name);

    if ((table == NULL) || !fsc_namespace_name_is_valid(name)) {
        FS_LOG_DUMP_INFO("exit: invalid param");
        return NULL;
    }

    entry = fstable_find_name_entry(table, name);
    if (entry == NULL) {
        FS_LOG_DUMP_INFO("exit: not found");
        return NULL;
    }

    FS_LOG_DUMP_INFO("exit: ns=%p", (void *)entry->ns);
    return entry->ns;
}

bool fstable_exists_fsid(
                fsc_table_t *table,
                fsc_fsid_t fsid)
{
    bool exists;

    exists = (fstable_lookup_fsid(table, fsid) != NULL);

    FS_LOG_DUMP_INFO("exit: %s", exists ? "true" : "false");
    return exists;
}

bool fstable_exists_name(
                fsc_table_t *table,
                const char *name)
{
    bool exists;

    exists = (fstable_lookup_name(table, name) != NULL);

    FS_LOG_DUMP_INFO("exit: %s", exists ? "true" : "false");
    return exists;
}

/*
 * ============================================================
 * stats
 * ============================================================
 */

uint64_t fstable_count(
                const fsc_table_t *table)
{
    uint64_t count;

    FS_LOG_DUMP_INFO("enter: table=%p", (const void *)table);

    if (table == NULL) {
        count = 0;
    } else {
        count = fs_hash_count(&table->fsid_index);
    }

    FS_LOG_DUMP_INFO("exit: count=%lu", (unsigned long)count);
    return count;
}
