#include "lsa_api.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* ============================================================
 * 打开文件
 * ============================================================ */
lsa_ret_t lsa_open(const char *path, int flags, mode_t mode, int *out_fd)
{
    if (!path || !out_fd) {
        return -EINVAL;
    }

    int fd = open(path, flags, mode);
    if (fd < 0) {
        return -errno;
    }

    *out_fd = fd;
    return 0;
}

lsa_ret_t lsa_open_at(int dirfd, const char *path, int flags,
    mode_t mode, int *out_fd)
{
    if (!path || !out_fd)
        return -EINVAL;

    int fd = openat(dirfd, path, flags, mode);
    if (fd < 0)
        return -errno;

    *out_fd = fd;
    return 0;
}

/* ============================================================
 * 打开文件（通过句柄）
 * ============================================================ */
lsa_ret_t lsa_open_by_handle_at(int mount_fd, struct file_handle *handle, 
    int flags, int *out_fd)
{
    if (!handle || !out_fd) {
        return -EINVAL;
    }

    int fd = open_by_handle_at(mount_fd, handle, flags);
    if (fd < 0) {
        return -errno;
    }

    *out_fd = fd;
    return 0;
}

/* ============================================================
 * 获取文件句柄与挂载ID
 * ============================================================ */
lsa_ret_t lsa_name_to_handle_at(int dirfd, const char *path,
    struct file_handle *handle, int *mount_id, int flags)
{
    if (!path || !handle || !mount_id) {
        return -EINVAL;
    }

    if (name_to_handle_at(dirfd, path, handle, mount_id, flags) < 0) {
        return -errno;
    }

    return 0;
}

/* ============================================================
 * 关闭文件
 * ============================================================ */
lsa_ret_t lsa_close(int fd)
{
    if (fd < 0) {
        return -EBADF;
    }

    if (close(fd) < 0) {
        return -errno;
    }

    return 0;
}

/* ============================================================
 * 顺序读
 * ============================================================ */
lsa_ret_t lsa_read(int fd, void *buf, size_t len, ssize_t *out_size)
{
    if (fd < 0 || !buf || !out_size) {
        return -EINVAL;
    }

    ssize_t ret = read(fd, buf, len);
    if (ret < 0) {
        return -errno;
    }

    *out_size = ret;
    return 0;
}

/* ============================================================
 * 顺序写
 * ============================================================ */
lsa_ret_t lsa_write(int fd, const void *buf, size_t len, ssize_t *out_size)
{
    if (fd < 0 || !buf || !out_size) {
        return -EINVAL;
    }

    ssize_t ret = write(fd, buf, len);
    if (ret < 0) {
        return -errno;
    }

    *out_size = ret;
    return 0;
}

/* ============================================================
 * 定位读
 * ============================================================ */
lsa_ret_t lsa_pread(int fd, void *buf, size_t len, off_t offset, ssize_t *out_size)
{
    if (fd < 0 || !buf || !out_size) {
        return -EINVAL;
    }

    ssize_t ret = pread(fd, buf, len, offset);
    if (ret < 0) {
        return -errno;
    }

    *out_size = ret;
    return 0;
}

/* ============================================================
 * 定位写
 * ============================================================ */
lsa_ret_t lsa_pwrite(int fd, const void *buf, size_t len, off_t offset, ssize_t *out_size)
{
    if (fd < 0 || !buf || !out_size) {
        return -EINVAL;
    }

    ssize_t ret = pwrite(fd, buf, len, offset);
    if (ret < 0) {
        return -errno;
    }

    *out_size = ret;
    return 0;
}

/* ============================================================
 * 刷盘
 * ============================================================ */
lsa_ret_t lsa_fsync(int fd)
{
    if (fd < 0) {
        return -EBADF;
    }

    if (fsync(fd) < 0) {
        return -errno;
    }

    return 0;
}