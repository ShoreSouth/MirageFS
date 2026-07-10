#include "namei/include/namei.h"

#include "common/error/fs_sub.h"
#include "namei/internal/namei_sub.h"

fs_error_t namei_init(void)
{
    FS_LOG_DUMP_INFO("enter");

    fs_sub_register(FS_MODULE_NAMEI,
                    (fs_sub_name_fn)namei_sub_name);

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void namei_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");
    FS_LOG_DUMP_INFO("exit: done");
}

void namei_ctx_make(namei_ctx_t *ctx,
                    const fuid_t *root_fuid,
                    const fuid_t *cwd_fuid)
{
    if (ctx == NULL) {
        return;
    }

    fuid_set_invalid(&ctx->root_fuid);
    fuid_set_invalid(&ctx->cwd_fuid);
    ctx->max_symlink_depth = NAMEI_SYMLINK_MAX;

    if (root_fuid != NULL) {
        ctx->root_fuid = *root_fuid;
    }
    if (cwd_fuid != NULL) {
        ctx->cwd_fuid = *cwd_fuid;
    } else if (root_fuid != NULL) {
        ctx->cwd_fuid = *root_fuid;
    }
}
