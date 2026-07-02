#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"

/*
 * ============================================================
 * type definition
 * ============================================================
 */

/*
 * fsc_fsid_t
 *
 * FSC 内部使用的 filesystem id 类型。
 *
 * 注意：Linux 系统头中已经存在 fsid_t，因此 FSC 不使用 fsid_t 作为
 * typedef 名称，避免和 <sys/types.h> 冲突。FUID 中的 Fsid_t 是底层
 * 语义类型，fsc_fsid_t 是 FSC 模块语境下的别名。
 *
 * 当前编码：
 *      high 32 bits : generation
 *      low  32 bits : allocator slot
 *
 * generation 用于在 slot 复用后识别 stale FSID。
 */
typedef Fsid_t fsc_fsid_t;

#define FSID_INVALID ((fsc_fsid_t)0)

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

/*
 * 初始化 FSID 分配器。
 *
 * 分配器内部维护 slot bitmap、free-list 和 generation 表。
 * 初始化后所有 slot 都处于可分配状态，generation 从 1 开始。
 *
 * 返回：
 *      FS_OK          : 成功
 *      fs_error_t     : 锁初始化等失败
 */
fs_error_t fsid_init(void);

/*
 * 销毁 FSID 分配器。
 *
 * 释放同步资源并清空内部状态。
 * 调用方必须保证没有并发的 fsid_alloc() / fsid_free()。
 */
void fsid_deinit(void);

/*
 * ============================================================
 * allocation
 * ============================================================
 */

/*
 * 分配一个 filesystem id。
 *
 * 分配时从 free-list 取出一个 slot，并使用该 slot 当前 generation
 * 组合成完整 fsc_fsid_t。
 *
 * 参数：
 *      [OUT] fsid  : 输出分配得到的 filesystem id
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 参数非法或无空闲 slot
 */
fs_error_t fsid_alloc(
                fsc_fsid_t *fsid);

/*
 * 释放一个 filesystem id。
 *
 * 释放成功后，对应 slot 会回到 free-list，generation 会递增。
 * 因此旧 FSID 即使 slot 被再次分配，也会因为 generation 不同而变成 stale。
 *
 * 参数：
 *      [IN] fsid   : 待释放 filesystem id
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : FSID 非法、重复释放或 stale
 */
fs_error_t fsid_free(
                fsc_fsid_t fsid);

/*
 * ============================================================
 * value ops
 * ============================================================
 */

/*
 * 判断 FSID 编码是否有效。
 *
 * 本函数只检查结构合法性，不检查该 FSID 当前是否 live。
 * live / stale 判断由 fsid_free() 等需要分配器状态的路径完成。
 *
 * 参数：
 *      [IN] fsid   : 待检查 filesystem id
 */
bool fsid_is_valid(
                fsc_fsid_t fsid);

/*
 * 计算 FSID hash 值。
 *
 * 参数：
 *      [IN] fsid   : filesystem id
 */
uint64_t fsid_hash(
                fsc_fsid_t fsid);
