#pragma once

#include <stdbool.h>

#include "common/fs_common.h"
#include "object/fuid/fuid.h"
#include "object/objruntime/objruntime.h"

/*
 * ============================================================
 * objmgr — 对象生命周期管理器
 *
 * objmgr 是 ObjTable 的上层服务，负责：
 *
 *   1. 对象生命周期管理
 *          create / delete / lookup
 *
 *   2. 引用计数管理
 *          get（获取引用）/ put（释放引用）
 *
 *   3. 状态迁移
 *          驱动 obj_runtime_t 的 state 状态机
 *          (INIT → ACTIVE → DELETING)
 *
 * objmgr 内部持有全局 obj_table_t 实例，
 * 所有对象操作均通过 objmgr 统一管理。
 *
 * 公共 API 返回 obj_meta_t *（&runtime->meta），
 * 内部以 obj_runtime_t * 作为运行时锚点。
 * ============================================================
 */

/*
 * ============================================================
 * 初始化
 * ============================================================
 */

/*
 * 初始化对象管理器。
 *
 * 创建内部 obj_table_t 实例。
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 失败
 */
fs_error_t objmgr_init(void);

/*
 * 销毁对象管理器。
 *
 * 释放内部 obj_table_t 及所有已注册对象。
 */
void objmgr_deinit(void);

/*
 * ============================================================
 * 对象生命周期
 * ============================================================
 */

/*
 * 创建对象。
 *
 * 将 obj_runtime_t 注册到内部对象表，
 * 并激活其生命周期状态。
 *
 * 参数：
 *      [IN] fuid      : 待创建对象的 FUID
 *      [IN] handle    : 对象句柄
 *
 * 返回：
 *      non-NULL    : 成功，返回 obj_meta_t 指针
 *      NULL        : 失败
 */
obj_meta_t *objmgr_create(
                const fuid_t *fuid,
                const obj_handle_t *handle);

fs_error_t objmgr_alloc_objectid(
                ObjectId_t *out_objectid);

/*
 * 删除对象。
 *
 * 驱动状态迁移 ACTIVE → DELETING，
 * 最终从对象表中移除并回收。
 *
 * 参数：
 *      [IN] fuid   : 待删除对象的 FUID
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 失败
 */
fs_error_t objmgr_delete(
                const fuid_t *fuid);

/*
 * ============================================================
 * 对象查找
 * ============================================================
 */

/*
 * lookup 仅返回对象指针。
 * 不会增加引用计数。
 * 如果需要长期持有对象，
 * 应调用 objmgr_get()。
 *
 * 参数：
 *      [IN] fuid   : 对象 FUID 标识
 *
 * 返回：
 *      NULL        : 未找到
 *      非 NULL     : 对象元数据指针（由 objmgr 管理生命周期，调用者不应释放）
 */
obj_meta_t *objmgr_lookup(
                const fuid_t *fuid);

/*
 * 判断对象是否存在。
 */
bool objmgr_exists(
                const fuid_t *fuid);

/*
 * ============================================================
 * 对象访问（推荐的对象访问接口）
 * ============================================================
 */

/*
 * 获取对象并增加引用。
 *
 * 本接口等价于：
 *
 *      lookup()
 *          +
 *      get()
 *
 * 如果对象不存在或当前状态不允许获取引用（例如
 * DELETING），返回 NULL。
 *
 * 返回：
 *      NULL        : 获取失败
 *      非 NULL     : 已获取引用的对象元数据
 *
 * 注意：
 *      调用成功后，必须对应调用 objmgr_release()
 *      释放引用。
 */
obj_meta_t *objmgr_acquire(
                const fuid_t *fuid);

obj_meta_t *objmgr_acquire_by_handle(
                const obj_handle_t *handle);

/*
 * 释放对象引用。
 *
 * 本接口等价于：
 *
 *      put()
 *
 * 当引用计数减至 0 时，
 * objmgr 将根据对象状态决定是否释放对象。
 *
 * 参数：
 *      [IN/OUT] meta  : acquire() 返回的对象元数据（refcnt 将被递减，可能回收）
 */
void objmgr_release(
                obj_meta_t *meta);

/*
 * ============================================================
 * 引用计数（底层引用计数接口）
 * ============================================================
 */

/*
 * 获取引用（refcnt++）。
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 失败（对象不存在 | 状态不允许）
 */
fs_error_t objmgr_get(
                const fuid_t *fuid);

/*
 * 释放引用（refcnt--）。
 */
fs_error_t objmgr_put(
                const fuid_t *fuid);

/*
 * 获取当前引用计数。
 */
int32_t objmgr_refcnt(
                const fuid_t *fuid);

/*
 * ============================================================
 * 对象状态
 * ============================================================
 */

/*
 * 获取对象生命周期状态。
 *
 * 返回 obj_state_t 枚举值。
 */
obj_state_t objmgr_state(
                const fuid_t *fuid);

/*
 * ============================================================
 * 统计
 * ============================================================
 */

/*
 * 当前注册对象数量。
 */
uint32_t objmgr_count(void);
