#include "object/obj_error.h"

/*
 * ============================================================
 * Object Layer 错误构造
 * ============================================================
 */

fs_error_t obj_error(obj_sub_t sub, int err)
{
    return FS_ERR(FS_SEV_ERROR, FS_MODULE_OBJECT, sub, (uint8_t)err);
}
