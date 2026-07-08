#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "common/op/fs_op_table.h"

typedef enum {

#define FS_OP_ENUM(name, str) FS_OP_##name,

    FS_OP_TABLE(FS_OP_ENUM)

#undef FS_OP_ENUM

    FS_OP_MAX

} fs_op_t;

const char *fs_op_name(fs_op_t op);

bool fs_op_valid(uint32_t op);
