#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fsc/fsc_sub_table.h"

/*
 * ============================================================
 * FSC sub-error enum
 * ============================================================
 */

/*
 * fsc_sub_t
 *
 * FSC 模块内部的 sub-error 分类。
 *
 * sub 字段用于指出错误发生在哪个 FSC 内部组件或操作阶段，
 * 例如 FSID 分配、Namespace 初始化、FSTable 插入、FSMgr 创建等。
 * fsc_error() 会把它编码进 fs_error_t 的 sub 字段。
 */
typedef enum fsc_sub
{

#define FSC_SUB_ENUM(name, str) FSC_SUB_##name,

    FSC_SUB_TABLE(FSC_SUB_ENUM)

#undef FSC_SUB_ENUM

            FSC_SUB_MAX

} fsc_sub_t;

/*
 * ============================================================
 * helper
 * ============================================================
 */

/*
 * 返回 sub-error 的字符串名称。
 *
 * 参数：
 *      [IN] sub   : FSC sub-error
 */
const char *fsc_sub_name(fsc_sub_t sub);

/*
 * 判断 sub-error 是否在 FSC 定义范围内。
 */
bool fsc_sub_valid(uint32_t sub);
