#pragma once

#include "common/log/fs_log.h"

#include <stdlib.h>
#include <assert.h>

/* ============================================================
 *  配置开关
 * ============================================================ */

/*
 * 默认行为：
 * Debug 版本：启用 assert
 * Release 版本：关闭 assert
 */
#ifndef FS_ENABLE_ASSERT
#ifdef NDEBUG
#define FS_ENABLE_ASSERT 0
#else
#define FS_ENABLE_ASSERT 1
#endif
#endif

/* ============================================================
 *  基础断言
 * ============================================================ */

#if FS_ENABLE_ASSERT

#define FS_ASSERT(cond)                                                        \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            FS_LOG_DUMP_ERROR("ASSERT FAIL: (%s)", #cond);                     \
            abort();                                                           \
        }                                                                      \
    } while (0)

#else

#define FS_ASSERT(cond)                                                        \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            FS_LOG_DUMP_ERROR("ASSERT FAIL (ignored): (%s)", #cond);           \
        }                                                                      \
    } while (0)

#endif

/* ============================================================
 *  带信息断言
 * ============================================================ */

#define FS_ASSERT_MSG(cond, fmt, ...)                                          \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            FS_LOG_DUMP_ERROR("ASSERT FAIL: (%s) " fmt, #cond, ##__VA_ARGS__); \
            FS_ASSERT(cond);                                                   \
        }                                                                      \
    } while (0)

/* ============================================================
 *  返回类断言
 * ============================================================ */

#define FS_ASSERT_RET(cond, ret)                                               \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            FS_LOG_DUMP_ERROR("ASSERT FAIL: (%s), return %d", #cond,           \
                              (int)(ret));                                     \
            return (ret);                                                      \
        }                                                                      \
    } while (0)

#define FS_ASSERT_RET_VOID(cond)                                               \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            FS_LOG_DUMP_ERROR("ASSERT FAIL: (%s)", #cond);                     \
            return;                                                            \
        }                                                                      \
    } while (0)

/* ============================================================
 *  goto 风格
 * ============================================================ */

#define FS_ASSERT_GOTO(cond, label)                                            \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            FS_LOG_DUMP_ERROR("ASSERT FAIL: (%s), goto %s", #cond, #label);    \
            goto label;                                                        \
        }                                                                      \
    } while (0)

/* ============================================================
 *  unlikely 优化（可选）
 * ============================================================ */

#ifndef FS_UNLIKELY
#define FS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#define FS_ASSERT_UNLIKELY(cond)                                               \
    do                                                                         \
    {                                                                          \
        if (FS_UNLIKELY(!(cond)))                                              \
        {                                                                      \
            FS_LOG_DUMP_ERROR("ASSERT FAIL: (%s)", #cond);                     \
            FS_ASSERT(cond);                                                   \
        }                                                                      \
    } while (0)
