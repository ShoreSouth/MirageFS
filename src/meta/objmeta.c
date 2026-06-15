#include <string.h>

#include "common/fs_common.h"
#include "meta/objmeta.h"

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
                ObjMeta_t *meta,
                const objkey_t *key,
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
           sizeof(ObjMeta_t));

    meta->key          = *key;
    meta->mount_id     = mount_id;
    meta->handle_type  = handle_type;
    meta->handle_bytes = handle_bytes;

    memcpy(meta->file_handle,
           file_handle,
           handle_bytes);

    return 0;
}

void objmeta_reset(
            ObjMeta_t *meta)
{
    if (meta == NULL) {
        return;
    }

    memset(meta,
           0,
           sizeof(ObjMeta_t));
}

int32_t objmeta_is_valid(
                const ObjMeta_t *meta)
{
    if (meta == NULL) {
        return 0;
    }

    if (!objkey_valid(
            &meta->key)) {

        return 0;
    }

    if (!objmeta_handle_valid(
            meta->handle_bytes)) {

        return 0;
    }

    return 1;
}

int32_t objmeta_equal(
                const ObjMeta_t *lhs,
                const ObjMeta_t *rhs)
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

    if (lhs->mount_id != rhs->mount_id) {
        return 0;
    }

    if (lhs->handle_type != rhs->handle_type) {
        return 0;
    }

    if (lhs->handle_bytes != rhs->handle_bytes) {
        return 0;
    }

    if (memcmp(lhs->file_handle,
               rhs->file_handle,
               lhs->handle_bytes) != 0) {

        return 0;
    }

    return 1;
}

void objmeta_dump(
            const ObjMeta_t *meta)
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
         i < meta->handle_bytes;
         i++) {

        offset += snprintf(
                    handle_buf + offset,
                    sizeof(handle_buf) - offset,
                    "%02x",
                    meta->file_handle[i]);

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
            "mount_id     : %d",
            meta->mount_id);

    FS_LOG_DUMP_INFO(
            "handle_type  : %u",
            meta->handle_type);

    FS_LOG_DUMP_INFO(
            "handle_bytes : %u",
            meta->handle_bytes);

    FS_LOG_DUMP_INFO(
            "file_handle  : %s",
            handle_buf);

    FS_LOG_DUMP_INFO(
            "=============================");
}