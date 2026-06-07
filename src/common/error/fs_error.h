#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "common/error/fs_errno.h"
#include "common/module/fs_module.h"

/* ============================================================
 * error type
 * ============================================================ */

typedef uint32_t fs_error_t;

/* ============================================================
 * error severity
 * ============================================================ */

typedef enum fs_err_severity {

    FS_SEV_INFO = 0,

    FS_SEV_WARN,

    FS_SEV_ERROR,

    FS_SEV_FATAL,

} fs_err_severity_t;

/* ============================================================
 * error layout (32bit)
 *
 *  31 30 | 29 -------- 20 | 19 -------- 8 | 7 -------- 0
 * -------------------------------------------------------
 * severity|   module id   |   sub error   |    errno
 *
 * severity : 2bit
 * module   : 10bit
 * sub      : 12bit
 * errno    : 8bit
 *
 * ============================================================ */

#define FS_ERR_SEV_SHIFT        30U
#define FS_ERR_MODULE_SHIFT     20U
#define FS_ERR_SUB_SHIFT         8U

#define FS_ERR_SEV_MASK         0x3U
#define FS_ERR_MODULE_MASK      0x3FFU
#define FS_ERR_SUB_MASK         0xFFFU
#define FS_ERRNO_MASK           0xFFU

/* ============================================================
 * standard values
 * ============================================================ */

#define FS_OK ((fs_error_t)0)

#define FS_SUB_NONE  0U

/* ============================================================
 * error builder
 * ============================================================ */

#define FS_ERR(sev, module, sub, err)                       \
    ((fs_error_t)(                                         \
        (((uint32_t)(sev)    & FS_ERR_SEV_MASK)    << FS_ERR_SEV_SHIFT)    | \
        (((uint32_t)(module) & FS_ERR_MODULE_MASK) << FS_ERR_MODULE_SHIFT) | \
        (((uint32_t)(sub)    & FS_ERR_SUB_MASK)    << FS_ERR_SUB_SHIFT)    | \
        (((uint32_t)(err)    & FS_ERRNO_MASK))                              \
    ))

/* ============================================================
 * field extractor
 * ============================================================ */

static inline fs_err_severity_t
fs_err_severity(fs_error_t err)
{
    return (fs_err_severity_t)
        ((err >> FS_ERR_SEV_SHIFT) & FS_ERR_SEV_MASK);
}

static inline fs_module_t
fs_err_module(fs_error_t err)
{
    return (fs_module_t)
        ((err >> FS_ERR_MODULE_SHIFT) & FS_ERR_MODULE_MASK);
}

static inline uint32_t
fs_err_sub(fs_error_t err)
{
    return (uint32_t)
        ((err >> FS_ERR_SUB_SHIFT) & FS_ERR_SUB_MASK);
}

static inline fs_errno_t
fs_err_errno(fs_error_t err)
{
    return (fs_errno_t)
        (err & FS_ERRNO_MASK);
}

/* ============================================================
 * helper
 * ============================================================ */

static inline bool
fs_succeeded(fs_error_t err)
{
    return err == FS_OK;
}

static inline bool
fs_failed(fs_error_t err)
{
    return err != FS_OK;
}

/* ============================================================
 * severity helper
 * ============================================================ */

const char *fs_severity_name(fs_err_severity_t sev);

/* ============================================================
 * error formatter
 * ============================================================ */

const char *fs_error_str(fs_error_t err);