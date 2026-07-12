#pragma once

/*
 * Mirage Shell 对外入口。
 *
 * msh 是 MirageFS V1.0 的交互式控制台层。它负责解析用户命令、展示
 * prompt，并通过 Runtime API 执行文件系统操作。
 */

/*
 * 启动 msh。
 *
 * 参数：
 *      [IN] argc : main 函数收到的参数数量
 *      [IN] argv : main 函数收到的参数数组
 *
 * 返回：
 *      0 : 正常退出
 *      1 : 初始化失败、命令执行失败或参数错误
 */
int msh_main(int argc, char **argv);
