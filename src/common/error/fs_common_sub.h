#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "common/error/fs_error.h"

/*
 * ============================================================
 * COMMON Module sub-error table
 *
 * _(enum_name, string_name)
 *
 * 用于 fs_error_t 的 sub 字段，
 * 标识 COMMON 模块内部发生错误的组件。
 *
 * FS_COMMON_SUB_NONE 仅当错误无法明确归属于
 * 某个组件时作为保底值使用，不应作为默认值。
 * ============================================================
 */

#define FS_COMMON_SUB_TABLE(_)                                                 \
                                                                               \
    _(NONE, "NONE")                                                            \
                                                                               \
    _(HASH, "HASH")                                                            \
    _(LOCK, "LOCK")                                                            \
    _(PATH, "PATH")                                                            \
    _(MEMPOOL, "MEMPOOL")                                                      \
    _(ATOMIC, "ATOMIC")                                                        \
    _(LIST, "LIST")                                                            \
    _(LOG, "LOG")                                                              \
    _(OS, "OS")                                                                \
    _(TRACE, "TRACE")                                                          \
    _(UTILS, "UTILS")                                                          \
    _(ERROR, "ERROR")                                                          \
    _(MODULE, "MODULE")                                                        \
    _(OP, "OP")

/*
 * ============================================================
 * COMMON sub-error enum
 * ============================================================
 */

typedef enum fs_common_sub
{

#define FS_COMMON_SUB_ENUM(name, str) FS_COMMON_SUB_##name,

    FS_COMMON_SUB_TABLE(FS_COMMON_SUB_ENUM)

#undef FS_COMMON_SUB_ENUM

            FS_COMMON_SUB_MAX

} fs_common_sub_t;

/*
 * ============================================================
 * helper
 * ============================================================
 */

static inline const char *fs_common_sub_name(uint32_t sub)
{
    switch (sub)
    {
#define FS_COMMON_SUB_CASE(name, str)                                          \
    case FS_COMMON_SUB_##name:                                                 \
        return str;

        FS_COMMON_SUB_TABLE(FS_COMMON_SUB_CASE)

#undef FS_COMMON_SUB_CASE

    default:
        return "UNKNOWN";
    }
}

static inline bool fs_common_sub_valid(uint32_t sub)
{
    return sub < FS_COMMON_SUB_MAX;
}

/*
 * ============================================================
 * error constructor
 *
 * 构造 COMMON 模块错误码。
 *
 * 参数：
 *      sub     : COMMON 内部组件（fs_common_sub_t）
 *      err     : Linux errno
 * ============================================================
 */

static inline fs_error_t fs_common_error(uint32_t sub, int err)
{
    return FS_ERR(FS_SEV_ERROR, FS_MODULE_COMMON, sub, (uint8_t)err);
}
