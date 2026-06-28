#include <string.h>

#include "common/fs_common.h"
#include "object/objruntime/objruntime.h"

/* ============================================================
 * debug
 * ============================================================ */

void objruntime_dump(
            const obj_runtime_t *rt)
{
    uint32_t i;
    uint32_t offset;

    char handle_buf[128];

    FS_LOG_DUMP_INFO("enter: rt=%p", (const void *)rt);

    if (rt == NULL) {
        FS_LOG_DUMP_ERROR("param check failed: rt is NULL");
        return;
    }

    memset(handle_buf, 0, sizeof(handle_buf));

    offset = 0;

    for (i = 0;
         i < rt->meta.handle.len;
         i++) {

        offset += snprintf(
                    handle_buf + offset,
                    sizeof(handle_buf) - offset,
                    "%02x",
                    rt->meta.handle.data[i]);

        if (offset >= sizeof(handle_buf)) {
            break;
        }
    }

    FS_LOG_DUMP_INFO(
            "========== ObjRuntime ==========");

    FS_LOG_DUMP_INFO(
            "objectid     : %lu",
            (unsigned long)rt->meta.key.objectid);

    FS_LOG_DUMP_INFO(
            "gen          : %u",
            (unsigned int)rt->meta.key.gen);

    FS_LOG_DUMP_INFO(
            "refcnt       : %d",
            (int)rt->refcnt);

    FS_LOG_DUMP_INFO(
            "state        : %u",
            (unsigned int)objruntime_state(rt));

    FS_LOG_DUMP_INFO(
            "mount_id     : %d",
            (int)rt->meta.handle.mount_id);

    FS_LOG_DUMP_INFO(
            "handle_type  : %u",
            (unsigned int)rt->meta.handle.type);

    FS_LOG_DUMP_INFO(
            "handle_bytes : %u",
            (unsigned int)rt->meta.handle.len);

    FS_LOG_DUMP_INFO(
            "file_handle  : %s",
            handle_buf);

    FS_LOG_DUMP_INFO(
            "=============================");

    FS_LOG_DUMP_INFO("exit: done");
}
