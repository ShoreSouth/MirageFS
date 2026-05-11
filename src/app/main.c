/**
 * @file main.c
 * @brief MirageFS 主程序入口
 */

#include <stdio.h>

#include "common/fs_common.h"
#include "config/fs_config.h"

int main(void)
{
    printf("MirageFS Starting...\n");

    /*
     * 初始化全局配置
     */
    fs_config_init();

    /*
     * 初始化日志打印
     */
    fs_log_init(NULL, FS_LOG_INFO);

    /*
     * 打印配置
     */
    fs_config_dump();

    printf("MirageFS Exit.\n");

    return 0;
}