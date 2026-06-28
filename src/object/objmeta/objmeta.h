#pragma once

/*
 * MirageFS 对象元数据层
 *
 * ObjMeta 负责对象定位映射：
 *
 *      (objectid, gen) → backend object locator
 *
 * 仅描述对象固有属性（身份标识 + 后端定位），
 * 不包含运行时生命周期信息（refcnt、state 等由 obj_runtime_t 管理）。
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
 * ObjMeta 结构固定大小。
 *
 *      sizeof(obj_meta_t) == 40
 *
 * 便于：
 * - cache friendly
 * - mempool 管理
 * - KV 持久化
 *
 * 运行时生命周期信息（refcnt、state）由 obj_runtime_t 管理，
 * sizeof(obj_runtime_t) == 48。
 */
#define OBJMETA_SIZE 40

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
 * MirageFS 对象元数据 —— 描述对象固有属性。
 *
 *   key     : MirageFS 对象唯一标识 (objectid + gen)
 *   handle  : Linux backend handle，封装 mount_id + file_handle
 *
 * obj_meta_t 仅描述"对象是什么"，不包含运行时状态。
 * 运行时生命周期信息（refcnt、state）由 obj_runtime_t 管理。
 */
typedef struct obj_meta {

    obj_key_t       key;    /* MirageFS 对象唯一标识 */
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
 * 写入 key + backend handle。
 * 调用者需通过 objmgr 初始化 runtime 并管理引用计数。
 *
 * 参数：
 *      [OUT] meta      : 目标对象（由 objpool_alloc 分配）
 *      [IN]  fuid      : MirageFS 对象标识
 *      [IN]  handle    : Linux backend handle
 *
 * 返回：
 *      FS_OK           : 成功
 *      >0              : 失败（fs_error_t）
 */
int32_t objmeta_init(
                obj_meta_t *meta,
                const fuid_t *fuid,
                const obj_handle_t *handle);

/*
 * 清空 ObjMeta。
 *
 * 将整个结构置零（包括 key 和 handle），
 * 等价于回到未初始化状态。
 *
 * 参数：
 *      [OUT] meta  : 目标对象（内容将被清零）
 */
void objmeta_reset(
                obj_meta_t *meta);

/*
 * 判断 ObjMeta 是否有效。
 *
 * 参数：
 *      [IN] meta   : 待检查的对象元数据
 */
bool objmeta_is_valid(
                const obj_meta_t *meta);

/*
 * 比较两个 ObjMeta 是否相同。
 *
 * 参数：
 *      [IN] lhs    : 左操作数
 *      [IN] rhs    : 右操作数
 */
bool objmeta_equal(
                const obj_meta_t *lhs,
                const obj_meta_t *rhs);

/*
 * 打印 ObjMeta 信息。
 *
 * 输出 key、handle 全部字段，
 * 用于 debug / trace / 日志分析。
 *
 * 注意：refcnt 和 state 由 objruntime_dump() 输出。
 *
 * 参数：
 *      [IN] meta   : 待打印的对象元数据
 */
void objmeta_dump(
                const obj_meta_t *meta);
