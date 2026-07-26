#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "fsc/namespace/namespace.h"
#include "fops/include/fops_types.h"
#include "namei/include/namei_types.h"
#include "object/fuid/fuid.h"
#include "object/objmeta/objmeta.h"

/*
 * ============================================================
 * Runtime 配置
 * ============================================================
 */

/*
 * runtime_config_t
 *
 * Runtime 启动配置。当前用于 V1 控制台启动阶段的默认 namespace
 * 创建/进入策略；未来 CLI、Web UI 或协议适配层可以复用该入口。
 */
typedef struct runtime_config
{
    const char *default_namespace; /* 默认 namespace 名称，可为 NULL */
    bool auto_create;              /* 启动时自动创建默认 namespace */
    bool auto_use;                 /* 启动时自动进入默认 namespace */

} runtime_config_t;

/*
 * ============================================================
 * Runtime 生命周期
 * ============================================================
 */

/*
 * 初始化 MirageFS 运行时。
 *
 * 本函数统一初始化 config、trace、log、LSA、Object、FSC、FOPS、NAMEI，
 * 并注册 Runtime sub-error 名称。入口层不应手工维护底层初始化顺序。
 *
 * 参数：
 *      [IN] cfg   : 启动配置，可为 NULL
 *
 * 返回：
 *      FS_OK      : 初始化成功
 *      fs_error_t : 初始化失败原因
 */
fs_error_t runtime_init(const runtime_config_t *cfg);

/*
 * 反初始化 MirageFS 运行时。
 *
 * 本函数会先退出当前 namespace 会话，再按初始化逆序释放底层模块。
 */
void runtime_deinit(void);

/* 返回 Runtime 是否已经初始化。 */
bool runtime_is_initialized(void);

/*
 * ============================================================
 * Namespace 会话管理
 * ============================================================
 */

/*
 * 创建 namespace。
 *
 * 参数：
 *      [IN]  name     : namespace 名称
 *      [OUT] root_out : 创建出的 root FUID，可为 NULL
 */
fs_error_t runtime_fs_create(const char *name, fuid_t *root_out);

/*
 * 销毁 namespace。
 *
 * 如果待销毁 namespace 是当前会话，Runtime 会先退出当前会话。
 * 当前 FSC 语义要求后端根目录为空。
 *
 * 参数：
 *      [IN] name : namespace 名称
 */
fs_error_t runtime_fs_destroy(const char *name);

/* 递归删除 namespace 内容后销毁 namespace。 */
fs_error_t runtime_fs_destroy_tree(const char *name);

/* 重命名 namespace。 */
fs_error_t runtime_fs_rename(const char *old_name, const char *new_name);

/*
 * 进入指定 namespace，并把 cwd 设置到 namespace root。
 *
 * 参数：
 *      [IN] name : namespace 名称
 */
fs_error_t runtime_fs_use(const char *name);

/* runtime_fs_use() 的新命名入口。 */
fs_error_t runtime_fs_enter(const char *name);

/* 退出当前 namespace 会话，清空 root/cwd FUID 和显示路径。 */
fs_error_t runtime_fs_leave(void);

/* 返回当前是否已经进入 namespace。 */
bool runtime_fs_is_active(void);

/* 返回当前 namespace 名称；未进入 namespace 时返回 NULL。 */
const char *runtime_fs_current(void);

/* 列出当前 runtime 中已注册的 namespace 名称。 */
fs_error_t runtime_fs_list(char names[][FSC_NAMESPACE_NAME_MAX], uint32_t cap,
                           uint32_t *actual_out);

/*
 * ============================================================
 * 当前工作目录与 NAMEI 上下文
 * ============================================================
 */

/*
 * 构造当前会话的 NAMEI 上下文。
 *
 * 参数：
 *      [OUT] out_ctx : 输出 root/cwd 组成的 namei_ctx_t
 */
fs_error_t runtime_get_ctx(namei_ctx_t *out_ctx);

/*
 * 获取当前会话 root FUID。
 *
 * 参数：
 *      [OUT] out_fuid : 输出 root FUID
 */
fs_error_t runtime_get_root(fuid_t *out_fuid);

/*
 * 获取当前会话 cwd FUID。
 *
 * 参数：
 *      [OUT] out_fuid : 输出 cwd FUID
 */
fs_error_t runtime_get_cwd(fuid_t *out_fuid);

/*
 * 获取控制台显示用 cwd 路径。
 *
 * 该路径只用于入口层展示和相对路径拼接参考；真实路径语义仍由 NAMEI
 * 根据 root_fuid/cwd_fuid 执行。
 *
 * 参数：
 *      [OUT] buf  : 输出缓冲区
 *      [IN]  size : 缓冲区大小
 */
fs_error_t runtime_getcwd(char *buf, size_t size);

/*
 * 切换当前工作目录。
 *
 * Runtime 会先通过 NAMEI 校验目标路径必须是目录，然后更新 cwd_fuid
 * 和控制台显示路径。
 *
 * 参数：
 *      [IN] path : 目标目录路径
 */
fs_error_t runtime_chdir(const char *path);

/*
 * ============================================================
 * 路径解析
 * ============================================================
 */

/*
 * 按路径查找对象 FUID。
 *
 * 参数：
 *      [IN]  path     : 目标路径
 *      [IN]  flags    : FS_FLAG_* 约束
 *      [OUT] out_fuid : 输出对象 FUID
 */
fs_error_t runtime_lookup(const char *path, fs_flags_t flags, fuid_t *out_fuid);

/* 按路径查找对象 FUID 和属性快照。 */
fs_error_t runtime_lookup_plus(const char *path, fs_flags_t flags,
                               fops_object_result_t *out);

/* 按路径解析父目录 FUID 和 leaf name。 */
fs_error_t runtime_lookup_parent(const char *path, fs_flags_t flags,
                                 namei_parent_result_t *out);

/*
 * ============================================================
 * 命名对象创建、删除与重命名
 * ============================================================
 */

/* 创建或复用普通文件。 */
fs_error_t runtime_create(const char *path, const fops_create_attr_t *attr,
                          fs_flags_t flags, fops_object_result_t *out);

/* 创建目录。 */
fs_error_t runtime_mkdir(const char *path, const fops_create_attr_t *attr,
                         fs_flags_t flags, fops_object_result_t *out);

/* 创建特殊对象，例如 FIFO、block device、char device。 */
fs_error_t runtime_mknod(const char *path, fs_type_t type,
                         const fops_create_attr_t *attr,
                         const fops_device_t *device, fs_flags_t flags,
                         fops_object_result_t *out);

/* 删除非目录对象。 */
fs_error_t runtime_unlink(const char *path, fs_flags_t flags);

/* 删除空目录。 */
fs_error_t runtime_rmdir(const char *path, fs_flags_t flags);

/* 重命名或移动对象。 */
fs_error_t runtime_rename(const char *old_path, const char *new_path,
                          fs_flags_t flags);

/* 创建硬链接。 */
fs_error_t runtime_link(const char *old_path, const char *new_path,
                        fs_flags_t flags, fops_object_result_t *out);

/* 创建符号链接。 */
fs_error_t runtime_symlink(const char *target, const char *linkpath,
                           fs_flags_t flags, fops_object_result_t *out);

/* 读取符号链接内容。 */
fs_error_t runtime_readlink(const char *path, fs_flags_t flags, char *buf,
                            size_t size, size_t *actual);

/*
 * ============================================================
 * 目录读取
 * ============================================================
 */

/* 读取目录项，不返回属性。 */
fs_error_t runtime_readdir(const char *path, fs_flags_t flags,
                           fops_dirent_t *entries, uint32_t entry_cap,
                           uint32_t *out_entry_nr, bool *out_eof);

/* 读取目录项，并返回属性。 */
fs_error_t runtime_readdirplus(const char *path, fs_flags_t flags,
                               fops_dirent_plus_t *entries, uint32_t entry_cap,
                               uint32_t *out_entry_nr, bool *out_eof);

/*
 * ============================================================
 * 属性、权限与大小
 * ============================================================
 */

/* 读取对象属性。 */
fs_error_t runtime_getattr(const char *path, fs_flags_t flags,
                           fops_attr_t *out_attr);

/* 读取对象 FUID 和最新属性快照。 */
fs_error_t runtime_stat(const char *path, fs_flags_t flags,
                        fops_object_result_t *out);

/* 修改对象属性。 */
fs_error_t runtime_setattr(const char *path, const fops_setattr_t *attr,
                           fs_flags_t flags);

/* 检查对象访问权限。 */
fs_error_t runtime_access(const char *path, int mask, fs_flags_t flags);

/* 修改对象大小。 */
fs_error_t runtime_truncate(const char *path, uint64_t size, fs_flags_t flags);

/*
 * ============================================================
 * 文件句柄与读写
 * ============================================================
 */

/* 打开路径对象，返回 opaque FOPS 文件句柄。 */
fs_error_t runtime_open(const char *path, fs_flags_t flags,
                        fops_file_t **out_file);

/* 关闭 Runtime/FOPS 文件句柄。 */
fs_error_t runtime_close(fops_file_t *file);

/* 从已打开文件句柄读取数据。 */
fs_error_t runtime_read(fops_file_t *file, void *buf, size_t size,
                        size_t *actual);

/* 向已打开文件句柄写入数据。 */
fs_error_t runtime_write(fops_file_t *file, const void *buf, size_t size,
                         size_t *actual);

/*
 * ============================================================
 * 扩展属性
 * ============================================================
 */

/* 读取扩展属性。 */
fs_error_t runtime_getxattr(const char *path, const char *name, void *value,
                            size_t size, size_t *actual);

/* 设置扩展属性。 */
fs_error_t runtime_setxattr(const char *path, const char *name,
                            const void *value, size_t size, fs_flags_t flags);

/* 列出扩展属性名称。 */
fs_error_t runtime_listxattr(const char *path, char *list, size_t size,
                             size_t *actual);

/* 删除扩展属性。 */
fs_error_t runtime_removexattr(const char *path, const char *name);

/*
 * ============================================================
 * 文件系统级操作与适配器辅助
 * ============================================================
 */

/* 读取文件系统统计信息。 */
fs_error_t runtime_statfs(const char *path, fops_statfs_t *out_statfs);

/* 同步对象所在文件系统。 */
fs_error_t runtime_syncfs(const char *path);

/* 复制路径对象的后端 handle。 */
fs_error_t runtime_gethandle(const char *path, obj_handle_t *out_handle);

/* 通过已注册后端 handle 打开对象。 */
fs_error_t runtime_openhandle(const obj_handle_t *handle, fs_flags_t flags,
                              fops_file_t **out_file);
