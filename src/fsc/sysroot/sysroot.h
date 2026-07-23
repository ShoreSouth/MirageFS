#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <common/fs_common.h>
#include <object/fuid/fuid.h>
#include <object/objmeta/objmeta.h>

/*
 * ============================================================
 * FSC Sysroot
 * ============================================================
 *
 * sysroot 是 MirageFS 运行期唯一的项目系统根目录，位于所有文件系统
 * 根目录之上。sysroot 下的一级目录应当只表示各个文件系统根目录，
 * 普通文件和普通目录仍归属于具体文件系统。
 *
 * 本子模块维护 sysroot 的独立运行态。它不进入 objtable，也不进入
 * fstable，避免把这个更高层级的全局锚点混入普通对象表或文件系统
 * 根目录表。
 *
 * sysroot 是 LSA 之上的唯一路径启动例外。启动完成后，上层模块仍应
 * 使用 FUID 和 obj_handle_t，不把 fd/path 扩散到非 LSA 模块接口。
 */

/* sysroot 绝对路径缓冲区大小。 */
#define FSC_SYSROOT_PATH_MAX 4096U

/*
 * sysroot 的保留 FUID。
 *
 * Fsid 使用全 1 作为 FSC 内部保留值，避免和 fsid 分配器产生的正常
 * 小整数 ID 冲突。ObjectId/GenId 固定为 1，因为 sysroot 全局唯一，
 * 生命周期也跟随 FSC 模块。
 */
#define FSC_SYSROOT_FSID ((Fsid_t)0xffffffffffffffffULL)
#define FSC_SYSROOT_OBJECT_ID ((ObjectId_t)1ULL)
#define FSC_SYSROOT_GEN ((GenId_t)1U)

/* sysroot 生命周期状态。 */
typedef enum fsc_sysroot_state
{
    FSC_SYSROOT_STATE_INVALID = 0,
    FSC_SYSROOT_STATE_INIT,
    FSC_SYSROOT_STATE_ACTIVE,
    FSC_SYSROOT_STATE_DELETING,
} fsc_sysroot_state_t;

/*
 * 初始化 sysroot。
 *
 * path 为 NULL 时使用默认路径 ./miragefs.root，并在内部转换为绝对
 * 路径。函数会委托 LSA 创建目录、获取 file handle，再保存为
 * obj_handle_t。
 */
fs_error_t fsc_sysroot_init(const char *path);

/* 销毁 sysroot 内存运行态。当前不会删除磁盘目录。 */
void fsc_sysroot_deinit(void);

/* 返回 sysroot 是否已经完成启动并处于 ACTIVE 状态。 */
bool fsc_sysroot_is_active(void);

/* 获取 sysroot 的保留 FUID。 */
fs_error_t fsc_sysroot_get_fuid(fuid_t *root_out);

/* 获取 sysroot 目录对应的对象 handle。 */
fs_error_t fsc_sysroot_get_handle(obj_handle_t *handle_out);

/* 获取 sysroot 绝对路径，仅用于诊断，不作为业务访问入口。 */
const char *fsc_sysroot_get_path(void);
