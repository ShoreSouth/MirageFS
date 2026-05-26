#pragma once

/*
 * MirageFS Object Metadata Layer
 *
 * ObjMeta 负责维护：
 *
 *      objectid -> backend object locator
 *
 * 当前阶段：
 * - 不维护 namespace
 * - 不维护 path
 * - 不维护 stat cache
 * - 不维护 hierarchy
 *
 * 仅作为：
 *      MirageFS object
 *          与
 *      Linux backend object
 *
 * 之间的桥梁。
 *
 * backend object 当前基于：
 *      mount_id + file_handle
 *
 * 实现稳定定位。
 */

#include <stdint.h>

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
 *      sizeof(FsObjMeta_t) == 32
 *
 * 便于：
 * - cache friendly
 * - mempool 管理
 * - KV 持久化
 */
#define FS_OBJMETA_SIZE 32

/* ============================================================
 * 核心结构
 * ============================================================ */

/*
 * FsObjMeta_t
 *
 * MirageFS 对象元数据。
 *
 * 作用：
 *      保存 objectid 对应的 backend object 定位信息。
 */
typedef struct ObjMeta {

    uint64_t objectid; /* MirageFS 内部对象唯一ID */

    int32_t mount_id; /* Linux mount id */

    uint16_t handle_type; /* Linux file_handle 类型 */

    uint16_t handle_bytes; /* file_handle 实际长度 */

    uint8_t file_handle[OBJMETA_MAX_HANDLE_SIZE]; /* Linux backend handle */

} ObjMeta_t;

/* ============================================================
 * 编译期检查
 * ============================================================ */

_Static_assert(sizeof(ObjMeta_t) == FS_OBJMETA_SIZE,
               "ObjMeta_t size invalid");

/* ============================================================
 * 对外接口
 * ============================================================ */

/*
 * 初始化 ObjMeta。
 *
 * 参数：
 *      meta            : 目标对象
 *      objectid        : MirageFS object id
 *      mount_id        : Linux mount id
 *      handle_type     : Linux handle type
 *      handle_bytes    : handle 实际长度
 *      file_handle     : handle 数据
 *
 * 返回：
 *      0       : success
 *      <0      : failed
 */
int32_t objmeta_init(ObjMeta_t *meta,
                uint64_t objectid,
                int32_t mount_id,
                uint16_t handle_type,
                uint16_t handle_bytes,
                const uint8_t *file_handle);

/*
 * 清空 ObjMeta。
 */
void objmeta_reset(ObjMeta_t *meta);

/*
 * 判断 ObjMeta 是否有效。
 *
 * 返回：
 *      1 : valid
 *      0 : invalid
 */
int32_t objmeta_is_valid(const ObjMeta_t *meta);

/*
 * 比较两个 ObjMeta 是否相同。
 *
 * 返回：
 *      1 : equal
 *      0 : not equal
 */
int32_t objmeta_equal(const ObjMeta_t *lhs,
                        const ObjMeta_t *rhs);

/*
 * 打印 ObjMeta 信息。
 *
 * 用于：
 * - debug
 * - trace
 * - 日志
 */
void objmeta_dump(const ObjMeta_t *meta);
