#include <assert.h>

#include "object/objkey/objkey.h"

/*
 * ============================================================
 * 转换接口
 * ============================================================
 */

void objkey_from_fuid(
                obj_key_t *key,
                const fuid_t *fuid)
{
    FS_LOG_DUMP_INFO("enter: key=%p, fuid=%p",
                     (void *)key, (const void *)fuid);

    assert(key != NULL);
    assert(fuid != NULL);

    key->objectid = fuid->objectid;
    key->gen      = fuid->gen;

    FS_LOG_DUMP_INFO("exit: key=(%lu,%u)",
                     (unsigned long)key->objectid,
                     (unsigned int)key->gen);
}
