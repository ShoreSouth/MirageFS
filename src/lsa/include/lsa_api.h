#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "lsa/internal/lsa_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ============================================================
 * Basic Types
 * ============================================================
 */

typedef fs_error_t lsa_ret_t;

/*
 * ============================================================
 * Directory Cookie
 * ============================================================
 */

typedef struct lsa_dir_cookie {

    uint64_t value;

} lsa_dir_cookie_t;

/*
 * ============================================================
 * Directory Entry
 * ============================================================
 */

typedef struct lsa_dirent {

    char name[NAME_MAX + 1];

    uint64_t ino;

    fs_type_t type;

} lsa_dirent_t;

/*
 * ============================================================
 * Directory Entry Plus
 * ============================================================
 */

typedef struct lsa_dirent_plus {

    lsa_dirent_t entry;

    struct stat st;

} lsa_dirent_plus_t;

/*
 * ============================================================
 * Directory Iterator (opaque handle)
 * ============================================================
 */

typedef struct lsa_dir_iter lsa_dir_iter_t;

/*
 * ============================================================
 * device identifier
 *
 * used by:
 *      lsa_mknod()
 *
 * valid for:
 *      FS_TYPE_BLK
 *      FS_TYPE_CHR
 * ============================================================
 */
typedef struct lsa_device {

    uint32_t major_id;

    uint32_t minor_id;

} lsa_device_t;

/*
 * ============================================================
 * Handle Operations
 * ============================================================
 *
 * pathname
 *      ↓
 * file_handle
 *
 * file_handle
 *      ↓
 * fd
 */

typedef struct lsa_file_handle {

    uint32_t handle_bytes;

    int32_t handle_type;

    uint8_t data[LSA_HANDLE_MAX_SIZE];

} lsa_file_handle_t;

lsa_ret_t lsa_name_to_handle_at(
                int dirfd,
                const char *path,
                lsa_file_handle_t *handle,
                int32_t *mount_id,
                int flags);

lsa_ret_t lsa_open_by_handle_at(
                int mount_fd,
                const lsa_file_handle_t *handle,
                int flags,
                int *fd);

/*
 * 通过 LSA 内部维护的 mount id 打开 file handle。
 *
 * 非 LSA 模块不应保存 mount fd。启动阶段由 lsa_bootstrap_root()
 * 注册 mount id 与 mount fd 的映射，后续上层只携带 mount id + handle，
 * 由本函数在 LSA 内部解析并临时打开对象。
 */
lsa_ret_t lsa_open_by_handle_id(
                int32_t mount_id,
                const lsa_file_handle_t *handle,
                int flags,
                int *fd);

/* 释放 LSA 内部为指定 mount id 保存的 mount fd。 */
lsa_ret_t lsa_release_mount(
                int32_t mount_id);

/*
 * 通过路径启动全局唯一的 FSC sysroot。
 *
 * 这是系统根锚点的窄边界例外，只应在启动阶段使用。
 * 非 LSA 模块不应在此边界之外暴露 fd/path，调用方应保存并复用
 * 返回的 file handle。
 */
lsa_ret_t lsa_bootstrap_root(
                const char *path,
                lsa_file_handle_t *handle,
                int32_t *mount_id);

/*
 * ============================================================
 * Namespace Operations
 * ============================================================
 */

lsa_ret_t lsa_lookup(
                int dirfd,
                const char *name,
                fs_flags_t flags,
                int *fd);

lsa_ret_t lsa_create(
                int dirfd,
                const char *name,
                fs_flags_t flags,
                mode_t mode,
                int *fd);

lsa_ret_t lsa_mkdir(
                int dirfd,
                const char *name,
                fs_flags_t flags,
                mode_t mode);

lsa_ret_t lsa_unlink(
                int dirfd,
                const char *name,
                fs_flags_t flags);

lsa_ret_t lsa_rmdir(
                int dirfd,
                const char *name,
                fs_flags_t flags);

lsa_ret_t lsa_rename(
                int old_dirfd,
                const char *old_name,
                int new_dirfd,
                const char *new_name,
                fs_flags_t flags);

lsa_ret_t lsa_link(
                int old_dirfd,
                const char *old_name,
                int new_dirfd,
                const char *new_name,
                fs_flags_t flags);

lsa_ret_t lsa_symlink(
                const char *target,
                int dirfd,
                const char *name,
                fs_flags_t flags);

lsa_ret_t lsa_readlink(
                int dirfd,
                const char *name,
                char *buf,
                size_t size,
                size_t *actual);

lsa_ret_t lsa_mknod(
                int dirfd,
                const char *name,
                fs_type_t type,
                mode_t mode,
                const lsa_device_t *device);

/*
 * ============================================================
 * File Operations
 * ============================================================
 */

lsa_ret_t lsa_close(
                int fd);

lsa_ret_t lsa_read(
                int fd,
                void *buf,
                size_t size,
                size_t *actual);

/*
 * 完整读取 size 字节。
 *
 * lsa_read() 是单次 read(2) 封装，允许 partial IO；
 * lsa_read_full() 会循环读取，直到读满、EOF 或出错。
 * EOF 不视为错误，actual 返回实际读取字节数。
 */
lsa_ret_t lsa_read_full(
                int fd,
                void *buf,
                size_t size,
                size_t *actual);

lsa_ret_t lsa_write(
                int fd,
                const void *buf,
                size_t size,
                size_t *actual);

/*
 * 完整写入 size 字节。
 *
 * lsa_write() 是单次 write(2) 封装，允许 partial IO；
 * lsa_write_full() 会循环写入，直到写满或出错。
 */
lsa_ret_t lsa_write_full(
                int fd,
                const void *buf,
                size_t size,
                size_t *actual);

lsa_ret_t lsa_pread(
                int fd,
                void *buf,
                size_t size,
                off_t offset,
                size_t *actual);

lsa_ret_t lsa_pwrite(
                int fd,
                const void *buf,
                size_t size,
                off_t offset,
                size_t *actual);

lsa_ret_t lsa_lseek(
                int fd,
                off_t offset,
                int whence,
                off_t *new_offset);

lsa_ret_t lsa_fsync(
                int fd);

lsa_ret_t lsa_ftruncate(
                int fd,
                off_t length);

/*
 * ============================================================
 * Metadata Operations
 * ============================================================
 */

lsa_ret_t lsa_fstat(
                int fd,
                struct stat *st);

lsa_ret_t lsa_fstatat(
                int dirfd,
                const char *path,
                fs_flags_t flags,
                struct stat *st);

lsa_ret_t lsa_fchmod(
                int fd,
                mode_t mode);

lsa_ret_t lsa_fchown(
                int fd,
                uid_t uid,
                gid_t gid);

lsa_ret_t lsa_faccess(
                int fd,
                int mode);

/*
 * ============================================================
 * Directory Iterator API
 * ============================================================
 */

/*
 * 打开目录迭代器。
 *
 * dirfd 为借用句柄，迭代器不会接管所有权；调用方仍负责关闭
 * dirfd。lsa_dir_iter_close() 只释放 iterator 自身和内部缓冲区。
 */
lsa_ret_t lsa_dir_iter_open(
                int dirfd,
                uint32_t buffer_size,
                lsa_dir_iter_t **iter_out);

lsa_ret_t lsa_dir_iter_close(
                lsa_dir_iter_t *iter);

lsa_ret_t lsa_dir_iter_next(
                lsa_dir_iter_t *iter,
                lsa_dirent_t *entry);

lsa_ret_t lsa_dir_iter_next_plus(
                lsa_dir_iter_t *iter,
                lsa_dirent_plus_t *entry);

lsa_dir_cookie_t lsa_dir_iter_get_cookie(
                const lsa_dir_iter_t *iter);

lsa_ret_t lsa_dir_iter_seek(
                lsa_dir_iter_t *iter,
                lsa_dir_cookie_t cookie);

/*
 * ============================================================
 * Extended Attribute Operations
 * ============================================================
 */

lsa_ret_t lsa_setxattr(
                int fd,
                const char *name,
                const void *value,
                size_t size,
                fs_flags_t flags);

lsa_ret_t lsa_getxattr(
                int fd,
                const char *name,
                void *value,
                size_t size,
                size_t *actual);

lsa_ret_t lsa_listxattr(
                int fd,
                char *list,
                size_t size,
                size_t *actual);

lsa_ret_t lsa_removexattr(
                int fd,
                const char *name);

/*
 * ============================================================
 * Filesystem Operations
 * ============================================================
 */

lsa_ret_t lsa_statfs(
                int fd,
                struct statfs *st);

lsa_ret_t lsa_syncfs(
                int fd);

/*
 * ============================================================
 * Space Management
 * ============================================================
 */

lsa_ret_t lsa_fallocate(
                int fd,
                int mode,
                off_t offset,
                off_t len);

lsa_ret_t lsa_unmap(
                int fd,
                off_t offset,
                off_t len);

/*
 * ============================================================
 * 初始化
 * ============================================================
 */

void lsa_init(void);

#ifdef __cplusplus
}
#endif
