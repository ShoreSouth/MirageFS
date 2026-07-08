#include "lsa/include/lsa_api.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"

typedef struct lsa_linux_file_handle {

    struct file_handle hdr;

    uint8_t data[LSA_HANDLE_MAX_SIZE];

} lsa_linux_file_handle_t;

#define LSA_MOUNT_SLOT_NR 32U

typedef struct lsa_mount_slot {

    bool used;
    int32_t mount_id;
    int mount_fd;

} lsa_mount_slot_t;

static lsa_mount_slot_t g_lsa_mounts[LSA_MOUNT_SLOT_NR];

static lsa_mount_slot_t *lsa_mount_find(int32_t mount_id)
{
    uint32_t i;

    for (i = 0U; i < LSA_MOUNT_SLOT_NR; i++) {
        if (g_lsa_mounts[i].used &&
            (g_lsa_mounts[i].mount_id == mount_id)) {
            return &g_lsa_mounts[i];
        }
    }

    return NULL;
}

static lsa_ret_t lsa_mount_register(int32_t mount_id, int mount_fd)
{
    lsa_mount_slot_t *slot;
    uint32_t i;

    if (mount_fd < 0) {
        return lsa_error(FS_OP_OPENHANDLE, EINVAL);
    }

    slot = lsa_mount_find(mount_id);
    if (slot != NULL) {
        (void)close(slot->mount_fd);
        slot->mount_fd = mount_fd;
        return FS_OK;
    }

    for (i = 0U; i < LSA_MOUNT_SLOT_NR; i++) {
        if (!g_lsa_mounts[i].used) {
            g_lsa_mounts[i].used = true;
            g_lsa_mounts[i].mount_id = mount_id;
            g_lsa_mounts[i].mount_fd = mount_fd;
            return FS_OK;
        }
    }

    return lsa_error(FS_OP_OPENHANDLE, ENOSPC);
}

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

lsa_ret_t lsa_open_by_handle_id(
                int32_t mount_id,
                const lsa_file_handle_t *handle,
                int flags,
                int *fd)
{
    lsa_mount_slot_t *slot;

    FS_LOG_DUMP_INFO("enter: mount_id=%d, flags=%d",
                     mount_id, flags);

    slot = lsa_mount_find(mount_id);
    if (slot == NULL) {
        FS_LOG_DUMP_ERROR("open_by_handle_id failed: mount not found, "
                          "mount_id=%d", mount_id);
        return lsa_error(FS_OP_OPENHANDLE, ENOENT);
    }

    return lsa_open_by_handle_at(slot->mount_fd, handle, flags, fd);
}

lsa_ret_t lsa_release_mount(
                int32_t mount_id)
{
    lsa_mount_slot_t *slot;

    slot = lsa_mount_find(mount_id);
    if (slot == NULL) {
        return FS_OK;
    }

    (void)close(slot->mount_fd);
    memset(slot, 0, sizeof(*slot));

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
    int mount_fd;

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
    if (fs_failed(err)) {
        return err;
    }

    mount_fd = open(path, O_PATH | O_DIRECTORY);
    if (mount_fd < 0) {
        return lsa_error(FS_OP_OPENHANDLE, errno);
    }

    err = lsa_mount_register(*mount_id, mount_fd);
    if (fs_failed(err)) {
        (void)close(mount_fd);
        return err;
    }

    return FS_OK;
}
