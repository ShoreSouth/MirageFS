#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "object/objmeta/objmeta.h"

/* ============================================================
 * 常量定义
 * ============================================================ */

#define OBJRUNTIME_SIZE 48

/* ============================================================
 * object lifecycle state
 * ============================================================
 *
 * obj_runtime_t 对象生命周期状态机：
 *
 *      INIT
 *        │
 *        ▼
 *     ACTIVE
 *        │
 *        ▼
 *    DELETING
 *        │
 *        ▼
 *   ObjTable Remove
 *        +
 *   ObjPool Free
 *
 * 对象释放后即不存在，
 * 不再进入任何状态。
 *
 * 说明：
 * - INVALID  : 对象不存在（由 lookup 返回，不存储在 runtime 中）
 * - INIT     : 刚分配，尚未被 objmgr 激活
 * - ACTIVE   : 已激活，可正常使用
 * - DELETING : 正在删除中，阻止新引用；refcnt==0 时回收
 *
 * 状态的读写由 objmgr 负责，objruntime 层仅保存状态值。
 */

typedef enum obj_state
{
    OBJ_STATE_INVALID = 0,
    OBJ_STATE_INIT,
    OBJ_STATE_ACTIVE,
    OBJ_STATE_DELETING

} obj_state_t;

/* ============================================================
 * 核心结构
 * ============================================================ */

/*
 * obj_runtime_t
 *
 * MirageFS 运行时对象实例。
 *
 * obj_runtime_t 是所有模块共同引用的运行时锚点：
 *
 *   meta    : 对象元数据（key + handle），描述"对象是什么"
 *   refcnt  : 原子引用计数，由 objmgr 管理并发访问
 *   state   : 生命周期状态 (obj_state_t)，由 objmgr 驱动状态迁移
 *
 * 与 obj_meta_t 的关系：
 *   - obj_meta_t 描述对象固有属性（身份标识 + 后端定位）
 *   - obj_runtime_t 包裹 obj_meta_t，附加运行时生命周期信息
 *
 * Cache、Storage、Journal 等一级模块通过 obj_runtime_t *
 * 与对象建立关联，不直接修改 obj_meta_t。
 */
typedef struct obj_runtime {

    obj_meta_t       meta;   /* 对象元数据（key + handle） */
    fs_atomic32_t   refcnt; /* 引用计数，原子操作 */
    uint32_t         state; /* 生命周期状态，见 obj_state_t */

} obj_runtime_t;

/* ============================================================
 * 编译期检查
 * ============================================================ */

_Static_assert(sizeof(obj_runtime_t) == OBJRUNTIME_SIZE,
               "obj_runtime_t size invalid");

/* ============================================================
 * helper（内联）
 * ============================================================ */

/*
 * 读取对象生命周期状态。
 *
 * 封装 runtime->state 的直接访问，
 * 后续引入 Atomic/Barrier/RCU 时仅需修改此 getter。
 *
 * 参数：
 *      [IN] rt     : 运行时对象
 */
static inline obj_state_t objruntime_state(
                const obj_runtime_t *rt)
{
    return (obj_state_t)rt->state;
}

/* ============================================================
 * debug
 * ============================================================ */

/*
 * 打印 obj_runtime_t 完整信息。
 *
 * 输出 meta、refcnt、state 全部字段，
 * 用于 debug / trace / 日志分析。
 *
 * 参数：
 *      [IN] rt     : 待打印的运行时对象
 */
void objruntime_dump(
                const obj_runtime_t *rt);
