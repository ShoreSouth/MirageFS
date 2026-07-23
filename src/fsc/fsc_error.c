#include "fsc/fsc_error.h"

/*
 * ============================================================
 * FSC error builder
 * ============================================================
 */

fs_error_t fsc_error(fsc_sub_t sub, int err)
{
    return FS_ERR(FS_SEV_ERROR, FS_MODULE_FSC, sub, (uint8_t)err);
}
