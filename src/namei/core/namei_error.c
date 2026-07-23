#include "namei/internal/namei_error.h"

fs_error_t namei_error(namei_sub_t sub, int err)
{
    return FS_ERR(FS_SEV_ERROR, FS_MODULE_NAMEI, sub, (uint8_t)err);
}
