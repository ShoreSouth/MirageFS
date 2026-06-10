#include "lsa/internal/lsa_internal.h"

#include "common/fs_common.h"

fs_error_t lsa_error(
                fs_op_t sub,
                int err)
{
    return FS_ERR(
                FS_SEV_ERROR,
                FS_MODULE_LSA,
                sub,
                (uint8_t)err);
}
