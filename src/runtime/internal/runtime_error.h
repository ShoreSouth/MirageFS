#pragma once

#include "common/fs_common.h"
#include "runtime/internal/runtime_sub.h"

/*
 * 构造 Runtime 模块错误码。
 *
 * 参数：
 *      [IN] sub : Runtime 内部 sub-error
 *      [IN] err : Linux errno
 *
 * 返回：
 *      fs_error_t : module 固定为 FS_MODULE_RUNTIME 的结构化错误
 */
fs_error_t runtime_error(runtime_sub_t sub, int err);
