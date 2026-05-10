/**
 * @file main.c
 * @brief MirageFS 主程序入口
 */

#include <stdio.h>

#include "config/fs_config.h"

int main(void)
{
    printf("MirageFS Starting...\n");

    /*
     * 初始化全局配置
     */
    fs_config_init();

    /*
     * 打印配置
     */
    fs_config_dump();

    printf("MirageFS Exit.\n");

    return 0;
}