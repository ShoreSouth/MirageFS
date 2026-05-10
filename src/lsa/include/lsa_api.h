#ifndef MIRAGEFS_LSA_API_H
#define MIRAGEFS_LSA_API_H

#define _GNU_SOURCE
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 基础类型定义
 * ============================================================ */

typedef int lsa_ret_t;

/* 通用返回码 */
#define LSA_OK              0
#define LSA_ERR_GENERIC    -1

/* ============================================================
 * 文件类型（统一抽象）
 * ============================================================ */

typedef enum {
    LSA_TYPE_REG = 0,
    LSA_TYPE_DIR,
    LSA_TYPE_SYMLINK,
    LSA_TYPE_CHR,
    LSA_TYPE_BLK,
    LSA_TYPE_FIFO,
    LSA_TYPE_SOCKET,
    LSA_TYPE_UNKNOWN
} lsa_file_type_t;

/* ============================================================
 * 文件句柄结构
 * ============================================================ */

#define LSA_MAX_FH_SIZE 128

typedef struct {
    uint32_t handle_bytes;
    int32_t  handle_type;
    uint8_t  data[LSA_MAX_FH_SIZE];
} lsa_file_handle_t;

/* ============================================================
 * 目录项结构
 * ============================================================ */

#define LSA_NAME_MAX 256

typedef struct {
    char name[LSA_NAME_MAX];
    lsa_file_type_t type;
} lsa_dirent_t;

/* ============================================================
 * 文件IO操作
 * ============================================================ */

lsa_ret_t lsa_open(const char *path, int flags, mode_t mode, int *out_fd);

lsa_ret_t lsa_open_at(int dirfd, const char *path, int flags,
    mode_t mode, int *out_fd);

lsa_ret_t lsa_open_by_handle_at(int mount_fd, struct file_handle *handle, 
    int flags, int *out_fd);

lsa_ret_t lsa_name_to_handle_at(int dirfd, const char *path, 
    struct file_handle *handle, int *mount_id, int flags);

lsa_ret_t lsa_close(int fd);

lsa_ret_t lsa_read(int fd, void *buf, size_t len, ssize_t *out_size);

lsa_ret_t lsa_write(int fd, const void *buf, size_t len, ssize_t *out_size);

lsa_ret_t lsa_pread(int fd, void *buf, size_t len, 
    off_t offset, ssize_t *out_size);

lsa_ret_t lsa_pwrite(int fd, const void *buf, size_t len, 
    off_t offset, ssize_t *out_size);

lsa_ret_t lsa_fsync(int fd);

/* ============================================================
 * 目录操作
 * ============================================================ */

lsa_ret_t lsa_mkdir(const char *path, mode_t mode);

lsa_ret_t lsa_rmdir(const char *path);

lsa_ret_t lsa_readdir(const char *path,
                      lsa_dirent_t *out_list,
                      int max_entries,
                      int *out_count);

lsa_ret_t lsa_readdirplus(const char *path,
                          lsa_dirent_t *entries,
                          struct stat *stats,
                          int max_entries,
                          int *out_count);

/* ============================================================
 * 文件创建（含特殊类型）
 * ============================================================ */

lsa_ret_t lsa_create(const char *path, mode_t mode);

lsa_ret_t lsa_create_at(int dirfd, const char *path, mode_t mode);

/* 通用 mknod */
lsa_ret_t lsa_mknod(const char *path, mode_t mode, dev_t dev);

/* 语义封装 */
lsa_ret_t lsa_mkfifo(const char *path, mode_t mode);

lsa_ret_t lsa_mkchr(const char *path, mode_t mode, dev_t dev);

lsa_ret_t lsa_mkblk(const char *path, mode_t mode, dev_t dev);

lsa_ret_t lsa_mksocket(const char *path, mode_t mode);

/* ============================================================
 * 路径关系操作
 * ============================================================ */

lsa_ret_t lsa_unlink(const char *path);

lsa_ret_t lsa_rename(const char *oldpath, const char *newpath);

lsa_ret_t lsa_link(const char *oldpath, const char *newpath);

lsa_ret_t lsa_symlink(const char *target, const char *linkpath);

lsa_ret_t lsa_readlink(const char *path, char *buf, size_t buf_size);

/* fd → path（非标准能力） */
lsa_ret_t lsa_getpath(int fd, char *buf, size_t buf_size);

/* ============================================================
 * 元数据操作
 * ============================================================ */

lsa_ret_t lsa_stat(const char *path, struct stat *st);

lsa_ret_t lsa_lstat(const char *path, struct stat *st);

lsa_ret_t lsa_fstat(int fd, struct stat *st);

/* setattr（拆分接口） */
lsa_ret_t lsa_chmod(const char *path, mode_t mode);

lsa_ret_t lsa_chown(const char *path, uid_t uid, gid_t gid);

lsa_ret_t lsa_truncate(const char *path, off_t length);

lsa_ret_t lsa_ftruncate(int fd, off_t length);

/* ============================================================
 * 扩展属性（xattr）
 * ============================================================ */

lsa_ret_t lsa_setxattr(const char *path,
                       const char *name,
                       const void *value,
                       size_t size,
                       int flags);

lsa_ret_t lsa_getxattr(const char *path,
                       const char *name,
                       void *value,
                       size_t size,
                       ssize_t *out_size);

lsa_ret_t lsa_listxattr(const char *path,
                        char *list,
                        size_t size,
                        ssize_t *out_size);

lsa_ret_t lsa_removexattr(const char *path,
                          const char *name);

/* ============================================================
 * 文件系统级操作
 * ============================================================ */

lsa_ret_t lsa_statfs(const char *path, struct statfs *fsinfo);

/* ============================================================
 * 空间管理 / 高级IO
 * ============================================================ */

/* 预分配 */
lsa_ret_t lsa_fallocate(int fd, int mode, off_t offset, off_t len);

/* 打洞（unmap） */
lsa_ret_t lsa_unmap(int fd, off_t offset, off_t len);

/* ============================================================
 * 其他辅助
 * ============================================================ */

lsa_ret_t lsa_access(const char *path, int mode);

lsa_ret_t lsa_dup(int fd, int *out_fd);

lsa_ret_t lsa_lseek(int fd, off_t offset, int whence, off_t *out_offset);

/* ============================================================
 * 错误处理（内部实现）
 * ============================================================ */

lsa_ret_t lsa_errno_map(int err);

#ifdef __cplusplus
}
#endif

#endif /* MIRAGEFS_LSA_API_H */