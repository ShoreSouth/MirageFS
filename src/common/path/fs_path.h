#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "common/error/fs_common_sub.h"

/* =========================================================
 * 路径工具模块（fs_path）
 *
 * ⚠️ 设计原则：
 * 1. 纯字符串处理（不访问 MirageFS）
 * 2. 可用于 log / init / config / 工具层
 * 3. 不涉及 inode / lookup / 权限
 * ========================================================= */

/* =========================
 * 基础路径操作
 * ========================= */

/* 拼接路径：dst = a + "/" + b */
fs_error_t fs_path_join(
    char *dst,
    size_t size,
    const char *a,
    const char *b
);

/* 安全拼接（避免重复 '/'） */
fs_error_t fs_path_join_safe(
    char *dst,
    size_t size,
    const char *a,
    const char *b
);

/* =========================
 * 路径解析
 * ========================= */

/*
 * 路径规范化：
 *   去掉多余 '/', '.', '..'
 *
 * 示例：
 *   "/a//b/./c/../d" -> "/a/b/d"
 */
fs_error_t fs_path_normalize(
    char *dst,
    size_t size,
    const char *src
);

/* =========================
 * 路径拆分
 * ========================= */

/* dirname："/a/b/c" -> "/a/b" */
fs_error_t fs_path_dirname(
    char *dst,
    size_t size,
    const char *path
);

/* basename："/a/b/c" -> "c" */
const char* fs_path_basename(const char *path);

/* =========================
 * 路径属性判断
 * ========================= */

/* 是否绝对路径 */
bool fs_path_is_absolute(const char *path);

/* 是否为空路径 */
bool fs_path_is_empty(const char *path);

/* =========================
 * OS辅助（工具用途）
 * ========================= */

/*
 * 判断路径是否存在（基于宿主OS）
 *
 * ⚠️ 非MirageFS语义
 */
bool fs_path_exists(const char *path);

/*
 * 递归创建目录（mkdir -p）
 *
 * ⚠️ 仅用于：
 *   - 初始化
 *   - 日志目录
 *   - 工具代码
 */
fs_error_t fs_path_mkdir_recursive(
    const char *path,
    int mode
);
