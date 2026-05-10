#include "lsa_api.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

/* ============================================================
 * create（REG 文件）
 * ============================================================ */
lsa_ret_t lsa_create(const char *path, mode_t mode)
{
    if (!path)
        return -EINVAL;

    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
    if (fd < 0)
        return -errno;

    if (close(fd) < 0)
        return -errno;

    return 0;
}

lsa_ret_t lsa_create_at(int dirfd, const char *path, mode_t mode)
{
    if (!path)
        return -EINVAL;

    int fd = openat(dirfd, path, O_CREAT | O_WRONLY | O_TRUNC, mode);
    if (fd < 0)
        return -errno;

    if (close(fd) < 0)
        return -errno;

    return 0;
}

/* ============================================================
 * mkdir
 * ============================================================ */
lsa_ret_t lsa_mkdir(const char *path, mode_t mode)
{
    if (!path)
        return -EINVAL;

    if (mkdir(path, mode) < 0)
        return -errno;

    return 0;
}

lsa_ret_t lsa_mkdir_at(int dirfd, const char *path, mode_t mode)
{
    if (!path)
        return -EINVAL;

    if (mkdirat(dirfd, path, mode) < 0)
        return -errno;

    return 0;
}

/* ============================================================
 * mknod（通用）
 * ============================================================ */
lsa_ret_t lsa_mknod(const char *path, mode_t mode, dev_t dev)
{
    if (!path)
        return -EINVAL;

    if (mknod(path, mode, dev) < 0)
        return -errno;

    return 0;
}

lsa_ret_t lsa_mknod_at(int dirfd,
                       const char *path,
                       mode_t mode,
                       dev_t dev)
{
    if (!path)
        return -EINVAL;

    if (mknodat(dirfd, path, mode, dev) < 0)
        return -errno;

    return 0;
}

/* ============================================================
 * 语义封装
 * ============================================================ */

lsa_ret_t lsa_mkfifo(const char *path, mode_t mode)
{
    return lsa_mknod(path, S_IFIFO | mode, 0);
}

lsa_ret_t lsa_mkfifo_at(int dirfd, const char *path, mode_t mode)
{
    return lsa_mknod_at(dirfd, path, S_IFIFO | mode, 0);
}

lsa_ret_t lsa_mkchr(const char *path, mode_t mode, dev_t dev)
{
    return lsa_mknod(path, S_IFCHR | mode, dev);
}

lsa_ret_t lsa_mkchr_at(int dirfd,
                       const char *path,
                       mode_t mode,
                       dev_t dev)
{
    return lsa_mknod_at(dirfd, path, S_IFCHR | mode, dev);
}

lsa_ret_t lsa_mkblk(const char *path, mode_t mode, dev_t dev)
{
    return lsa_mknod(path, S_IFBLK | mode, dev);
}

lsa_ret_t lsa_mkblk_at(int dirfd,
                       const char *path,
                       mode_t mode,
                       dev_t dev)
{
    return lsa_mknod_at(dirfd, path, S_IFBLK | mode, dev);
}

/* socket 文件（注意：一般不用 mknod 创建） */
lsa_ret_t lsa_mksocket(const char *path, mode_t mode)
{
    return lsa_mknod(path, S_IFSOCK | mode, 0);
}

lsa_ret_t lsa_mksocket_at(int dirfd,
                          const char *path,
                          mode_t mode)
{
    return lsa_mknod_at(dirfd, path, S_IFSOCK | mode, 0);
}