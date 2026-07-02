#pragma once

#include "common/fs_common.h"

/*
 * ============================================================
 * FSC lifecycle
 * ============================================================
 */

/*
 * 初始化 Filesystem Control 模块。
 *
 * FSC 是 MirageFS 的文件系统控制平面，负责：
 *
 *   - FSID 分配
 *   - Namespace runtime object 生命周期
 *   - Namespace 注册表
 *   - fsmgr 对外门面
 *
 * 初始化顺序：
 *
 *      fsid_init()
 *          -> nspool_init()
 *          -> fsmgr_init()
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 任一子模块初始化失败
 */
fs_error_t fsc_init(void);

/*
 * 销毁 Filesystem Control 模块。
 *
 * 销毁顺序与初始化顺序相反：
 *
 *      fsmgr_deinit()
 *          -> nspool_deinit()
 *          -> fsid_deinit()
 */
void fsc_deinit(void);
