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
 */
typedef Fsid_t fsc_fsid_t;

#define FSID_INVALID ((fsc_fsid_t)0)

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

fs_error_t fsid_init(void);

/*
 * 销毁 FSID 分配器。
 *
 * 当前实现是单调递增分配器，没有复用空闲 FSID；
 * deinit 仅重置下一次分配的起始值。
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
 * 参数：
 *      [OUT] fsid  : 输出分配得到的 filesystem id
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 参数非法或分配溢出
 */
fs_error_t fsid_alloc(
                fsc_fsid_t *fsid);

/*
 * 释放一个 filesystem id。
 *
 * 当前版本不复用 FSID；本接口用于保持生命周期语义完整，
 * 后续可在这里接入 bitmap / free-list。
 *
 * 参数：
 *      [IN] fsid   : 待释放 filesystem id
 */
fs_error_t fsid_free(
                fsc_fsid_t fsid);

/*
 * ============================================================
 * value ops
 * ============================================================
 */

bool fsid_is_valid(
                fsc_fsid_t fsid);

uint64_t fsid_hash(
                fsc_fsid_t fsid);
