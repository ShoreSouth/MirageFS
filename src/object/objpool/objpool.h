#pragma once

#include "common/fs_common.h"

#include "object/objruntime/objruntime.h"

/*
 * ============================================================
 * objpool
 *
 * Object Layer 专用对象池。
 *
 * ObjPool 负责 obj_runtime_t 的统一内存管理，
 * 屏蔽底层内存池实现（Buddy / Slab）。
 *
 * obj_runtime_t 作为运行时对象载体，包含：
 *   - obj_meta_t meta    : 对象元数据
 *   - refcnt             : 引用计数
 *   - state              : 生命周期状态
 *
 * 当前版本基于 common/mempool，
 * 后续可无缝切换为 Slab。
 * ============================================================
 */

/*
 * ============================================================
 * 生命周期
 * ============================================================
 */

/*
 * 初始化对象池。
 *
 * 返回：
 *      FS_OK      成功
 *      其它       失败
 */
fs_error_t objpool_init(void);

/*
 * 销毁对象池。
 */
void objpool_deinit(void);

/*
 * ============================================================
 * 对象申请 / 释放
 * ============================================================
 */

/*
 * 分配一个 obj_runtime_t。
 *
 * 返回：
 *      NULL        分配失败
 *      非 NULL     obj_runtime_t
 */
obj_runtime_t *objpool_alloc(void);

/*
 * 释放一个 obj_runtime_t。
 *
 * 参数：
 *      [IN] rt     : 待释放的运行时对象（必须由 objpool_alloc 分配）
 */
void objpool_free(obj_runtime_t *rt);
