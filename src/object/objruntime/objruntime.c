#include "object/objruntime/objruntime.h"

#include <string.h>

#include "common/fs_common.h"

/* ============================================================
 * debug
 * ============================================================ */

void objruntime_dump(
            const obj_runtime_t *rt)
{
    uint32_t i;
    uint32_t offset;

    char handle_buf[128];

    if (rt == NULL) {
        FS_LOG_DUMP_INFO("objruntime: null");
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

    FS_LOG_DUMP_INFO("objruntime: objectid=%lu, gen=%u, refcnt=%d, "
                     "state=%u, mount_id=%d, handle_type=%u, "
                     "handle_bytes=%u, file_handle=%s",
                     (unsigned long)rt->meta.key.objectid,
                     (unsigned int)rt->meta.key.gen,
                     (int)fs_atomic32_load(&rt->refcnt),
                     (unsigned int)objruntime_state(rt),
                     (int)rt->meta.handle.mount_id,
                     (unsigned int)rt->meta.handle.type,
                     (unsigned int)rt->meta.handle.len,
                     handle_buf);
}
