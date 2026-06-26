#include "lsa/internal/lsa_internal.h"

#include "common/fs_common.h"
#include "common/error/fs_sub.h"
#include "common/op/fs_op.h"

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

void lsa_init(void)
{
    fs_sub_register(FS_MODULE_LSA,
                    (fs_sub_name_fn)fs_op_name);
}
