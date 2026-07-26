/**
 * @file fs_config.c
 * @brief MirageFS 全局配置管理实现
 */

#include "fs_config.h"

#include <stdio.h>
#include <string.h>

#include "common/fs_common.h"

/* ============================================================
 * 全局配置对象
 * ============================================================ */

FsConfig_t g_fs_config;

/* ============================================================
 * 内部函数
 * ============================================================ */

/**
 * @brief 加载默认配置
 */
static void fs_config_load_default(void)
{
    g_fs_config.mempool_size = FS_DEFAULT_MEMPOOL_SIZE;

    g_fs_config.worker_nr = FS_DEFAULT_WORKER_NR;

    g_fs_config.trace_enable = true;

    g_fs_config.debug_enable = true;

    g_fs_config.metrics.mode = FS_METRICS_CORE;
    g_fs_config.metrics.sample_interval_ms =
            FS_METRICS_DEFAULT_SAMPLE_INTERVAL_MS;
    g_fs_config.metrics.history_capacity =
            FS_METRICS_DEFAULT_HISTORY_CAPACITY;
#ifdef FS_TEST_FAULTS
    g_fs_config.metrics.report_enable = false;
#else
    g_fs_config.metrics.report_enable = true;
#endif
    g_fs_config.metrics.report_json_enable = true;
    g_fs_config.metrics.report_html_enable = true;
    (void)snprintf(g_fs_config.metrics.report_directory,
                   sizeof(g_fs_config.metrics.report_directory), "%s",
                   "./reports");
}

/* ============================================================
 * 对外接口
 * ============================================================ */

void fs_config_init(void)
{
    fs_config_load_default();
}

void fs_config_dump(void)
{
    FS_LOG_DUMP_INFO("\n");
    FS_LOG_DUMP_INFO("========== MirageFS Config ==========\n");

    FS_LOG_DUMP_INFO("mempool_size : %lu MB\n",
                     g_fs_config.mempool_size / FS_MB);

    FS_LOG_DUMP_INFO("worker_nr    : %u\n", g_fs_config.worker_nr);

    FS_LOG_DUMP_INFO("trace_enable : %s\n",
                     g_fs_config.trace_enable ? "true" : "false");

    FS_LOG_DUMP_INFO("debug_enable : %s\n",
                     g_fs_config.debug_enable ? "true" : "false");

    FS_LOG_DUMP_INFO("metrics_mode : %s\n",
                     fs_metrics_mode_to_str(g_fs_config.metrics.mode));
    FS_LOG_DUMP_INFO("metrics_report : %s\n",
                     g_fs_config.metrics.report_enable ? "true" : "false");
    FS_LOG_DUMP_INFO("metrics_report_dir : %s\n",
                     g_fs_config.metrics.report_directory);

    FS_LOG_DUMP_INFO("=====================================\n");
    FS_LOG_DUMP_INFO("\n");
}
