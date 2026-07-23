#pragma once

#include <stdint.h>

#include "common/module/fs_module.h"

/*
 * ============================================================
 * sub-error name dispatch
 *
 * fs_error_t 的 sub 字段含义随模块不同而变化：
 *
 *   LSA     → fs_op_t     (common/op/fs_op.h)
 *   OBJECT  → obj_sub_t   (object/obj_sub.h)
 *
 * 为避免 common/ 反向依赖上层模块，
 * 本模块提供注册机制：
 *
 *   1. 上层模块在 init 时调用 fs_sub_register()
 *      注册自己的 sub → name 转换函数。
 *
 *   2. fs_error_str() 通过 fs_sub_name() 获取
 *      任意模块 sub 字段的名称字符串。
 *
 * 未来新增模块只需：
 *   1. 定义自己的 xxx_sub_table.h
 *   2. 实现 xxx_sub_name()
 *   3. 在模块 init 中注册
 * ============================================================
 */

/*
 * sub-name 转换函数签名。
 *
 * 参数：
 *      sub     : fs_error_t 的 sub 字段原始值
 *
 * 返回：
 *      sub 名称字符串（静态存储期）
 *      NULL 表示该 module 未注册
 */
typedef const char *(*fs_sub_name_fn)(uint32_t sub);

/*
 * 注册模块的 sub-name 转换函数。
 *
 * 通常在模块 init 中调用。
 */
void fs_sub_register(fs_module_t module, fs_sub_name_fn fn);

/*
 * 获取 sub 字段的名称字符串。
 *
 * 参数：
 *      module  : fs_error_t 的 module 字段
 *      sub     : fs_error_t 的 sub 字段
 *
 * 返回：
 *      名称字符串指针（静态存储期）
 *      NULL 表示该 module 未注册
 */
const char *fs_sub_name(fs_module_t module, uint32_t sub);
