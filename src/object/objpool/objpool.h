#pragma once

#include "common/fs_common.h"

#include "object/objmeta/objmeta.h"

/*
 * ============================================================
 * objpool
 *
 * Object Layer 专用对象池。
 *
 * ObjPool 负责 ObjMeta 的统一内存管理，
 * 屏蔽底层内存池实现（Buddy / Slab）。
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
int32_t objpool_init(void);

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
 * 分配一个 ObjMeta。
 *
 * 返回：
 *      NULL        分配失败
 *      非 NULL     ObjMeta
 */
obj_meta_t *objpool_alloc(void);

/*
 * 释放一个 ObjMeta。
 */
void objpool_free(
                obj_meta_t *meta);