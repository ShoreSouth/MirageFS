#pragma once

#include "common/error/fs_error.h"

#include "object/obj_sub.h"

/*
 * ============================================================
 * Object Layer 错误构造
 * ============================================================
 */

/*
 * 构造 Object Layer 错误码。
 *
 * 参数：
 *      sub     : Object Layer 子操作
 *      err     : Linux errno
 */
fs_error_t obj_error(
                obj_sub_t sub,
                int err);