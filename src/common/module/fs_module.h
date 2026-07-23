#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/module/fs_module_table.h"

/* ============================================================
 * module id
 * ============================================================ */

typedef enum fs_module
{

    FS_MODULE_NONE = 0,

#define FS_MODULE_ENUM(name, str) FS_MODULE_##name,

    FS_MODULE_TABLE(FS_MODULE_ENUM)

#undef FS_MODULE_ENUM

            FS_MODULE_MAX

} fs_module_t;

/* ============================================================
 * helper
 * ============================================================ */

const char *fs_module_name(fs_module_t module);

bool fs_module_valid(uint32_t module);
