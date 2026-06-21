#include <string.h>

#include "common/fs_common.h"
#include "object/objmeta/objmeta.h"

/* ============================================================
 * 内部函数
 * ============================================================ */

static bool objmeta_handle_valid(
                    uint16_t handle_bytes)
{
    if (handle_bytes == 0) {
        return false;
    }

    if (handle_bytes > OBJMETA_MAX_HANDLE_SIZE) {
        return false;
    }

    return true;
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

bool objmeta_is_valid(
                const obj_meta_t *meta)
{
    if (meta == NULL) {
        return false;
    }

    if (!objkey_valid(
            &meta->key)) {

        return false;
    }

    if (!objmeta_handle_valid(
            meta->handle.len)) {

        return false;
    }

    return true;
}

bool objmeta_equal(
                const obj_meta_t *lhs,
                const obj_meta_t *rhs)
{
    if ((lhs == NULL) ||
        (rhs == NULL)) {

        return false;
    }

    if (!objkey_equal(
            &lhs->key,
            &rhs->key)) {

        return false;
    }

    if (lhs->handle.mount_id != rhs->handle.mount_id) {
        return false;
    }

    if (lhs->handle.type != rhs->handle.type) {
        return false;
    }

    if (lhs->handle.len != rhs->handle.len) {
        return false;
    }

    if (memcmp(lhs->handle.data,
               rhs->handle.data,
               lhs->handle.len) != 0) {

        return false;
    }

    return true;
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
            (unsigned long)meta->key.objectid);

    FS_LOG_DUMP_INFO(
            "gen          : %u",
            (unsigned int)meta->key.gen);

    FS_LOG_DUMP_INFO(
            "refcnt       : %d",
            (int)meta->refcnt);

    FS_LOG_DUMP_INFO(
            "state        : %u",
            (unsigned int)meta->state);

    FS_LOG_DUMP_INFO(
            "mount_id     : %d",
            (int)meta->handle.mount_id);

    FS_LOG_DUMP_INFO(
            "handle_type  : %u",
            (unsigned int)meta->handle.type);

    FS_LOG_DUMP_INFO(
            "handle_bytes : %u",
            (unsigned int)meta->handle.len);

    FS_LOG_DUMP_INFO(
            "file_handle  : %s",
            handle_buf);

    FS_LOG_DUMP_INFO(
            "=============================");
}