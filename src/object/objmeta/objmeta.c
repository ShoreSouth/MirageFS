#include "object/objmeta/objmeta.h"

#include <string.h>

#include "common/fs_common.h"
#include "object/obj_error.h"

/* ============================================================
 * 内部函数
 * ============================================================ */

static bool objmeta_handle_valid(uint16_t handle_bytes)
{
    if (handle_bytes == 0)
    {
        return false;
    }

    if (handle_bytes > OBJMETA_MAX_HANDLE_SIZE)
    {
        return false;
    }

    return true;
}

/* ============================================================
 * 对外接口
 * ============================================================ */

fs_error_t objmeta_handle_from_lsa(obj_handle_t *out,
                                   const lsa_file_handle_t *handle,
                                   int32_t mount_id)
{
    fs_error_t err;

    if ((out == NULL) || (handle == NULL))
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: null pointer, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (OBJMETA_MAX_HANDLE_SIZE < handle->handle_bytes)
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EOVERFLOW);
        FS_LOG_DUMP_ERROR("handle convert failed: too large, "
                          "bytes=%u, err=%s (0x%x)",
                          handle->handle_bytes, fs_error_str(err), err);
        return err;
    }

    if ((handle->handle_type < 0) ||
        (UINT16_MAX < (uint32_t)handle->handle_type))
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EOVERFLOW);
        FS_LOG_DUMP_ERROR("handle convert failed: invalid type=%d, "
                          "err=%s (0x%x)",
                          handle->handle_type, fs_error_str(err), err);
        return err;
    }

    memset(out, 0, sizeof(*out));

    out->mount_id = mount_id;
    out->type = (uint16_t)handle->handle_type;
    out->len = (uint16_t)handle->handle_bytes;
    memcpy(out->data, handle->data, handle->handle_bytes);

    return FS_OK;
}

fs_error_t objmeta_handle_to_lsa(lsa_file_handle_t *out,
                                 const obj_handle_t *handle)
{
    fs_error_t err;

    if ((out == NULL) || (handle == NULL))
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: null pointer, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (!objmeta_handle_valid(handle->len))
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("handle convert failed: invalid len=%u, "
                          "err=%s (0x%x)",
                          (unsigned int)handle->len, fs_error_str(err), err);
        return err;
    }

    if (LSA_HANDLE_MAX_SIZE < handle->len)
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EOVERFLOW);
        FS_LOG_DUMP_ERROR("handle convert failed: lsa overflow, "
                          "len=%u, err=%s (0x%x)",
                          (unsigned int)handle->len, fs_error_str(err), err);
        return err;
    }

    memset(out, 0, sizeof(*out));

    out->handle_bytes = handle->len;
    out->handle_type = (int32_t)handle->type;
    memcpy(out->data, handle->data, handle->len);

    return FS_OK;
}

fs_error_t objmeta_init(obj_meta_t *meta, const fuid_t *fuid,
                        const obj_handle_t *handle)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter: meta=%p, fuid=%p, handle=%p", (void *)meta,
                     (const void *)fuid, (const void *)handle);

    if (meta == NULL)
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: meta is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (fuid == NULL)
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: fuid is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (!fuid_is_valid(fuid))
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid fuid, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (handle == NULL)
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: handle is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (!objmeta_handle_valid(handle->len))
    {
        err = obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR(
                "param check failed: invalid handle_len=%u, err=%s (0x%x)",
                (unsigned int)handle->len, fs_error_str(err), err);
        return err;
    }

    memset(meta, 0, sizeof(obj_meta_t));

    objkey_from_fuid(&meta->key, fuid);
    meta->handle = *handle;

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void objmeta_deinit(obj_meta_t *meta)
{
    FS_LOG_DUMP_INFO("enter: meta=%p", (void *)meta);

    if (meta == NULL)
    {
        return;
    }

    memset(meta, 0, sizeof(obj_meta_t));

    FS_LOG_DUMP_INFO("exit: done");
}

void objmeta_reset(obj_meta_t *meta)
{
    objmeta_deinit(meta);
}

bool objmeta_is_valid(const obj_meta_t *meta)
{
    bool valid;

    FS_LOG_DUMP_INFO("enter: meta=%p", (const void *)meta);

    if (meta == NULL)
    {
        valid = false;
        goto out;
    }

    if (!objkey_is_valid(&meta->key))
    {
        valid = false;
        goto out;
    }

    if (!objmeta_handle_valid(meta->handle.len))
    {
        valid = false;
        goto out;
    }

    valid = true;

out:
    FS_LOG_DUMP_INFO("exit: %s", valid ? "true" : "false");
    return valid;
}

bool objmeta_equal(const obj_meta_t *lhs, const obj_meta_t *rhs)
{
    bool equal;

    FS_LOG_DUMP_INFO("enter: lhs=%p, rhs=%p", (const void *)lhs,
                     (const void *)rhs);

    if ((lhs == NULL) || (rhs == NULL))
    {
        equal = false;
        goto out;
    }

    if (!objkey_equal(&lhs->key, &rhs->key))
    {
        equal = false;
        goto out;
    }

    if (lhs->handle.mount_id != rhs->handle.mount_id)
    {
        equal = false;
        goto out;
    }

    if (lhs->handle.type != rhs->handle.type)
    {
        equal = false;
        goto out;
    }

    if (lhs->handle.len != rhs->handle.len)
    {
        equal = false;
        goto out;
    }

    if (memcmp(lhs->handle.data, rhs->handle.data, lhs->handle.len) != 0)
    {
        equal = false;
        goto out;
    }

    equal = true;

out:
    FS_LOG_DUMP_INFO("exit: %s", equal ? "true" : "false");
    return equal;
}

void objmeta_dump(const obj_meta_t *meta)
{
    uint32_t i;
    uint32_t offset;

    char handle_buf[128];

    if (meta == NULL)
    {
        FS_LOG_DUMP_INFO("objmeta: null");
        return;
    }

    memset(handle_buf, 0, sizeof(handle_buf));

    offset = 0;

    for (i = 0; i < meta->handle.len; i++)
    {
        offset += snprintf(handle_buf + offset, sizeof(handle_buf) - offset,
                           "%02x", meta->handle.data[i]);

        if (offset >= sizeof(handle_buf))
        {
            break;
        }
    }

    FS_LOG_DUMP_INFO("objmeta: objectid=%lu, gen=%u, mount_id=%d, "
                     "handle_type=%u, handle_bytes=%u, file_handle=%s",
                     (unsigned long)meta->key.objectid,
                     (unsigned int)meta->key.gen, (int)meta->handle.mount_id,
                     (unsigned int)meta->handle.type,
                     (unsigned int)meta->handle.len, handle_buf);
}
