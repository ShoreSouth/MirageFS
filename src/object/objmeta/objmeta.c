#include <string.h>

#include "common/fs_common.h"
#include "object/objmeta/objmeta.h"

/* ============================================================
 * 内部函数
 * ============================================================ */

static int32_t objmeta_handle_valid(
                    uint16_t handle_bytes)
{
    if (handle_bytes == 0) {
        return 0;
    }

    if (handle_bytes > OBJMETA_MAX_HANDLE_SIZE) {
        return 0;
    }

    return 1;
}

/* ============================================================
 * 对外接口
 * ============================================================ */

int32_t objmeta_init(
                obj_meta_t *meta,
                const obj_key_t *key,
                int32_t mount_id,
                uint16_t handle_type,
                uint16_t handle_bytes,
                const uint8_t *file_handle)
{
    if (meta == NULL) {
        FS_LOG_DUMP_ERROR("meta is NULL");
        return -1;
    }

    if (key == NULL) {
        FS_LOG_DUMP_ERROR("key is NULL");
        return -1;
    }

    if (!objkey_valid(key)) {
        FS_LOG_DUMP_ERROR("invalid key");
        return -1;
    }

    if (!objmeta_handle_valid(handle_bytes)) {

        FS_LOG_DUMP_ERROR(
                "invalid handle_bytes=%u",
                handle_bytes);

        return -1;
    }

    if (file_handle == NULL) {

        FS_LOG_DUMP_ERROR(
                "file_handle is NULL");

        return -1;
    }

    memset(meta,
           0,
           sizeof(obj_meta_t));

    meta->key             = *key;
    meta->state           = OBJ_STATE_INIT;
    meta->handle.mount_id = mount_id;
    meta->handle.type     = handle_type;
    meta->handle.len      = handle_bytes;

    memcpy(meta->handle.data,
           file_handle,
           handle_bytes);

    return 0;
}

void objmeta_reset(
            obj_meta_t *meta)
{
    if (meta == NULL) {
        return;
    }

    memset(meta,
           0,
           sizeof(obj_meta_t));
}

int32_t objmeta_is_valid(
                const obj_meta_t *meta)
{
    if (meta == NULL) {
        return 0;
    }

    if (!objkey_valid(
            &meta->key)) {

        return 0;
    }

    if (!objmeta_handle_valid(
            meta->handle.len)) {

        return 0;
    }

    return 1;
}

int32_t objmeta_equal(
                const obj_meta_t *lhs,
                const obj_meta_t *rhs)
{
    if ((lhs == NULL) ||
        (rhs == NULL)) {

        return 0;
    }

    if (!objkey_equal(
            &lhs->key,
            &rhs->key)) {

        return 0;
    }

    if (lhs->handle.mount_id != rhs->handle.mount_id) {
        return 0;
    }

    if (lhs->handle.type != rhs->handle.type) {
        return 0;
    }

    if (lhs->handle.len != rhs->handle.len) {
        return 0;
    }

    if (memcmp(lhs->handle.data,
               rhs->handle.data,
               lhs->handle.len) != 0) {

        return 0;
    }

    return 1;
}

void objmeta_dump(
            const obj_meta_t *meta)
{
    uint32_t i;
    uint32_t offset;

    char handle_buf[128];

    if (meta == NULL) {

        FS_LOG_DUMP_ERROR(
                "meta is NULL");

        return;
    }

    memset(handle_buf,
           0,
           sizeof(handle_buf));

    offset = 0;

    for (i = 0;
         i < meta->handle.len;
         i++) {

        offset += snprintf(
                    handle_buf + offset,
                    sizeof(handle_buf) - offset,
                    "%02x",
                    meta->handle.data[i]);

        if (offset >= sizeof(handle_buf)) {
            break;
        }
    }

    FS_LOG_DUMP_INFO(
            "========== ObjMeta ==========");

    FS_LOG_DUMP_INFO(
            "objectid     : %lu",
            meta->key.objectid);

    FS_LOG_DUMP_INFO(
            "gen          : %u",
            meta->key.gen);

    FS_LOG_DUMP_INFO(
            "refcnt       : %d",
            meta->refcnt);

    FS_LOG_DUMP_INFO(
            "state        : %u",
            meta->state);

    FS_LOG_DUMP_INFO(
            "mount_id     : %d",
            meta->handle.mount_id);

    FS_LOG_DUMP_INFO(
            "handle_type  : %u",
            meta->handle.type);

    FS_LOG_DUMP_INFO(
            "handle_bytes : %u",
            meta->handle.len);

    FS_LOG_DUMP_INFO(
            "file_handle  : %s",
            handle_buf);

    FS_LOG_DUMP_INFO(
            "=============================");
}