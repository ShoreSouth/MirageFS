#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>

#include "common/fs_common.h"

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
 * Directory Iterator
 * ============================================================
 */

typedef struct lsa_dir_iter {

    int dirfd;

    lsa_dir_cookie_t cookie;

    bool eof;

    void *private_data;

} lsa_dir_iter_t;

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

lsa_ret_t lsa_name_to_handle_at(
                int dirfd,
                const char *path,
                struct file_handle *handle,
                int *mount_id,
                int flags);

lsa_ret_t lsa_open_by_handle_at(
                int mount_fd,
                struct file_handle *handle,
                int flags,
                int *out_fd);

/*
 * ============================================================
 * Namespace Operations
 * ============================================================
 */

lsa_ret_t lsa_openat(
                int dirfd,
                const char *path,
                int flags,
                mode_t mode,
                int *out_fd);

lsa_ret_t lsa_createat(
                int dirfd,
                const char *path,
                mode_t mode,
                int *out_fd);

lsa_ret_t lsa_mkdirat(
                int dirfd,
                const char *path,
                mode_t mode);

lsa_ret_t lsa_mknodat(
                int dirfd,
                const char *path,
                mode_t mode,
                dev_t dev);

lsa_ret_t lsa_mkfifoat(
                int dirfd,
                const char *path,
                mode_t mode);

lsa_ret_t lsa_mkchrat(
                int dirfd,
                const char *path,
                mode_t mode,
                dev_t dev);

lsa_ret_t lsa_mkblkat(
                int dirfd,
                const char *path,
                mode_t mode,
                dev_t dev);

lsa_ret_t lsa_symlinkat(
                const char *target,
                int newdirfd,
                const char *linkpath);

lsa_ret_t lsa_linkat(
                int olddirfd,
                const char *oldpath,
                int newdirfd,
                const char *newpath,
                int flags);

lsa_ret_t lsa_renameat(
                int olddirfd,
                const char *oldpath,
                int newdirfd,
                const char *newpath);

lsa_ret_t lsa_unlinkat(
                int dirfd,
                const char *path,
                int flags);

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
                size_t len,
                ssize_t *actual);

lsa_ret_t lsa_write(
                int fd,
                const void *buf,
                size_t len,
                ssize_t *actual);

lsa_ret_t lsa_pread(
                int fd,
                void *buf,
                size_t len,
                off_t offset,
                ssize_t *actual);

lsa_ret_t lsa_pwrite(
                int fd,
                const void *buf,
                size_t len,
                off_t offset,
                ssize_t *actual);

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
                struct stat *st,
                int flags);

lsa_ret_t lsa_fchmod(
                int fd,
                mode_t mode);

lsa_ret_t lsa_fchown(
                int fd,
                uid_t uid,
                gid_t gid);

/*
 * ============================================================
 * Directory Iterator API
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_open(
                int dirfd,
                lsa_dir_iter_t *iter);

lsa_ret_t lsa_dir_iter_close(
                lsa_dir_iter_t *iter);

lsa_ret_t lsa_dir_iter_next(
                lsa_dir_iter_t *iter,
                lsa_dirent_t *entries,
                uint32_t max_entries,
                uint32_t *actual);

lsa_ret_t lsa_dir_iter_next_plus(
                lsa_dir_iter_t *iter,
                lsa_dirent_plus_t *entries,
                uint32_t max_entries,
                uint32_t *actual);

lsa_ret_t lsa_dir_iter_get_cookie(
                lsa_dir_iter_t *iter,
                lsa_dir_cookie_t *cookie);

lsa_ret_t lsa_dir_iter_seek(
                lsa_dir_iter_t *iter,
                const lsa_dir_cookie_t *cookie);

/*
 * ============================================================
 * Extended Attribute Operations
 * ============================================================
 */

lsa_ret_t lsa_fsetxattr(
                int fd,
                const char *name,
                const void *value,
                size_t size,
                int flags);

lsa_ret_t lsa_fgetxattr(
                int fd,
                const char *name,
                void *value,
                size_t size,
                ssize_t *actual);

lsa_ret_t lsa_flistxattr(
                int fd,
                char *list,
                size_t size,
                ssize_t *actual);

lsa_ret_t lsa_fremovexattr(
                int fd,
                const char *name);

/*
 * ============================================================
 * Filesystem Operations
 * ============================================================
 */

lsa_ret_t lsa_statfs(
                int fd,
                struct statfs *buf);

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
 * Error Helper
 * ============================================================
 */

fs_error_t lsa_errno_map(
                int err);

#ifdef __cplusplus
}
#endif