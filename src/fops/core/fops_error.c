#include "fops/internal/fops_error.h"

fs_error_t fops_error(fs_op_t sub, int err)
{
    return FS_ERR(FS_SEV_ERROR, FS_MODULE_FOPS, sub, (uint8_t)err);
}
