/**
 * @file fs_macros.h
 * @brief MirageFS 通用宏定义 偏"工具 " 、"语法增强"、"通用能力"
 */

#pragma once

#include <stddef.h>

/* ============================================================
 *  基础宏
 * ============================================================ */

/* 数组大小 */
#define FS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* ============================================================
 *  MIN / MAX
 * ============================================================ */

#define FS_MIN(a, b) \
    ({ __typeof__(a) _a = (a); \
       __typeof__(b) _b = (b); \
       _a < _b ? _a : _b; })

#define FS_MAX(a, b) \
    ({ __typeof__(a) _a = (a); \
       __typeof__(b) _b = (b); \
       _a > _b ? _a : _b; })

/* ============================================================
 * 对齐相关
 * ============================================================ */

#define FS_ALIGN_UP(x, a)      (((x) + ((a) - 1)) & ~((a) - 1))

#define FS_ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))

#define FS_IS_ALIGNED(x, a)    (((x) & ((a) - 1)) == 0)

/* ============================================================
 *  位操作
 * ============================================================ */

#define FS_BIT(n)              (1UL << (n))
#define FS_BIT_ULL(n)          (1ULL << (n))

#define FS_SET_FLAG(v, f)      ((v) |= (f))
#define FS_CLR_FLAG(v, f)      ((v) &= ~(f))
#define FS_HAS_FLAG(v, f)      ((v) & (f))

/* ============================================================
 *  container_of
 * ============================================================ */

#define FS_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* ============================================================
 *  likely / unlikely
 * ============================================================ */

#ifndef FS_LIKELY
#define FS_LIKELY(x)   __builtin_expect(!!(x), 1)
#endif

#ifndef FS_UNLIKELY
#define FS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

/* ============================================================
 *  静态断言（编译期检查）
 * ============================================================ */

#define FS_STATIC_ASSERT(cond, msg) \
    _Static_assert(cond, msg)

/* ============================================================
 *  unused / fallthrough（编译警告控制）
 * ============================================================ */

#define FS_UNUSED(x) (void)(x)

#if defined(__GNUC__) || defined(__clang__)
#define FS_FALLTHROUGH __attribute__((fallthrough))
#else
#define FS_FALLTHROUGH
#endif

/* ============================================================
 *  类型安全检查（调试用，可选）
 * ============================================================ */

#define FS_TYPE_CHECK(a, b) ((void)sizeof((typeof(a) *)1 == (typeof(b) *)1))
