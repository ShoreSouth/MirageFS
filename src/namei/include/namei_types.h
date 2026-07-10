#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "fops/include/fops_types.h"
#include "object/fuid/fuid.h"

#define NAMEI_SYMLINK_MAX 40U

typedef struct namei_ctx {

    fuid_t root_fuid;
    fuid_t cwd_fuid;

    uint32_t max_symlink_depth;

} namei_ctx_t;

typedef struct namei_parent_result {

    fuid_t parent_fuid;
    char name[FS_MAX_NAME_LEN + 1U];

} namei_parent_result_t;
