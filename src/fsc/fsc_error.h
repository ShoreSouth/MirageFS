#pragma once

#include "common/error/fs_error.h"
#include "fsc/fsc_sub.h"

/*
 * ============================================================
 * FSC error builder
 * ============================================================
 */

/*
 * 构造 FSC 模块错误码。
 *
 * FSC 使用统一 fs_error_t 布局，本函数固定 module 字段为
 * FS_MODULE_FSC，调用者只需要传入 FSC 子错误和 Linux errno。
 *
 * 参数：
 *      [IN] sub   : FSC 子错误，见 fsc_sub_t
 *      [IN] err   : Linux errno
 *
 * 返回：
 *      fs_error_t : 编码后的 FSC 错误
 */
fs_error_t fsc_error(fsc_sub_t sub, int err);
