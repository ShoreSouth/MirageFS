#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "fsc/fsid/fsid.h"
#include "fsc/namespace/namespace.h"

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

/*
 * 初始化 FSC manager。
 *
 * fsmgr 是 FSC 的门面层，内部协调：
 *
 *   - fsid   : 分配 filesystem id
 *   - nspool : 分配 namespace runtime object
 *   - fstable: 建立 namespace 索引
 *
 * 返回：
 *      FS_OK      : 成功
 *      fs_error_t : 初始化 hash/lock 失败
 */
fs_error_t fsmgr_init(void);

/*
 * 销毁 FSC manager。
 *
 * 会释放 fstable 中仍注册的 namespace，并重置内部计数。
 */
void fsmgr_deinit(void);

/*
 * ============================================================
 * namespace lifecycle
 * ============================================================
 */

/*
 * 创建 namespace。
 *
 * 成功后，返回的 namespace 已经处于 ACTIVE 状态，并已注册到 fstable。
 * 返回值是借用指针，生命周期由 fsmgr 管理。
 *
 * 参数：
 *      [IN] name      : namespace 名称，必须唯一
 *      [IN] root      : root object backend handle，可为 NULL
 *
 * 返回：
 *      NULL           : 创建失败
 *      非 NULL        : 创建成功的 namespace
 */
fsc_namespace_t *fsmgr_create(
                const char *name,
                const obj_handle_t *root);

/*
 * 销毁 namespace。
 *
 * 参数：
 *      [IN] fsid      : 目标 filesystem id
 *
 * 返回：
 *      FS_OK          : 成功
 *      fs_error_t     : 未找到、忙碌或参数非法
 */
fs_error_t fsmgr_destroy(
                fsc_fsid_t fsid);

/*
 * ============================================================
 * lookup
 * ============================================================
 */

/*
 * 通过名称查找 namespace。
 *
 * 参数：
 *      [IN] name      : namespace 名称
 *
 * 返回：
 *      NULL           : 未找到或不是 ACTIVE
 *      非 NULL        : namespace 借用指针
 */
fsc_namespace_t *fsmgr_lookup(
                const char *name);

/*
 * 通过 FSID 查找 namespace。
 *
 * 参数：
 *      [IN] fsid      : filesystem id
 *
 * 返回：
 *      NULL           : 未找到或不是 ACTIVE
 *      非 NULL        : namespace 借用指针
 */
fsc_namespace_t *fsmgr_lookup_fsid(
                fsc_fsid_t fsid);

/*
 * 判断指定名称的 namespace 是否存在。
 */
bool fsmgr_exists(
                const char *name);

/*
 * 获取 namespace 的 root object handle。
 *
 * 参数：
 *      [IN] fsid      : filesystem id
 *
 * 返回：
 *      NULL           : 未找到
 *      非 NULL        : root handle 借用指针
 */
const obj_handle_t *fsmgr_get_root(
                fsc_fsid_t fsid);

/*
 * ============================================================
 * stats
 * ============================================================
 */

/*
 * 返回当前注册中的 namespace 数量。
 */
uint32_t fsmgr_count(void);
