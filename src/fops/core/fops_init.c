#include "fops/include/fops.h"

#include "common/error/fs_sub.h"
#include "common/op/fs_op.h"
#include "fops/internal/fops_internal.h"

fops_context_t g_fops_ctx;
fs_metric_id_t g_fops_metric_ids[FS_OP_MAX];

fs_error_t fops_init(void)
{
    uint32_t op;

    FS_LOG_DUMP_INFO("enter");

    fs_sub_register(FS_MODULE_FOPS, (fs_sub_name_fn)fs_op_name);

    for (op = 0U; op < FS_OP_MAX; op++)
    {
        g_fops_metric_ids[op] = FS_METRIC_ID_INVALID;
        if (op != FS_OP_NONE)
        {
            (void)fs_metrics_register("filesystem", fs_op_name((fs_op_t)op),
                                      false, &g_fops_metric_ids[op]);
        }
    }

    g_fops_ctx.inited = 1U;

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void fops_deinit(void)
{
    uint32_t op;

    FS_LOG_DUMP_INFO("enter");
    g_fops_ctx.inited = 0U;
    for (op = 0U; op < FS_OP_MAX; op++)
    {
        g_fops_metric_ids[op] = FS_METRIC_ID_INVALID;
    }
    FS_LOG_DUMP_INFO("exit: done");
}
