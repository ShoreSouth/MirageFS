#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "fsc/fsid/fsid.h"
#include "fsc/namespace/namespace.h"
#include "object/fuid/fuid.h"
#include "object/objmeta/objmeta.h"

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

/*
 * 初始化 FSC manager。
 *
 * fsmgr 是 FSC 的门面层，内部协调 fsid、nspool、fstable，
 * 并在 create/destroy 路径中把文件系统根目录的真实创建/删除动作
 * 委托给 LSA 完成。
 */
fs_error_t fsmgr_init(void);

/* 销毁 FSC manager，仅释放内存索引，不主动删除磁盘上的根目录。 */
void fsmgr_deinit(void);

/*
 * ============================================================
 * namespace lifecycle
 * ============================================================
 */

/*
 * 创建一个 filesystem namespace。
 *
 * 本函数是文件系统创建的对外入口：
 *   1. 分配 FSID；
 *   2. 构造文件系统根目录 FUID；
 *   3. 在 sysroot 下创建同名目录；
 *   4. 获取该目录的 backend handle；
 *   5. 注册 namespace 到 fstable。
 *
 * 对外输出 root_out，而不是 fd/path/obj_handle。后续业务应拿 FUID
 * 查询对象元数据，再由 LSA 层在边界内打开临时 fd。
 */
fs_error_t fsmgr_create(
                const char *name,
                fuid_t *root_out);

/*
 * 销毁 namespace，并删除 sysroot 下对应的文件系统根目录。
 *
 * 当前只允许销毁空目录；如果后端目录非空，LSA 会返回 ENOTEMPTY。
 */
fs_error_t fsmgr_destroy(
                fsc_fsid_t fsid);

/*
 * ============================================================
 * lookup
 * ============================================================
 */

/* 通过名称查找 ACTIVE namespace，返回借用指针。 */
fsc_namespace_t *fsmgr_lookup(
                const char *name);

/* 通过 FSID 查找 ACTIVE namespace，返回借用指针。 */
fsc_namespace_t *fsmgr_lookup_fsid(
                fsc_fsid_t fsid);

/* 判断指定名称的 namespace 是否存在。 */
bool fsmgr_exists(
                const char *name);

/* 获取 namespace 根目录 FUID。 */
fs_error_t fsmgr_get_root_fuid(
                fsc_fsid_t fsid,
                fuid_t *root_out);

/*
 * 获取 namespace 根目录 backend handle。
 *
 * 该接口只用于 FSC/Object 内部衔接，不应作为业务层主入口。
 */
fs_error_t fsmgr_get_root_handle(
                fsc_fsid_t fsid,
                obj_handle_t *handle_out);

/*
 * ============================================================
 * stats
 * ============================================================
 */

/* 返回当前注册中的 namespace 数量。 */
uint32_t fsmgr_count(void);
