#include "common/log/fs_log.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common/os/fs_os.h"
#include "common/path/fs_path.h"
#include "common/trace/fs_trace.h"

/* =========================
 * 全局配置
 * ========================= */

static char g_log_base_dir[256] = "./output/log";
static fs_log_level_t g_log_level = FS_LOG_DEBUG;

/* =========================
 * TLS
 * ========================= */

static __thread FILE *tls_fp = NULL;

/* =========================
 * 内部工具
 * ========================= */

static const char *fs_log_level_str(fs_log_level_t level)
{
    switch (level)
    {
    case FS_LOG_DEBUG:
        return "DEBUG";
    case FS_LOG_INFO:
        return "INFO";
    case FS_LOG_WARN:
        return "WARN";
    case FS_LOG_ERROR:
        return "ERROR";
    default:
        return "UNK";
    }
}

/* 初始化线程日志文件（lazy） */
static FILE *fs_log_get_fp(void)
{
    if (tls_fp)
        return tls_fp;

    const char *proc = fs_get_process_name();
    const char *thread = fs_get_thread_name();

    char dir_path[512];
    char file_path[512];

    /* base/proc */
    fs_path_join_safe(dir_path, sizeof(dir_path), g_log_base_dir, proc);

    /* makir -p */
    fs_path_mkdir_recursive(dir_path, 0777);

    /* base/proc/thread.log */
    char file_name[512];
    snprintf(file_name, sizeof(file_name), "%s.log", thread);

    fs_path_join_safe(file_path, sizeof(file_path), dir_path, file_name);

    tls_fp = fopen(file_path, "a"); /* 追加写 */

    if (!tls_fp)
        tls_fp = stderr;

    return tls_fp;
}

/* =========================
 * 初始化
 * ========================= */

void fs_log_init(const char *base_dir, fs_log_level_t level)
{
    if (base_dir && base_dir[0] != '\0')
    {
        snprintf(g_log_base_dir, sizeof(g_log_base_dir), "%s", base_dir);
    }

    g_log_level = level;
}

/* =========================
 * 核心输出
 * ========================= */

void fs_log_write(fs_log_level_t level, const char *file, int line,
                  const char *func, const char *fmt, ...)
{
    if (level < g_log_level)
        return;

    FILE *fp = fs_log_get_fp();

    fs_trace_ctx_t *ctx = FS_TRACE_GET();

    /* header */
    fprintf(fp, "[%s][%s][trace=0x%lu span=0x%lu][%s:%d %s] ",
            fs_log_level_str(level), fs_time_str(), ctx->trace_id, ctx->span_id,
            file, line, func);

    /* body */
    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);

    fprintf(fp, "\n");
    fflush(fp);
}
