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
    if (close(fd) < 0) {

        FS_LOG_DUMP_ERROR(
                "close failed: "
                "fd=%d "
                "errno=%d(%s)",
                fd,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_CLOSE,
                    errno);
    }

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

    if (buf == NULL) {
        return lsa_error(
                    FS_OP_READ,
                    EINVAL);
    }

    ret = read(
                fd,
                buf,
                size);

    if (ret < 0) {

        FS_LOG_DUMP_ERROR(
                "read failed: "
                "fd=%d "
                "size=%zu "
                "errno=%d(%s)",
                fd,
                size,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_READ,
                    errno);
    }

    if (actual != NULL) {
        *actual = (size_t)ret;
    }

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

    if (buf == NULL) {
        return lsa_error(
                    FS_OP_WRITE,
                    EINVAL);
    }

    ret = write(
                fd,
                buf,
                size);

    if (ret < 0) {

        FS_LOG_DUMP_ERROR(
                "write failed: "
                "fd=%d "
                "size=%zu "
                "errno=%d(%s)",
                fd,
                size,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_WRITE,
                    errno);
    }

    if (actual != NULL) {
        *actual = (size_t)ret;
    }

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

    if (buf == NULL) {
        return lsa_error(
                    FS_OP_READ,
                    EINVAL);
    }

    ret = pread(
                fd,
                buf,
                size,
                offset);

    if (ret < 0) {

        FS_LOG_DUMP_ERROR(
                "pread failed: "
                "fd=%d "
                "size=%zu "
                "offset=%ld "
                "errno=%d(%s)",
                fd,
                size,
                (long)offset,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_READ,
                    errno);
    }

    if (actual != NULL) {
        *actual = (size_t)ret;
    }

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

    if (buf == NULL) {
        return lsa_error(
                    FS_OP_WRITE,
                    EINVAL);
    }

    ret = pwrite(
                fd,
                buf,
                size,
                offset);

    if (ret < 0) {

        FS_LOG_DUMP_ERROR(
                "pwrite failed: "
                "fd=%d "
                "size=%zu "
                "offset=%ld "
                "errno=%d(%s)",
                fd,
                size,
                (long)offset,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_WRITE,
                    errno);
    }

    if (actual != NULL) {
        *actual = (size_t)ret;
    }

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

    ret = lseek(
                fd,
                offset,
                whence);

    if (ret < 0) {

        FS_LOG_DUMP_ERROR(
                "lseek failed: "
                "fd=%d "
                "offset=%ld "
                "whence=%d "
                "errno=%d(%s)",
                fd,
                (long)offset,
                whence,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_NONE,
                    errno);
    }

    if (new_offset != NULL) {
        *new_offset = ret;
    }

    return FS_OK;
}

/* ============================================================
 * fsync
 * ============================================================
 */

lsa_ret_t lsa_fsync(
                int fd)
{
    if (fsync(fd) < 0) {

        FS_LOG_DUMP_ERROR(
                "fsync failed: "
                "fd=%d "
                "errno=%d(%s)",
                fd,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_WRITE,
                    errno);
    }

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
    if (ftruncate(
                fd,
                length) < 0) {

        FS_LOG_DUMP_ERROR(
                "ftruncate failed: "
                "fd=%d "
                "length=%ld "
                "errno=%d(%s)",
                fd,
                (long)length,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_TRUNCATE,
                    errno);
    }

    return FS_OK;
}