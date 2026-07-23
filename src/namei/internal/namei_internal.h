#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "common/fs_common.h"
#include "fops/include/fops_types.h"
#include "namei/include/namei_types.h"

typedef struct namei_walk_result
{
    fuid_t fuid;
    fops_attr_t attr;
    bool has_attr;

} namei_walk_result_t;

fs_error_t namei_ctx_check(const namei_ctx_t *ctx);

fs_error_t namei_walk(const namei_ctx_t *ctx, const char *path,
                      fs_flags_t flags, namei_walk_result_t *out);

bool namei_path_is_absolute(const char *path);
bool namei_path_has_trailing_slash(const char *path);
