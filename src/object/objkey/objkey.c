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
    assert(key != NULL);
    assert(fuid != NULL);

    key->objectid = fuid->objectid;
    key->gen      = fuid->gen;
}