#pragma once

/*
 * MirageFS 对象元数据层
 *
 * ObjMeta 负责：
 *
 *   1. 对象定位映射
 *          (objectid, gen) → backend object locator
 *
 *   2. 生命周期支撑
 *          提供 refcnt 和 state 字段，由 objmgr 管理并发
 *          访问和状态迁移。
 *
 * 当前阶段：
 * - 不维护 namespace
 * - 不维护 path
 * - 不维护 stat cache
 * - 不维护 hierarchy
 *
 * backend object 当前基于：
 *
 *      mount_id + file_handle
 *
 * 实现稳定定位。
 */

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "object/fuid/fuid.h"
#include "object/objkey/objkey.h"

/* ============================================================
 * 常量定义
 * ============================================================ */

/*
 * Linux file_handle 最大保存长度。
 *
 * 当前阶段：
 * - ext4/xfs 等主流文件系统通常远小于该值
 * - 16 bytes 对实验性项目已足够
 *
 * 后续若需要：
 * - 可扩展
 * - 可改为动态分配
 */
#define OBJMETA_MAX_HANDLE_SIZE 16

/*
 * ObjMeta 结构固定大小检查。
 *
 * 当前设计目标：
 *
 *      sizeof(obj_meta_t) == 48
 *
 * 便于：
 * - cache friendly
 * - mempool 管理
 * - KV 持久化
 */
#define OBJMETA_SIZE 48

/*
 * ============================================================
 * object lifecycle state
 * ============================================================
 *
 * ObjMeta 对象生命周期状态机：
 *
 *   INIT ──→ ACTIVE ──→ DELETING ──→ DELETED
 *     │                     │
 *     └─────────────────────┘
 *          (异常路径：直接销毁未激活对象)
 *
 * 说明：
 * - INIT     : 刚分配，尚未被 objmgr 激活
 * - ACTIVE   : 已激活，可正常使用
 * - DELETING : 正在删除中，阻止新引用
 * - DELETED  : 已销毁，等待回收
 *
 * 状态的读写由 objmgr 负责，objmeta 层仅保存状态值。
 */

typedef enum obj_state
{
    OBJ_STATE_INIT = 0,
    OBJ_STATE_ACTIVE,
    OBJ_STATE_DELETING,
    OBJ_STATE_DELETED

} obj_state_t;

/* ============================================================
 * 核心结构
 * ============================================================ */

/*
 * obj_handle_t
 *
 * Linux backend handle 封装。
 *
 * 将 mount_id 与 file_handle 打包为一个结构，
 * 隐藏 Linux VFS name_to_handle_at / open_by_handle_at
 * 的原始语义，使上层 objmgr 操作更清晰。
 */
typedef struct obj_handle
{
    int32_t mount_id; /* Linux mount ID，定位文件系统实例 */
    uint16_t type; /* file_handle 类型 (FILEID_INO32_GEN 等) */
    uint16_t len; /* data[] 中实际使用的字节数 */
    uint8_t  data[OBJMETA_MAX_HANDLE_SIZE]; /* file_handle 原始字节 */

} obj_handle_t;

/*
 * obj_meta_t
 *
 * MirageFS 对象元数据。
 *
 * 包含三部分：
 *
 *   key     : MirageFS 对象唯一标识 (objectid + gen)
 *   refcnt  : 原子引用计数，由 objmgr 管理并发访问
 *   state   : 生命周期状态 (obj_state_t)，由 objmgr 驱动状态迁移
 *   handle  : Linux backend handle，封装 mount_id + file_handle
 *
 * refcnt 和 state 为 objmgr 生命周期管理准备，
 * objmeta 层本身不对其语义做假设。
 */
typedef struct obj_meta {

    obj_key_t       key;    /* MirageFS 对象唯一标识 */
    fs_atomic32_t  refcnt; /* 引用计数，原子操作 */
    uint32_t       state;  /* 生命周期状态，见 obj_state_t */
    obj_handle_t   handle; /* Linux backend handle */

} obj_meta_t;

/* ============================================================
 * 编译期检查
 * ============================================================ */

_Static_assert(sizeof(obj_meta_t) == OBJMETA_SIZE,
               "obj_meta_t size invalid");

/* ============================================================
 * 对外接口
 * ============================================================ */

/*
 * 初始化 ObjMeta。
 *
 * 写入 key + backend handle，并将 refcnt 置 0、state 置为
 * OBJ_STATE_INIT。调用者需通过 objmgr 激活状态并管理引用计数。
 *
 * 参数：
 *      meta            : 目标对象
 *      key             : MirageFS object key
 *      mount_id        : Linux mount id
 *      handle_type     : Linux handle type
 *      handle_bytes    : handle 实际长度
 *      file_handle     : handle 数据
 *
 * 返回：
 *      0       : success
 *      <0      : failed
 */
int32_t objmeta_init(
                obj_meta_t *meta,
                const obj_key_t *key,
                int32_t mount_id,
                uint16_t handle_type,
                uint16_t handle_bytes,
                const uint8_t *file_handle);

/*
 * 清空 ObjMeta。
 *
 * 将整个结构置零（包括 refcnt、state 和 handle），
 * 等价于回到未初始化状态。
 */
void objmeta_reset(
                obj_meta_t *meta);

/*
 * 判断 ObjMeta 是否有效。
 */
bool objmeta_is_valid(
                const obj_meta_t *meta);

/*
 * 比较两个 ObjMeta 是否相同。
 */
bool objmeta_equal(
                const obj_meta_t *lhs,
                const obj_meta_t *rhs);

/*
 * 打印 ObjMeta 完整信息。
 *
 * 输出 key、refcnt、state、handle 全部字段，
 * 用于 debug / trace / 日志分析。
 */
void objmeta_dump(
                const obj_meta_t *meta);