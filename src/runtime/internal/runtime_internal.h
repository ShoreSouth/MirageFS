#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "common/error/fs_sub.h"
#include "config/fs_config.h"
#include "fops/include/fops.h"
#include "fsc/fsc_init.h"
#include "fsc/fsmgr/fsmgr.h"
#include "lsa/include/lsa_api.h"
#include "namei/include/namei.h"
#include "object/object_init.h"
#include "runtime/include/runtime.h"
#include "runtime/internal/runtime_error.h"
#include "runtime/internal/runtime_monitoring.h"

/*
 * Runtime 全局会话状态。
 *
 * 当前 V1 仅支持单进程单会话控制台；后续如果接入 server 或多客户端，
 * 应把该结构拆成 runtime 全局状态和 per-session 状态。
 */
typedef struct runtime_state
{
    bool initialized;
    bool ns_active;

    char namespace_name[FSC_NAMESPACE_NAME_MAX];
    char cwd_path[FS_MAX_PATH_LEN + 1U];

    fuid_t root_fuid;
    fuid_t cwd_fuid;

    fs_trace_ctx_t trace_ctx;

} runtime_state_t;

extern runtime_state_t g_runtime;

fs_error_t runtime_require_initialized(void);
fs_error_t runtime_require_session(void);
fs_error_t runtime_make_ctx(namei_ctx_t *ctx);
fs_error_t runtime_dispatch(fops_args_t *args);
fs_error_t runtime_lookup_fuid(const char *path, fs_flags_t flags,
                               fuid_t *out_fuid);
fs_error_t runtime_lookup_parent_path(const char *path, fs_flags_t flags,
                                      namei_parent_result_t *out);
fs_error_t runtime_update_cwd_path(const char *path);
