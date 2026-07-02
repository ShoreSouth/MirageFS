#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "fsc/fsid/fsid.h"
#include "object/objmeta/objmeta.h"

/*
 * ============================================================
 * constants
 * ============================================================
 */

#define FSC_NAMESPACE_NAME_MAX 64U /* Namespace 名称最大长度（含结尾 '\0'） */
#define FSC_NAMESPACE_SIZE     128U /* fsc_namespace_t 固定大小，便于 pool 管理 */

/*
 * ============================================================
 * namespace lifecycle state
 * ============================================================
 */

/*
 * FSC Namespace 生命周期状态。
 *
 * Namespace 是 FSC 的运行时对象，代表一个 filesystem instance
 * 在 MirageFS 内部的控制平面入口。状态由 fsmgr 驱动：
 *
 *      INIT
 *        |
 *        v
 *      ACTIVE
 *        |
 *        v
 *      DELETING
 *        |
 *        v
 *      nspool_free()
 *
 * INVALID 仅作为查询失败 / 未初始化的哨兵值，不应存入有效对象。
 */
typedef enum fsc_namespace_state {

    FSC_NAMESPACE_STATE_INVALID = 0, /* 无效或不存在 */
    FSC_NAMESPACE_STATE_INIT,        /* 已分配并初始化，尚未对外可见 */
    FSC_NAMESPACE_STATE_ACTIVE,      /* 已注册到 fstable，可被 lookup */
    FSC_NAMESPACE_STATE_DELETING,    /* 正在销毁，阻止新的外部使用 */

} fsc_namespace_state_t;

/*
 * ============================================================
 * namespace runtime object
 * ============================================================
 */

/*
 * fsc_namespace_t
 *
 * FSC 的 Namespace Runtime Object。
 *
 * 它描述一个 filesystem instance 在控制平面的运行时状态：
 *
 *   fsid    : 文件系统身份。FSC 分配，Object/FUID 继承该身份。
 *   name    : namespace 名称，用于 fstable 的 name 索引。
 *   root    : root object 的后端 handle。当前阶段仅保存，不解释语义。
 *   refcnt  : 预留给后续 acquire/release 模型，当前 create/destroy 链路要求为 0。
 *   state   : 生命周期状态，见 fsc_namespace_state_t。
 *
 * 所有 fsc_namespace_t 实例必须由 nspool_alloc() 分配，
 * 由 nspool_free() 释放；fstable 只持有索引引用，不拥有对象内存。
 */
typedef struct fsc_namespace {

    fsc_fsid_t      fsid; /* 文件系统身份 */
    char            name[FSC_NAMESPACE_NAME_MAX]; /* namespace 名称 */

    obj_handle_t    root; /* root object 的 backend handle */

    fs_atomic32_t   refcnt; /* 引用计数，后续 acquire/release 使用 */
    uint32_t        state;  /* fsc_namespace_state_t */

    uint8_t         reserved[24]; /* 预留字段，保持结构体大小稳定 */

} fsc_namespace_t;

_Static_assert(sizeof(fsc_namespace_t) == FSC_NAMESPACE_SIZE,
               "fsc_namespace_t size invalid");

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

/*
 * 初始化 Namespace Runtime Object。
 *
 * 本函数只填充对象字段，不把对象注册进 fstable。
 * 对外可见性由 fsmgr 在 fstable_insert() 成功后切换为 ACTIVE。
 *
 * 参数：
 *      [OUT] ns       : 目标 namespace，由 nspool_alloc() 分配
 *      [IN]  fsid     : FSC 分配的 filesystem id
 *      [IN]  name     : namespace 名称，长度必须小于 FSC_NAMESPACE_NAME_MAX
 *      [IN]  root     : root object backend handle，可为 NULL
 *
 * 返回：
 *      FS_OK          : 成功
 *      fs_error_t     : 参数非法等失败
 */
fs_error_t fsc_namespace_init(
                fsc_namespace_t *ns,
                fsc_fsid_t fsid,
                const char *name,
                const obj_handle_t *root);

/*
 * 重置 Namespace Runtime Object。
 *
 * 参数：
 *      [OUT] ns       : 目标 namespace，内容将被清零
 */
void fsc_namespace_deinit(
                fsc_namespace_t *ns);

/*
 * ============================================================
 * value ops
 * ============================================================
 */

/*
 * 判断 namespace 是否是有效运行时对象。
 *
 * 参数：
 *      [IN] ns        : 待检查 namespace
 */
bool fsc_namespace_is_valid(
                const fsc_namespace_t *ns);

/*
 * 判断 namespace 名称是否合法。
 *
 * 参数：
 *      [IN] name      : 待检查名称
 */
bool fsc_namespace_name_is_valid(
                const char *name);

/*
 * 读取 namespace 生命周期状态。
 *
 * 参数：
 *      [IN] ns        : 目标 namespace
 */
fsc_namespace_state_t fsc_namespace_state(
                const fsc_namespace_t *ns);

/*
 * 判断 namespace 生命周期状态是否允许迁移。
 *
 * 当前仅允许：
 *      INIT   -> ACTIVE
 *      ACTIVE -> DELETING
 *
 * 参数：
 *      [IN] from      : 当前状态
 *      [IN] to        : 目标状态
 */
bool fsc_namespace_state_can_transit(
                fsc_namespace_state_t from,
                fsc_namespace_state_t to);

/*
 * 切换 namespace 生命周期状态。
 *
 * 这是 namespace 状态修改的唯一公共入口。
 * 调用方应通过本函数推进状态机，避免直接改写 ns->state。
 *
 * 参数：
 *      [IN/OUT] ns    : 目标 namespace
 *      [IN]     state : 新状态
 */
fs_error_t fsc_namespace_change_state(
                fsc_namespace_t *ns,
                fsc_namespace_state_t state);

/*
 * ============================================================
 * debug
 * ============================================================
 */

/*
 * 打印 namespace 的完整运行时状态。
 *
 * 参数：
 *      [IN] ns        : 待打印 namespace
 */
void fsc_namespace_dump(
                const fsc_namespace_t *ns);
