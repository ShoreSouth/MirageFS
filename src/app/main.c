/**
 * @file main.c
 * @brief MirageFS 主程序入口
 */

#include <stdio.h>

#include "common/fs_common.h"
#include "config/fs_config.h"
#include "fsc/fsc_init.h"
#include "fops/include/fops.h"
#include "lsa/include/lsa_api.h"
#include "object/object_init.h"

int main(void)
{
    fs_error_t err;
    printf("MirageFS Starting...\n");

    /*
     * 初始化全局配置
     */
    fs_config_init();

    /*
     * 初始化trace
     */
    fs_trace_ctx_t trace_ctx;
    FS_TRACE_BEGIN(&trace_ctx);

    /*
     * 初始化日志打印
     */
    fs_log_init(NULL, FS_LOG_INFO);

    /*
     * 模块初始化
     *
     * 顺序：从底层到上层
     *
     *   LSA     — 底层系统访问
     *   Object  — 对象模块（注册 + 子模块初始化）
     */
    lsa_init();

    err = object_init();
    if (fs_failed(err)) {
        FS_TRACE_END();
        return 1;
    }

    err = fsc_init();
    if (fs_failed(err)) {
        object_deinit();
        FS_TRACE_END();
        return 1;
    }

    err = fops_init();
    if (fs_failed(err)) {
        fsc_deinit();
        object_deinit();
        FS_TRACE_END();
        return 1;
    }

    /*
     * 打印配置
     */
    fs_config_dump();

    /*
     * 模块销毁（逆序）
     */
    fops_deinit();
    fsc_deinit();
    object_deinit();

    /*
     * 清除trace
     */
    FS_TRACE_END();

    printf("MirageFS Exit.\n");

    return 0;
}
