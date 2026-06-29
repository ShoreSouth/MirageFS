#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/include/lsa_api.h"
#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"

/* ============================================================
 * close
 * ============================================================
 */

lsa_ret_t lsa_close(
                int fd)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d", fd);

    if (close(fd) < 0) {
        err = lsa_error(FS_OP_CLOSE, errno);
        FS_LOG_DUMP_ERROR("close failed: fd=%d, err=%s (0x%x)",
                          fd, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * read
 * ============================================================
 */

lsa_ret_t lsa_read(
                int fd,
                void *buf,
                size_t size,
                size_t *actual)
{
    ssize_t ret;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, size=%zu", fd, size);

    if (buf == NULL) {
        err = lsa_error(FS_OP_READ, EINVAL);
        FS_LOG_DUMP_ERROR("read: invalid argument (buf is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    ret = read(
                fd,
                buf,
                size);

    if (ret < 0) {
        err = lsa_error(FS_OP_READ, errno);
        FS_LOG_DUMP_ERROR("read failed: fd=%d, size=%zu, err=%s (0x%x)",
                          fd, size, fs_error_str(err), err);
        return err;
    }

    if (actual != NULL) {
        *actual = (size_t)ret;
    }

    FS_LOG_DUMP_INFO("exit: ok, bytes=%zd", ret);
    return FS_OK;
}

/* ============================================================
 * write
 * ============================================================
 */

lsa_ret_t lsa_write(
                int fd,
                const void *buf,
                size_t size,
                size_t *actual)
{
    ssize_t ret;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, size=%zu", fd, size);

    if (buf == NULL) {
        err = lsa_error(FS_OP_WRITE, EINVAL);
        FS_LOG_DUMP_ERROR("write: invalid argument (buf is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    ret = write(
                fd,
                buf,
                size);

    if (ret < 0) {
        err = lsa_error(FS_OP_WRITE, errno);
        FS_LOG_DUMP_ERROR("write failed: fd=%d, size=%zu, err=%s (0x%x)",
                          fd, size, fs_error_str(err), err);
        return err;
    }

    if (actual != NULL) {
        *actual = (size_t)ret;
    }

    FS_LOG_DUMP_INFO("exit: ok, bytes=%zd", ret);
    return FS_OK;
}

/* ============================================================
 * pread
 * ============================================================
 */

lsa_ret_t lsa_pread(
                int fd,
                void *buf,
                size_t size,
                off_t offset,
                size_t *actual)
{
    ssize_t ret;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, size=%zu, offset=%ld",
                     fd, size, (long)offset);

    if (buf == NULL) {
        err = lsa_error(FS_OP_READ, EINVAL);
        FS_LOG_DUMP_ERROR("pread: invalid argument (buf is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    ret = pread(
                fd,
                buf,
                size,
                offset);

    if (ret < 0) {
        err = lsa_error(FS_OP_READ, errno);
        FS_LOG_DUMP_ERROR("pread failed: fd=%d, size=%zu, offset=%ld, "
                          "err=%s (0x%x)",
                          fd, size, (long)offset, fs_error_str(err), err);
        return err;
    }

    if (actual != NULL) {
        *actual = (size_t)ret;
    }

    FS_LOG_DUMP_INFO("exit: ok, bytes=%zd", ret);
    return FS_OK;
}

/* ============================================================
 * pwrite
 * ============================================================
 */

lsa_ret_t lsa_pwrite(
                int fd,
                const void *buf,
                size_t size,
                off_t offset,
                size_t *actual)
{
    ssize_t ret;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, size=%zu, offset=%ld",
                     fd, size, (long)offset);

    if (buf == NULL) {
        err = lsa_error(FS_OP_WRITE, EINVAL);
        FS_LOG_DUMP_ERROR("pwrite: invalid argument (buf is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    ret = pwrite(
                fd,
                buf,
                size,
                offset);

    if (ret < 0) {
        err = lsa_error(FS_OP_WRITE, errno);
        FS_LOG_DUMP_ERROR("pwrite failed: fd=%d, size=%zu, offset=%ld, "
                          "err=%s (0x%x)",
                          fd, size, (long)offset, fs_error_str(err), err);
        return err;
    }

    if (actual != NULL) {
        *actual = (size_t)ret;
    }

    FS_LOG_DUMP_INFO("exit: ok, bytes=%zd", ret);
    return FS_OK;
}

/* ============================================================
 * seek
 * ============================================================
 */

lsa_ret_t lsa_lseek(
                int fd,
                off_t offset,
                int whence,
                off_t *new_offset)
{
    off_t ret;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, offset=%ld, whence=%d",
                     fd, (long)offset, whence);

    ret = lseek(
                fd,
                offset,
                whence);

    if (ret < 0) {
        err = lsa_error(FS_OP_NONE, errno);
        FS_LOG_DUMP_ERROR("lseek failed: fd=%d, offset=%ld, whence=%d, "
                          "err=%s (0x%x)",
                          fd, (long)offset, whence, fs_error_str(err), err);
        return err;
    }

    if (new_offset != NULL) {
        *new_offset = ret;
    }

    FS_LOG_DUMP_INFO("exit: ok, new_offset=%ld", (long)ret);
    return FS_OK;
}

/* ============================================================
 * fsync
 * ============================================================
 */

lsa_ret_t lsa_fsync(
                int fd)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d", fd);

    if (fsync(fd) < 0) {
        err = lsa_error(FS_OP_WRITE, errno);
        FS_LOG_DUMP_ERROR("fsync failed: fd=%d, err=%s (0x%x)",
                          fd, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * truncate
 * ============================================================
 */

lsa_ret_t lsa_ftruncate(
                int fd,
                off_t length)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, length=%ld", fd, (long)length);

    if (ftruncate(
                fd,
                length) < 0) {
        err = lsa_error(FS_OP_TRUNCATE, errno);
        FS_LOG_DUMP_ERROR("ftruncate failed: fd=%d, length=%ld, "
                          "err=%s (0x%x)",
                          fd, (long)length, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}
