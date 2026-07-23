#include "fops/include/fops.h"

#include "common/error/fs_sub.h"
#include "common/op/fs_op.h"
#include "fops/internal/fops_internal.h"

fops_context_t g_fops_ctx;

fs_error_t fops_init(void)
{
    FS_LOG_DUMP_INFO("enter");

    fs_sub_register(FS_MODULE_FOPS, (fs_sub_name_fn)fs_op_name);

    g_fops_ctx.inited = 1U;

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void fops_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");
    g_fops_ctx.inited = 0U;
    FS_LOG_DUMP_INFO("exit: done");
}
