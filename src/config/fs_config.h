/**
 * @file fs_config.h
 * @brief MirageFS 全局配置管理
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

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
