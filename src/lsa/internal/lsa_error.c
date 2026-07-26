#include "lsa/internal/lsa_internal.h"

#include "common/fs_common.h"
#include "common/error/fs_sub.h"
#include "common/op/fs_op.h"

fs_metric_id_t g_lsa_metric_ids[FS_OP_MAX];

fs_error_t lsa_error(fs_op_t sub, int err)
{
    return FS_ERR(FS_SEV_ERROR, FS_MODULE_LSA, sub, (uint8_t)err);
}

void lsa_init(void)
{
    uint32_t op;

    fs_sub_register(FS_MODULE_LSA, (fs_sub_name_fn)fs_op_name);
    for (op = 0U; op < FS_OP_MAX; op++)
    {
        g_lsa_metric_ids[op] = FS_METRIC_ID_INVALID;
        if (op != FS_OP_NONE)
        {
            (void)fs_metrics_register("lsa", fs_op_name((fs_op_t)op), true,
                                      &g_lsa_metric_ids[op]);
        }
    }
}
