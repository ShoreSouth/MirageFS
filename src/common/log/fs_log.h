#pragma once

#include <stdint.h>

/* =========================
 * 日志级别
 * ========================= */

typedef enum
{
    FS_LOG_DEBUG = 0,
    FS_LOG_INFO,
    FS_LOG_WARN,
    FS_LOG_ERROR,
} fs_log_level_t;

/* =========================
 * 初始化
 * ========================= */

/*
 * base_dir: 日志根目录（必须已存在）
 * level   : 最低日志级别
 */
void fs_log_init(const char *base_dir, fs_log_level_t level);

/* =========================
 * 核心输出接口
 * ========================= */

void fs_log_write(fs_log_level_t level, const char *file, int line,
                  const char *func, const char *fmt, ...);

/* =========================
 * 宏封装（推荐使用）
 * ========================= */

#define FS_LOG_DUMP_DEBUG(fmt, ...)                                            \
    fs_log_write(FS_LOG_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define FS_LOG_DUMP_INFO(fmt, ...)                                             \
    fs_log_write(FS_LOG_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define FS_LOG_DUMP_WARN(fmt, ...)                                             \
    fs_log_write(FS_LOG_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define FS_LOG_DUMP_ERROR(fmt, ...)                                            \
    fs_log_write(FS_LOG_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
