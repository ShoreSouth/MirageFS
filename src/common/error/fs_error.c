#include "common/error/fs_error.h"

#include <stdio.h>

#include "common/error/fs_errno.h"
#include "common/error/fs_sub.h"
#include "common/module/fs_module.h"

/*
 * ============================================================
 * 旋转缓冲区
 *
 * 4 个静态缓冲区轮转使用，
 * 在无锁前提下为少量并发调用提供基本安全。
 * ============================================================
 */

#define FS_ERR_STR_BUF_NR   4
#define FS_ERR_STR_BUF_SIZE 256

static _Thread_local char g_fs_err_str_buf
    [FS_ERR_STR_BUF_NR][FS_ERR_STR_BUF_SIZE];
static _Thread_local int  g_fs_err_str_idx;

static char *fs_err_str_buf(void)
{
    char *buf = g_fs_err_str_buf[g_fs_err_str_idx];
    g_fs_err_str_idx = (g_fs_err_str_idx + 1) % FS_ERR_STR_BUF_NR;
    return buf;
}

/*
 * ============================================================
 * severity name
 * ============================================================
 */

const char *fs_severity_name(fs_err_severity_t sev)
{
    switch (sev) {
    case FS_SEV_INFO:  return "INFO";
    case FS_SEV_WARN:  return "WARN";
    case FS_SEV_ERROR: return "ERROR";
    case FS_SEV_FATAL: return "FATAL";
    default:           return "UNKNOWN";
    }
}

/*
 * ============================================================
 * error formatter
 * ============================================================
 */

const char *fs_error_str(fs_error_t err)
{
    fs_err_severity_t sev;
    fs_module_t       module;
    uint32_t          sub;
    fs_errno_t        eno;
    const char       *sub_str;
    const char       *eno_name;
    const char       *eno_desc;
    char             *buf;
    int               off;

    /* FS_OK */
    if (fs_succeeded(err)) {
        return "OK";
    }

    sev     = fs_err_severity(err);
    module  = fs_err_module(err);
    sub     = fs_err_sub(err);
    eno     = fs_err_errno(err);

    sub_str  = fs_sub_name(module, sub);
    eno_name = fs_errno_name(eno);
    eno_desc = fs_errno_desc(eno);

    buf = fs_err_str_buf();
    off = 0;

    /*
     * 格式：
     * [SEVERITY] MODULE::SUB => ERRNO (description)
     */

    off += snprintf(buf + off,
                    FS_ERR_STR_BUF_SIZE - off,
                    "[%s] %s::%s => %s (%s)",
                    fs_severity_name(sev),
                    fs_module_name(module),
                    sub_str  ? sub_str  : "?",
                    eno_name,
                    eno_desc);

    /* 如果格式化溢出，保证以 \0 结尾 */
    if (off >= FS_ERR_STR_BUF_SIZE) {
        buf[FS_ERR_STR_BUF_SIZE - 1] = '\0';
    }

    return buf;
}
