/**
 * @file fs_config.c
 * @brief MirageFS 全局配置管理实现
 */

#include <stdio.h>

#include "fs_config.h"
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

    FS_LOG_DUMP_INFO("worker_nr    : %u\n",
           g_fs_config.worker_nr);

    FS_LOG_DUMP_INFO("trace_enable : %s\n",
           g_fs_config.trace_enable ? "true" : "false");

    FS_LOG_DUMP_INFO("debug_enable : %s\n",
           g_fs_config.debug_enable ? "true" : "false");

    FS_LOG_DUMP_INFO("=====================================\n");
    FS_LOG_DUMP_INFO("\n");
}