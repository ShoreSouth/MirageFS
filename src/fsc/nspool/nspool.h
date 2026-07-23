#pragma once

#include "common/fs_common.h"
#include "fsc/namespace/namespace.h"

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

/*
 * 初始化 Namespace Pool。
 *
 * NSPool 是 FSC 专用对象池，负责 fsc_namespace_t 的内存管理。
 * 这样 namespace runtime object 和 Object Layer 的 obj_runtime_t
 * 一样，统一走项目内的 mempool 设施。
 */
fs_error_t nspool_init(void);

/*
 * 销毁 Namespace Pool。
 *
 * 调用前应确保 fsmgr/fstable 已释放所有 namespace。
 */
void nspool_deinit(void);

/*
 * ============================================================
 * allocation
 * ============================================================
 */

/*
 * 分配一个未初始化的 fsc_namespace_t。
 *
 * 返回：
 *      NULL        : 分配失败或 nspool 未初始化
 *      非 NULL     : 清零后的 namespace 对象
 */
fsc_namespace_t *nspool_alloc(void);

/*
 * 释放 namespace。
 *
 * 参数：
 *      [IN] ns     : nspool_alloc() 返回的 namespace，可为 NULL
 */
void nspool_free(fsc_namespace_t *ns);
