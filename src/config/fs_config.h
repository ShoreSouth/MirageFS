/**
 * @file fs_config.h
 * @brief MirageFS 全局配置管理
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "common/macros/fs_defs.h"
#include "common/metrics/fs_metrics.h"

#define FS_METRICS_REPORT_DIR_MAX 255U
#define FS_METRICS_DEFAULT_SAMPLE_INTERVAL_MS 1000U
#define FS_METRICS_DEFAULT_HISTORY_CAPACITY 300U

typedef struct FsMetricsConfig
{
    fs_metrics_mode_t mode;
    uint32_t sample_interval_ms;
    uint32_t history_capacity;
    bool report_enable;
    bool report_json_enable;
    bool report_html_enable;
    char report_directory[FS_METRICS_REPORT_DIR_MAX + 1U];
} FsMetricsConfig_t;

/* ============================================================
 * 全局配置结构
 * ============================================================ */

typedef struct FsConfig
{
    /*
     * 内存池总大小
     */
    uint64_t mempool_size;

    /*
     * worker 线程数量
     */
    uint32_t worker_nr;

    /*
     * 是否开启 trace
     */
    bool trace_enable;

    /*
     * 是否开启 debug
     */
    bool debug_enable;

    /*
     * 性能统计与退出报告配置
     */
    FsMetricsConfig_t metrics;
} FsConfig_t;

/* ============================================================
 * 全局配置对象
 * ============================================================ */

extern FsConfig_t g_fs_config;

/* ============================================================
 * 配置接口
 * ============================================================ */

/**
 * @brief 初始化全局配置
 *
 * 当前版本：
 *   - 仅加载默认配置
 */
void fs_config_init(void);

/**
 * @brief 打印当前配置
 */
void fs_config_dump(void);
