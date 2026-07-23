#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "fops/include/fops_types.h"
#include "object/objmeta/objmeta.h"

/*
 * 初始化 FOPS 模块状态，并注册 FOPS 子错误名。
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 失败原因
 */
fs_error_t fops_init(void);

/*
 * FOPS 统一调度入口。
 *
 * 上层可以只依赖本接口完成所有 FOPS OP 调用。args->op 指定操作字，
 * args 公共字段和 union 分支携带对应 OP 的参数。
 *
 * 参数：
 *      [IN/OUT] args : 统一 OP 参数对象，输出字段也写回该对象引用的缓冲区
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 参数错误、flag 不支持或底层操作失败
 */
fs_error_t fops_dispatch(fops_args_t *args);

/*
 * 反初始化 FOPS 模块状态。
 *
 * 调用方必须保证没有 FOPS 操作仍在运行。
 */
void fops_deinit(void);

/*
 * 在父目录下 lookup 一个名字，并返回子对象 FUID。
 *
 * 参数：
 *      [IN]  parent_fuid : 父目录 FUID
 *      [IN]  name        : 单级路径分量，允许 "." 和 ".."
 *      [IN]  flags       : FS_FLAG_NOFOLLOW / DIRECTORY / REGULAR
 *      [OUT] out_fuid    : 解析出的子对象 FUID
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 参数错误、类型不匹配或后端错误
 */
fs_error_t fops_lookup(const fuid_t *parent_fuid, const char *name,
                       fs_flags_t flags, fuid_t *out_fuid);

/*
 * lookup 一个名字，并返回 FUID 和属性。
 *
 * 参数：
 *      [IN]  parent_fuid : 父目录 FUID
 *      [IN]  name        : 单级路径分量，允许 "." 和 ".."
 *      [IN]  flags       : FS_FLAG_NOFOLLOW / DIRECTORY / REGULAR
 *      [OUT] out         : 解析出的 FUID 和属性快照
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 参数错误、类型不匹配或后端错误
 */
fs_error_t fops_lookup_plus(const fuid_t *parent_fuid, const char *name,
                            fs_flags_t flags, fops_object_result_t *out);

/*
 * 创建或复用普通文件，并返回 FUID。
 *
 * 参数：
 *      [IN]  parent_fuid : 父目录 FUID
 *      [IN]  name        : 新文件名，拒绝 "." 和 ".."
 *      [IN]  attr        : 创建时属性，NULL 表示使用默认值
 *      [IN]  flags       : REPLACE / EXCLUSIVE / TRUNCATE / NOFOLLOW /
 *                          SYNC / DIRECT / APPEND / REGULAR
 *      [OUT] out_fuid    : 创建或复用后的文件 FUID
 *
 * 返回：
 *      FS_OK       : 成功
 *      fs_error_t  : 参数错误、flag 冲突、类型不匹配或后端错误
 */
fs_error_t fops_create(const fuid_t *parent_fuid, const char *name,
                       const fops_create_attr_t *attr, fs_flags_t flags,
                       fuid_t *out_fuid);

/*
 * 创建或复用普通文件，并返回 FUID 和属性。
 *
 * 参数：
 *      [IN]  parent_fuid : 父目录 FUID
 *      [IN]  name        : 新文件名，拒绝 "." 和 ".."
 *      [IN]  attr        : 创建时属性，NULL 表示使用默认值
 *      [IN]  flags       : same as fops_create()
 *      [OUT] out         : 创建/复用后的 FUID 和属性
 */
fs_error_t fops_create_plus(const fuid_t *parent_fuid, const char *name,
                            const fops_create_attr_t *attr, fs_flags_t flags,
                            fops_object_result_t *out);

/*
 * 创建目录，并返回 FUID。
 *
 * 参数：
 *      [IN]  parent_fuid : 父目录 FUID
 *      [IN]  name        : 新目录名，拒绝 "." 和 ".."
 *      [IN]  attr        : 创建时属性，NULL 表示使用默认值;
 *                          SIZE is not supported for mkdir
 *      [IN]  flags       : FS_FLAG_EXCLUSIVE / DIRECTORY / NONE
 *      [OUT] out_fuid    : 创建出的目录 FUID
 */
fs_error_t fops_mkdir(const fuid_t *parent_fuid, const char *name,
                      const fops_create_attr_t *attr, fs_flags_t flags,
                      fuid_t *out_fuid);

/*
 * 创建目录，并返回 FUID 和属性。
 *
 * 参数：
 *      [IN]  parent_fuid : 父目录 FUID
 *      [IN]  name        : 新目录名，拒绝 "." 和 ".."
 *      [IN]  attr        : 创建时属性，NULL 表示使用默认值
 *      [IN]  flags       : 同 fops_mkdir()
 *      [OUT] out         : 创建出的目录 FUID and post-create attributes
 */
fs_error_t fops_mkdir_plus(const fuid_t *parent_fuid, const char *name,
                           const fops_create_attr_t *attr, fs_flags_t flags,
                           fops_object_result_t *out);

/*
 * 读取已有 FUID 的属性。
 *
 * 参数：
 *      [IN]  fuid     : 对象 FUID
 *      [IN]  flags    : FS_FLAG_DIRECTORY / REGULAR / NONE
 *      [OUT] out_attr : 属性快照
 */
fs_error_t fops_getattr(const fuid_t *fuid, fs_flags_t flags,
                        fops_attr_t *out_attr);

/*
 * 读取目录项，不返回属性。
 *
 * 参数：
 *      [IN]  dir_fuid     : 目录 FUID
 *      [IN]  flags        : FS_FLAG_DIRECTORY / NONE
 *      [OUT] entries      : 调用方提供的目录项数组
 *      [IN]  entry_cap    : entries 数组容量
 *      [OUT] out_entry_nr : 实际写入的目录项数量
 *      [OUT] out_eof      : 迭代器到达后端 EOF 时为 true
 */
fs_error_t fops_readdir(const fuid_t *dir_fuid, fs_flags_t flags,
                        fops_dirent_t *entries, uint32_t entry_cap,
                        uint32_t *out_entry_nr, bool *out_eof);

/*
 * 读取目录项，并返回属性。
 *
 * 参数：
 *      [IN]  dir_fuid     : 目录 FUID
 *      [IN]  flags        : FS_FLAG_DIRECTORY / NONE
 *      [OUT] entries      : 调用方提供的目录项+属性数组
 *      [IN]  entry_cap    : entries 数组容量
 *      [OUT] out_entry_nr : 实际写入的目录项数量
 *      [OUT] out_eof      : 迭代器到达后端 EOF 时为 true
 */
fs_error_t fops_readdirplus(const fuid_t *dir_fuid, fs_flags_t flags,
                            fops_dirent_plus_t *entries, uint32_t entry_cap,
                            uint32_t *out_entry_nr, bool *out_eof);

/*
 * 删除非目录子对象。
 *
 * 参数：
 *      [IN] parent_fuid : 父目录 FUID
 *      [IN] name        : 子对象名，拒绝 "." 和 ".."
 *      [IN] flags       : FS_FLAG_NOFOLLOW / REGULAR / NONE
 */
fs_error_t fops_unlink(const fuid_t *parent_fuid, const char *name,
                       fs_flags_t flags);

/*
 * 删除空目录子对象。
 *
 * 参数：
 *      [IN] parent_fuid : 父目录 FUID
 *      [IN] name        : 子对象名，拒绝 "." 和 ".."
 *      [IN] flags       : FS_FLAG_DIRECTORY / NONE
 */
fs_error_t fops_rmdir(const fuid_t *parent_fuid, const char *name,
                      fs_flags_t flags);

/*
 * 在两个父目录之间重命名一个子对象。
 *
 * 参数：
 *      [IN] old_parent_fuid : source 父目录 FUID
 *      [IN] old_name        : 源子对象名
 *      [IN] new_parent_fuid : destination 父目录 FUID
 *      [IN] new_name        : 目标子对象名
 *      [IN] flags           : FS_FLAG_REPLACE / EXCLUSIVE / NONE
 */
fs_error_t fops_rename(const fuid_t *old_parent_fuid, const char *old_name,
                       const fuid_t *new_parent_fuid, const char *new_name,
                       fs_flags_t flags);

/*
 * 创建硬链接，并返回被链接对象的 FUID。
 *
 * 参数：
 *      [IN]  old_parent_fuid : source 父目录 FUID
 *      [IN]  old_name        : 已有普通文件名
 *      [IN]  new_parent_fuid : destination 父目录 FUID
 *      [IN]  new_name        : 新链接名，必须不存在
 *      [IN]  flags           : FS_FLAG_EXCLUSIVE / NONE
 *      [OUT] out_fuid        : linked 对象 FUID
 */
fs_error_t fops_link(const fuid_t *old_parent_fuid, const char *old_name,
                     const fuid_t *new_parent_fuid, const char *new_name,
                     fs_flags_t flags, fuid_t *out_fuid);

/* 与 fops_link() 相同，但额外返回链接后的属性。 */
fs_error_t fops_link_plus(const fuid_t *old_parent_fuid, const char *old_name,
                          const fuid_t *new_parent_fuid, const char *new_name,
                          fs_flags_t flags, fops_object_result_t *out);

/*
 * 创建符号链接，并返回 FUID。
 *
 * 参数：
 *      [IN]  parent_fuid : 父目录 FUID
 *      [IN]  name        : 符号链接名，必须不存在
 *      [IN]  target      : 符号链接内容
 *      [IN]  flags       : FS_FLAG_EXCLUSIVE / NONE
 *      [OUT] out_fuid    : 符号链接 FUID
 */
fs_error_t fops_symlink(const fuid_t *parent_fuid, const char *name,
                        const char *target, fs_flags_t flags, fuid_t *out_fuid);

/* 与 fops_symlink() 相同，但额外返回符号链接属性。 */
fs_error_t fops_symlink_plus(const fuid_t *parent_fuid, const char *name,
                             const char *target, fs_flags_t flags,
                             fops_object_result_t *out);

/*
 * 读取符号链接内容。
 *
 * 参数：
 *      [IN]  parent_fuid : 符号链接所在父目录 FUID
 *      [IN]  name        : 符号链接名称
 *      [IN]  flags       : FS_FLAG_NOFOLLOW / NONE
 *      [OUT] buf         : 调用方提供的缓冲区
 *      [IN]  size        : 缓冲区容量
 *      [OUT] actual      : 实际读取字节数，不含结尾 NUL
 */
fs_error_t fops_readlink(const fuid_t *parent_fuid, const char *name,
                         fs_flags_t flags, char *buf, size_t size,
                         size_t *actual);

/*
 * 创建特殊对象，并返回 FUID。
 *
 * 参数：
 *      [IN]  req      : mknod 创建请求
 *      [OUT] out_fuid : created 对象 FUID
 */
fs_error_t fops_mknod(const fops_mknod_req_t *req, fuid_t *out_fuid);

/*
 * 与 fops_mknod() 相同，但额外返回创建后的属性。
 *
 * 参数：
 *      [IN]  req : mknod 创建请求
 *      [OUT] out : 创建成功后的 FUID 和属性
 */
fs_error_t fops_mknod_plus(const fops_mknod_req_t *req,
                           fops_object_result_t *out);

/*
 * 修改对象属性。
 *
 * 参数：
 *      [IN] fuid  : target 对象 FUID
 *      [IN] attr  : 属性修改请求
 *      [IN] flags : FS_FLAG_DIRECTORY / REGULAR / NONE type constraint
 */
fs_error_t fops_setattr(const fuid_t *fuid, const fops_setattr_t *attr,
                        fs_flags_t flags);

/*
 * 使用 R_OK/W_OK/X_OK/F_OK 风格的 mask 检查对象访问权限。
 *
 * 参数：
 *      [IN] fuid  : target 对象 FUID
 *      [IN] mask  : POSIX access mask
 *      [IN] flags : FS_FLAG_DIRECTORY / REGULAR / NONE type constraint
 */
fs_error_t fops_access(const fuid_t *fuid, int mask, fs_flags_t flags);

/*
 * 复制指定 FUID 已注册的后端对象 handle。
 *
 * 参数：
 *      [IN]  fuid       : target 对象 FUID
 *      [OUT] out_handle : 后端 handle 副本
 */
fs_error_t fops_gethandle(const fuid_t *fuid, obj_handle_t *out_handle);

/*
 * 打开对象，并返回 opaque FOPS 文件句柄。
 *
 * 参数：
 *      [IN]  fuid     : target 对象 FUID
 *      [IN]  flags    : READ / WRITE / APPEND / TRUNCATE / SYNC / DIRECT /
 *                       DIRECTORY / REGULAR
 *      [OUT] out_file : 打开后的 FOPS 文件句柄; close with fops_close()
 */
fs_error_t fops_open(const fuid_t *fuid, fs_flags_t flags,
                     fops_file_t **out_file);

/*
 * 打开一个已注册的后端 handle，并返回 FOPS 文件句柄。
 *
 * 参数：
 *      [IN]  handle   : 已注册到 ObjMgr 的后端 handle
 *      [IN]  flags    : 同 fops_open()
 *      [OUT] out_file : 打开后的 FOPS 文件句柄
 */
fs_error_t fops_openhandle(const obj_handle_t *handle, fs_flags_t flags,
                           fops_file_t **out_file);

/*
 * 关闭由 fops_open/openhandle 返回的 FOPS 文件句柄。
 *
 * 参数：
 *      [IN/OUT] file : FOPS 文件句柄；调用后失效
 */
fs_error_t fops_close(fops_file_t *file);

fs_error_t fops_read(fops_file_t *file, void *buf, size_t size, size_t *actual);

fs_error_t fops_write(fops_file_t *file, const void *buf, size_t size,
                      size_t *actual);

fs_error_t fops_pread(fops_file_t *file, void *buf, size_t size, off_t offset,
                      size_t *actual);

fs_error_t fops_pwrite(fops_file_t *file, const void *buf, size_t size,
                       off_t offset, size_t *actual);

fs_error_t fops_truncate(const fuid_t *fuid, uint64_t size, fs_flags_t flags);

fs_error_t fops_getxattr(const fuid_t *fuid, const char *name, void *value,
                         size_t size, size_t *actual);

fs_error_t fops_setxattr(const fuid_t *fuid, const char *name,
                         const void *value, size_t size, fs_flags_t flags);

fs_error_t fops_listxattr(const fuid_t *fuid, char *list, size_t size,
                          size_t *actual);

fs_error_t fops_removexattr(const fuid_t *fuid, const char *name);

fs_error_t fops_statfs(const fuid_t *fuid, fops_statfs_t *out_statfs);

fs_error_t fops_syncfs(const fuid_t *fuid);
