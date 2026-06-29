#pragma once

#include "common/error/fs_error.h"

/*
 * ============================================================
 * Object Module Bootstrap
 *
 * object_init() 负责：
 *
 *   1. 向 common/error 注册 COMMON 和 Object Layer 的
 *      sub-error → name 转换函数
 *
 *   2. 委托各子模块初始化（objmgr → objtable ...）
 *
 * object_deinit() 按逆序销毁。
 *
 * 使用方式：
 *
 *   main():
 *       object_init();
 *       ...
 *       object_deinit();
 * ============================================================
 */

fs_error_t object_init(void);

void object_deinit(void);
