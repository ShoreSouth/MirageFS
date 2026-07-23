#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "fsc/fsid/fsid.h"
#include "object/fuid/fuid.h"
#include "object/objmeta/objmeta.h"

/*
 * ============================================================
 * constants
 * ============================================================
 */

#define FSC_NAMESPACE_NAME_MAX 64U /* 名称最大长度，包含结尾 '\0' */
#define FSC_NAMESPACE_SIZE 192U    /* 固定大小，便于 pool 管理 */


/*
 * ============================================================
 * namespace lifecycle state
 * ============================================================
 */

typedef enum fsc_namespace_state
{

    FSC_NAMESPACE_STATE_INVALID = 0, /* 无效或不存在 */
    FSC_NAMESPACE_STATE_INIT,        /* 已初始化，尚未对外可见 */
    FSC_NAMESPACE_STATE_ACTIVE,      /* 已注册到 fstable，可被 lookup */
    FSC_NAMESPACE_STATE_DELETING, /* 正在销毁，阻止新的外部使用 */

} fsc_namespace_state_t;

/*
 * ============================================================
 * namespace runtime object
 * ============================================================
 */

/*
 * fsc_namespace_t
 *
 * FSC 的 Namespace Runtime Object，描述一个 filesystem instance 的
 * 控制面状态。
 *
 *   fsid        : 文件系统身份，由 FSID 分配器生成。
 *   name        : namespace 名称，也是 sysroot 下的根目录名。
 *   root_fuid   : 文件系统根目录对象的 FUID，对外查询优先使用它。
 *   root_handle : 根目录后端 handle，仅作为内部/对象层定位信息保存。
 *   refcnt      : 预留给后续 acquire/release 模型。
 *   state       : 生命周期状态，见 fsc_namespace_state_t。
 *
 * 注意：结构体不保存 fd/path。需要访问后端时，调用方应通过 FUID
 * 找到元数据，再由 LSA 在边界内打开临时 fd。
 */
typedef struct fsc_namespace
{
    fsc_fsid_t fsid;
    char name[FSC_NAMESPACE_NAME_MAX];

    fuid_t root_fuid;
    obj_handle_t root_handle;

    fs_atomic32_t refcnt;
    uint32_t state;

    uint8_t reserved[24];

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
 * 本函数只填充对象字段，不注册到 fstable，也不创建后端目录。
 * 后端目录创建由 fsmgr_create() 在调用本函数前完成。
 */
fs_error_t fsc_namespace_init(fsc_namespace_t *ns, fsc_fsid_t fsid,
                              const char *name, const fuid_t *root_fuid,
                              const obj_handle_t *root_handle);

/* 清空 Namespace Runtime Object。 */
void fsc_namespace_deinit(fsc_namespace_t *ns);

/*
 * ============================================================
 * value ops
 * ============================================================
 */

bool fsc_namespace_is_valid(const fsc_namespace_t *ns);

bool fsc_namespace_name_is_valid(const char *name);

fsc_namespace_state_t fsc_namespace_state(const fsc_namespace_t *ns);

bool fsc_namespace_state_can_transit(fsc_namespace_state_t from,
                                     fsc_namespace_state_t to);

fs_error_t fsc_namespace_change_state(fsc_namespace_t *ns,
                                      fsc_namespace_state_t state);

/*
 * ============================================================
 * debug
 * ============================================================
 */

/* 单行打印 namespace 快照，避免 dump 路径刷屏。 */
void fsc_namespace_dump(const fsc_namespace_t *ns);
