#include <string.h>

#include "common/fs_common.h"
#include "object/obj_error.h"
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
                const fuid_t *fuid,
                const obj_handle_t *handle)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter: meta=%p, fuid=%p, handle=%p",
                     (void *)meta, (const void *)fuid,
                     (const void *)handle);

    if (meta == NULL) {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: meta is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (fuid == NULL) {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: fuid is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (!fuid_is_valid(fuid)) {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid fuid, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (handle == NULL) {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: handle is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (!objmeta_handle_valid(handle->len)) {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR(
                "param check failed: invalid handle_len=%u, err=%s (0x%x)",
                (unsigned int)handle->len,
                fs_error_str(err), err);
        return err;
    }

    memset(meta, 0, sizeof(obj_meta_t));

    objkey_from_fuid(&meta->key, fuid);
    meta->state  = OBJ_STATE_INIT;
    meta->handle = *handle;

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void objmeta_reset(
            obj_meta_t *meta)
{
    FS_LOG_DUMP_INFO("enter: meta=%p", (void *)meta);

    if (meta == NULL) {
        return;
    }

    memset(meta,
           0,
           sizeof(obj_meta_t));

    FS_LOG_DUMP_INFO("exit: done");
}

bool objmeta_is_valid(
                const obj_meta_t *meta)
{
    bool valid;

    FS_LOG_DUMP_INFO("enter: meta=%p", (const void *)meta);

    if (meta == NULL) {
        valid = false;
        goto out;
    }

    if (!objkey_is_valid(
            &meta->key)) {
        valid = false;
        goto out;
    }

    if (!objmeta_handle_valid(
            meta->handle.len)) {
        valid = false;
        goto out;
    }

    valid = true;

out:
    FS_LOG_DUMP_INFO("exit: %s",
                     valid ? "true" : "false");
    return valid;
}

bool objmeta_equal(
                const obj_meta_t *lhs,
                const obj_meta_t *rhs)
{
    bool equal;

    FS_LOG_DUMP_INFO("enter: lhs=%p, rhs=%p",
                     (const void *)lhs, (const void *)rhs);

    if ((lhs == NULL) ||
        (rhs == NULL)) {
        equal = false;
        goto out;
    }

    if (!objkey_equal(
            &lhs->key,
            &rhs->key)) {
        equal = false;
        goto out;
    }

    if (lhs->handle.mount_id != rhs->handle.mount_id) {
        equal = false;
        goto out;
    }

    if (lhs->handle.type != rhs->handle.type) {
        equal = false;
        goto out;
    }

    if (lhs->handle.len != rhs->handle.len) {
        equal = false;
        goto out;
    }

    if (memcmp(lhs->handle.data,
               rhs->handle.data,
               lhs->handle.len) != 0) {
        equal = false;
        goto out;
    }

    equal = true;

out:
    FS_LOG_DUMP_INFO("exit: %s",
                     equal ? "true" : "false");
    return equal;
}

void objmeta_dump(
            const obj_meta_t *meta)
{
    uint32_t i;
    uint32_t offset;

    char handle_buf[128];

    FS_LOG_DUMP_INFO("enter: meta=%p", (const void *)meta);

    if (meta == NULL) {

        FS_LOG_DUMP_ERROR("param check failed: meta is NULL");

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
            (unsigned int)objmeta_state(meta));

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

    FS_LOG_DUMP_INFO("exit: done");
}
