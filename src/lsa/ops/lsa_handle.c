#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"
#include "lsa/include/lsa_api.h"

typedef struct lsa_linux_file_handle {

    struct file_handle hdr;

    uint8_t data[LSA_HANDLE_MAX_SIZE];

} lsa_linux_file_handle_t;

/*
 * ============================================================
 * name_to_handle_at
 * ============================================================
 */

lsa_ret_t lsa_name_to_handle_at(
                int dirfd,
                const char *path,
                lsa_file_handle_t *handle,
                int32_t *mount_id,
                int flags)
{
    lsa_linux_file_handle_t fh;
    lsa_ret_t err;
    int ret;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, path=%s, flags=%d",
                     dirfd, path ? path : "(null)", flags);

    if (path == NULL ||
        handle == NULL ||
        mount_id == NULL) {

        err = lsa_error(FS_OP_GETHANDLE, EINVAL);
        FS_LOG_DUMP_ERROR("name_to_handle_at: invalid argument, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    memset(&fh, 0, sizeof(fh));

    fh.hdr.handle_bytes = sizeof(fh.data);

    ret = name_to_handle_at(
                    dirfd,
                    path,
                    &fh.hdr,
                    mount_id,
                    flags);

    if (ret < 0) {
        err = lsa_error(FS_OP_GETHANDLE, errno);
        FS_LOG_DUMP_ERROR("name_to_handle_at failed: path=%s, "
                          "err=%s (0x%x)", path, fs_error_str(err), err);
        return err;
    }

    handle->handle_bytes = fh.hdr.handle_bytes;
    handle->handle_type  = fh.hdr.handle_type;

    memcpy(
        handle->data,
        fh.data,
        fh.hdr.handle_bytes);

    FS_LOG_DUMP_INFO("exit: ok, mount_id=%d", *mount_id);
    return FS_OK;
}

/*
 * ============================================================
 * open_by_handle_at
 * ============================================================
 */

lsa_ret_t lsa_open_by_handle_at(
                int mount_fd,
                const lsa_file_handle_t *handle,
                int flags,
                int *fd)
{
    lsa_linux_file_handle_t fh;
    lsa_ret_t err;
    int newfd;

    FS_LOG_DUMP_INFO("enter: mount_fd=%d, flags=%d", mount_fd, flags);

    if (handle == NULL ||
        fd == NULL) {

        err = lsa_error(FS_OP_OPENHANDLE, EINVAL);
        FS_LOG_DUMP_ERROR("open_by_handle_at: invalid argument, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    memset(&fh, 0, sizeof(fh));

    fh.hdr.handle_bytes =
            handle->handle_bytes;

    fh.hdr.handle_type =
            handle->handle_type;

    memcpy(
        fh.data,
        handle->data,
        handle->handle_bytes);

    newfd = open_by_handle_at(
                    mount_fd,
                    &fh.hdr,
                    flags);

    if (newfd < 0) {
        err = lsa_error(FS_OP_OPENHANDLE, errno);
        FS_LOG_DUMP_ERROR("open_by_handle_at failed: err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    *fd = newfd;

    FS_LOG_DUMP_INFO("exit: ok, fd=%d", newfd);
    return FS_OK;
}

/*
 * ============================================================
 * sysroot bootstrap
 * ============================================================
 *
 * 仅供 FSC sysroot 启动使用：创建或复用根目录，并取得其 file handle。
 */

lsa_ret_t lsa_bootstrap_root(
                const char *path,
                lsa_file_handle_t *handle,
                int32_t *mount_id)
{
    struct stat st;
    lsa_ret_t err;

    if ((path == NULL) ||
        (handle == NULL) ||
        (mount_id == NULL)) {
        return lsa_error(FS_OP_GETHANDLE, EINVAL);
    }

    if ((mkdir(path, FS_MODE_DIR_DEFAULT & FS_PERM_MASK) < 0) &&
        (errno != EEXIST)) {
        return lsa_error(FS_OP_MKDIR, errno);
    }

    if (stat(path, &st) < 0) {
        return lsa_error(FS_OP_GETATTR, errno);
    }

    if (!S_ISDIR(st.st_mode)) {
        return lsa_error(FS_OP_GETHANDLE, ENOTDIR);
    }

    err = lsa_name_to_handle_at(AT_FDCWD, path, handle, mount_id, 0);
    return err;
}
