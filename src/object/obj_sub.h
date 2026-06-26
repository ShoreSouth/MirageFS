#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "object/obj_sub_table.h"

/*
 * ============================================================
 * Object Layer sub-error enum
 *
 * 用于 fs_error_t 的 sub 字段，
 * 标识 Object Layer 内部具体执行的操作。
 * ============================================================
 */

typedef enum obj_sub {

#define OBJ_SUB_ENUM(name, str) OBJ_SUB_##name,

    OBJ_SUB_TABLE(OBJ_SUB_ENUM)

#undef OBJ_SUB_ENUM

    OBJ_SUB_MAX

} obj_sub_t;

/*
 * ============================================================
 * helper
 * ============================================================
 */

const char *obj_sub_name(obj_sub_t sub);

bool obj_sub_valid(uint32_t sub);
