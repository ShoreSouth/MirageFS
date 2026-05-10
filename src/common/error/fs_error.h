#pragma once

#include <stdint.h>
#include <errno.h>

/*
 * ============================================
 * 1. 基础类型
 * ============================================
 */
typedef uint32_t fs_error_t;

/*
 * ============================================
 * 2. 错误码位域定义（32bit）
 *
 * | 31 30 | 29 -------- 20 | 19 -------- 8 | 7 -------- 0 |
 * | level | module id      | sub error     | errno        |
 * ============================================
 */

/* bit offset */
#define FS_ERR_LEVEL_SHIFT    30
#define FS_ERR_MODULE_SHIFT   20
#define FS_ERR_SUB_SHIFT      8

/* bit mask */
#define FS_ERR_LEVEL_MASK     0x3
#define FS_ERR_MODULE_MASK    0x3FF
#define FS_ERR_SUB_MASK       0xFFF
#define FS_ERR_ERRNO_MASK     0xFF

/*
 * ============================================
 * 3. 错误级别定义
 * ============================================
 */
typedef enum {
    FS_ERR_LEVEL_OK = 0,        /* 成功 */
    FS_ERR_LEVEL_INFO = 1,      /* 提示/非错误 */
    FS_ERR_LEVEL_ERROR = 2,     /* 可恢复错误 */
    FS_ERR_LEVEL_FATAL = 3      /* 不可恢复错误 */
} fs_err_level_t;

/*
 * ============================================
 * 4. 模块ID定义（统一注册）
 * ============================================
 */
typedef enum {
    FS_MODULE_COMMON = 1,
    FS_MODULE_VFS = 2,
    FS_MODULE_FS = 3,
    FS_MODULE_LSA = 4,
    FS_MODULE_CACHE = 5,
    FS_MODULE_META = 6,
    FS_MODULE_STORAGE = 7,

    /* 预留扩展 */
    FS_MODULE_MAX = 1023
} fs_module_t;

/*
 * ============================================
 * 5. 错误码构造
 * ============================================
 */
#define FS_ERR(level, module, sub, err) \
    ( ((fs_error_t)(level & FS_ERR_LEVEL_MASK) << FS_ERR_LEVEL_SHIFT) | \
      ((fs_error_t)(module & FS_ERR_MODULE_MASK) << FS_ERR_MODULE_SHIFT) | \
      ((fs_error_t)(sub & FS_ERR_SUB_MASK) << FS_ERR_SUB_SHIFT) | \
      ((fs_error_t)(err & FS_ERR_ERRNO_MASK)) )

/*
 * ============================================
 * 6. 常用快捷宏
 * ============================================
 */

/* 成功 */
#define FS_OK  ((fs_error_t)0)

/* 通用错误 */
#define FS_ERR_COMMON(sub, err) \
    FS_ERR(FS_ERR_LEVEL_ERROR, FS_MODULE_COMMON, sub, err)

/* fatal */
#define FS_ERR_FATAL(module, sub, err) \
    FS_ERR(FS_ERR_LEVEL_FATAL, module, sub, err)

/* 从 errno 构造 */
#define FS_ERR_FROM_ERRNO(module, err) \
    FS_ERR(FS_ERR_LEVEL_ERROR, module, 0, err)

/*
 * ============================================
 * 7. 解码宏
 * ============================================
 */

#define FS_ERR_GET_LEVEL(e) \
    (((e) >> FS_ERR_LEVEL_SHIFT) & FS_ERR_LEVEL_MASK)

#define FS_ERR_GET_MODULE(e) \
    (((e) >> FS_ERR_MODULE_SHIFT) & FS_ERR_MODULE_MASK)

#define FS_ERR_GET_SUB(e) \
    (((e) >> FS_ERR_SUB_SHIFT) & FS_ERR_SUB_MASK)

#define FS_ERR_GET_ERRNO(e) \
    ((e) & FS_ERR_ERRNO_MASK)

/*
 * ============================================
 * 8. 判断宏
 * ============================================
 */

#define FS_IS_OK(e) \
    ((e) == FS_OK)

#define FS_IS_ERROR(e) \
    (FS_ERR_GET_LEVEL(e) == FS_ERR_LEVEL_ERROR)

#define FS_IS_FATAL(e) \
    (FS_ERR_GET_LEVEL(e) == FS_ERR_LEVEL_FATAL)

/* 是否可重试 */
#define FS_IS_RETRYABLE(e) \
    (FS_ERR_GET_LEVEL(e) == FS_ERR_LEVEL_ERROR)

/* 是否是某模块错误 */
#define FS_IS_MODULE(e, m) \
    (FS_ERR_GET_MODULE(e) == (m))

/*
 * ============================================
 * 9. errno 映射（关键！）
 * ============================================
 */

/* 转为 Linux errno（用于对外接口） */
static inline int fs_err_to_errno(fs_error_t e)
{
    if (FS_IS_OK(e))
        return 0;

    return -(int)FS_ERR_GET_ERRNO(e);
}


/*
 * ============================================
 * 10. 常用错误（COMMON模块）
 * ============================================
 */

enum {
    FS_ERR_SUB_UNKNOWN = 1,
    FS_ERR_SUB_INVALID_ARG,
    FS_ERR_SUB_NO_MEMORY,
    FS_ERR_SUB_NOT_FOUND,
    FS_ERR_SUB_EXIST,
    FS_ERR_SUB_PERMISSION,
};

/* 常用封装 */
#define FS_EINVAL \
    FS_ERR_COMMON(FS_ERR_SUB_INVALID_ARG, EINVAL)

#define FS_ENOMEM \
    FS_ERR_COMMON(FS_ERR_SUB_NO_MEMORY, ENOMEM)

#define FS_ENOENT \
    FS_ERR_COMMON(FS_ERR_SUB_NOT_FOUND, ENOENT)

#define FS_EEXIST \
    FS_ERR_COMMON(FS_ERR_SUB_EXIST, EEXIST)

#define FS_EPERM \
    FS_ERR_COMMON(FS_ERR_SUB_PERMISSION, EPERM)

/*
 * ============================================
 * 11. 调试辅助（可选）
 * ============================================
 */

static inline const char* fs_err_level_str(fs_error_t e)
{
    switch (FS_ERR_GET_LEVEL(e)) {
        case FS_ERR_LEVEL_OK: return "OK";
        case FS_ERR_LEVEL_INFO: return "INFO";
        case FS_ERR_LEVEL_ERROR: return "ERROR";
        case FS_ERR_LEVEL_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

static inline const char* fs_module_str(uint32_t m)
{
    switch (m) {
        case FS_MODULE_COMMON: return "COMMON";
        case FS_MODULE_VFS: return "VFS";
        case FS_MODULE_FS: return "FS";
        case FS_MODULE_LSA: return "LSA";
        case FS_MODULE_CACHE: return "CACHE";
        case FS_MODULE_META: return "META";
        case FS_MODULE_STORAGE: return "STORAGE";
        default: return "UNKNOWN";
    }
}
