#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "fsc/fsid/fsid.h"
#include "fsc/namespace/namespace.h"

/*
 * ============================================================
 * table entry
 * ============================================================
 */

/*
 * fsc_table_entry_t
 *
 * fstable 内部索引节点。
 *
 * 一个 entry 同时挂入两个 hash index：
 *
 *   fsid_index : fsid -> fsc_namespace_t
 *   name_index : name -> fsc_namespace_t
 *
 * entry 由 fstable 分配和释放。
 * entry 只持有 namespace 指针，不拥有 namespace 内存。
 * namespace 的分配和释放由 nspool/fsmgr 负责。
 */
typedef struct fsc_table_entry {

    fsc_namespace_t *ns; /* 被索引的 namespace */

    fs_list_head_t fsid_node; /* fsid_index hash 节点 */
    fs_list_head_t name_node; /* name_index hash 节点 */

} fsc_table_entry_t;

/*
 * ============================================================
 * table
 * ============================================================
 */

/*
 * fsc_table_t
 *
 * FSC Namespace 注册表。
 *
 * 负责维护 filesystem instance 的两类唯一索引：
 *
 *      fsid -> namespace
 *      name -> namespace
 *
 * fstable 不负责生命周期状态迁移，也不负责 refcnt；
 * 它只是索引层，对应 Object Layer 中 objtable 的角色。
 */
typedef struct fsc_table {

    fs_hash_t fsid_index; /* 按 FSID 查找 namespace */
    fs_hash_t name_index; /* 按名称查找 namespace */

} fsc_table_t;

/*
 * fstable_deinit() 销毁索引时的 namespace 回收回调。
 *
 * 参数：
 *      [IN] ns        : 待回收 namespace
 */
typedef void (*fstable_reclaim_fn)(
                fsc_namespace_t *ns);

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

/*
 * 初始化 namespace 注册表。
 *
 * 参数：
 *      [OUT] table     : 待初始化注册表
 *      [IN]  bucket_nr : hash bucket 数量
 *
 * 返回：
 *      FS_OK           : 成功
 *      fs_error_t      : 参数非法或 hash 初始化失败
 */
fs_error_t fstable_init(
                fsc_table_t *table,
                uint32_t bucket_nr);

/*
 * 销毁 namespace 注册表。
 *
 * 参数：
 *      [IN/OUT] table   : 待销毁表，内部 hash 资源会被释放并清零
 *      [IN]     reclaim : namespace 回收回调，可为 NULL
 */
void fstable_deinit(
                fsc_table_t *table,
                fstable_reclaim_fn reclaim);

/*
 * ============================================================
 * operations
 * ============================================================
 */

/*
 * 插入 namespace。
 *
 * fstable 会同时建立 FSID 和 name 两个索引。
 * 如果任一索引已存在，则返回 EEXIST 类错误。
 *
 * 参数：
 *      [IN/OUT] table : namespace 注册表
 *      [IN]     ns    : 待注册 namespace，必须已初始化
 *
 * 返回：
 *      FS_OK          : 成功
 *      fs_error_t     : 参数非法、重复或内存不足
 */
fs_error_t fstable_insert(
                fsc_table_t *table,
                fsc_namespace_t *ns);

/*
 * 移除 namespace。
 *
 * 本函数只移除索引和 entry wrapper，不释放 namespace 本体；
 * namespace 指针通过 ns_out 返回给调用者，由 fsmgr/nspool 回收。
 *
 * 参数：
 *      [IN/OUT] table  : namespace 注册表
 *      [IN]     fsid   : 待移除 namespace 的 FSID
 *      [OUT]    ns_out : 返回被移除的 namespace，可为 NULL
 *
 * 返回：
 *      FS_OK           : 成功
 *      fs_error_t      : 参数非法或未找到
 */
fs_error_t fstable_remove(
                fsc_table_t *table,
                fsc_fsid_t fsid,
                fsc_namespace_t **ns_out);

/*
 * 通过 FSID 查找 namespace。
 *
 * 参数：
 *      [IN] table     : namespace 注册表
 *      [IN] fsid      : filesystem id
 *
 * 返回：
 *      NULL           : 未找到
 *      非 NULL        : fsc_namespace_t 借用指针
 */
fsc_namespace_t *fstable_lookup_fsid(
                fsc_table_t *table,
                fsc_fsid_t fsid);

/*
 * 通过名称查找 namespace。
 *
 * 参数：
 *      [IN] table     : namespace 注册表
 *      [IN] name      : namespace 名称
 *
 * 返回：
 *      NULL           : 未找到
 *      非 NULL        : fsc_namespace_t 借用指针
 */
fsc_namespace_t *fstable_lookup_name(
                fsc_table_t *table,
                const char *name);

/*
 * 判断指定 FSID 是否已注册。
 */
bool fstable_exists_fsid(
                fsc_table_t *table,
                fsc_fsid_t fsid);

/*
 * 判断指定名称是否已注册。
 */
bool fstable_exists_name(
                fsc_table_t *table,
                const char *name);

/*
 * ============================================================
 * stats
 * ============================================================
 */

/*
 * 返回当前注册的 namespace 数量。
 *
 * 参数：
 *      [IN] table     : namespace 注册表
 */
uint64_t fstable_count(
                const fsc_table_t *table);
