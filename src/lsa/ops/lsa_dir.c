#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/internal/lsa_dir.h"
#include "lsa/internal/lsa_common.h"
#include "lsa/internal/lsa_internal.h"
#include "lsa/internal/lsa_error.h"

/* ============================================================
 * linux_dirent64
 * ============================================================
 */

struct linux_dirent64 {

    uint64_t d_ino;

    int64_t d_off;

    uint16_t d_reclen;

    uint8_t d_type;

    char d_name[];

} linux_dirent64_t;

/* ============================================================
 * helper
 * ============================================================
 */

static int lsa_getdents64(
                int fd,
                void *buf,
                size_t size)
{
    return syscall(
                SYS_getdents64,
                fd,
                buf,
                size);
}

static fs_type_t lsa_type_from_dtype(
                unsigned char dtype)
{
    switch (dtype) {

    case DT_REG:
        return FS_TYPE_REG;

    case DT_DIR:
        return FS_TYPE_DIR;

    case DT_LNK:
        return FS_TYPE_LNK;

    case DT_FIFO:
        return FS_TYPE_FIFO;

    case DT_BLK:
        return FS_TYPE_BLK;

    case DT_CHR:
        return FS_TYPE_CHR;

    case DT_SOCK:
        return FS_TYPE_SOCK;

    default:
        return FS_TYPE_UNKNOWN;
    }
}

/* ============================================================
 * open
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_open(
                int dirfd,
                uint32_t buffer_size,
                lsa_dir_iter_t **iter_out)
{
    lsa_dir_iter_t *iter;

    if (iter_out == NULL) {

        return lsa_error(
                    FS_OP_READDIR,
                    EINVAL);
    }

    if (buffer_size == 0U) {

        buffer_size =
                LSA_DIR_BUFFER_SIZE_DEFAULT;
    }

    if (buffer_size <
        LSA_DIR_BUFFER_SIZE_MIN) {

        return lsa_error(
                    FS_OP_READDIR,
                    EINVAL);
    }

    iter = calloc(
                1,
                sizeof(*iter));

    if (iter == NULL) {

        FS_LOG_DUMP_ERROR(
                "calloc failed");

        return lsa_error(
                    FS_OP_READDIR,
                    ENOMEM);
    }

    iter->buffer =
        malloc(buffer_size);

    if (iter->buffer == NULL) {

        free(iter);

        FS_LOG_DUMP_ERROR(
                "malloc failed");

        return lsa_error(
                    FS_OP_READDIR,
                    ENOMEM);
    }

    iter->dirfd = dirfd;

    iter->buffer_size = buffer_size;

    *iter_out = iter;

    return FS_OK;
}

/* ============================================================
 * close
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_close(
                lsa_dir_iter_t *iter)
{
    if (iter == NULL) {
        return FS_OK;
    }

    free(iter->buffer);

    free(iter);

    return FS_OK;
}

/* ============================================================
 * refill
 * ============================================================
 */

static int lsa_dir_refill(
                lsa_dir_iter_t *iter)
{
    int ret;

    ret = lsa_getdents64(
                iter->dirfd,
                iter->buffer,
                iter->buffer_size);

    if (ret < 0) {
        return -errno;
    }

    if (ret == 0) {

        iter->eof = true;

        return 0;
    }

    iter->offset = 0;

    iter->bytes = (uint32_t)ret;

    return ret;
}

/* ============================================================
 * next
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_next(
                lsa_dir_iter_t *iter,
                lsa_dirent_t *entry)
{
    struct linux_dirent64 *dent;
    int ret;

    if (iter == NULL ||
        entry == NULL) {

        return lsa_error(
                    FS_OP_READDIR,
                    EINVAL);
    }

    while (1) {

        if (iter->offset >=
            iter->bytes) {

            ret = lsa_dir_refill(iter);

            if (ret < 0) {

                FS_LOG_DUMP_ERROR(
                        "getdents64 failed");

                return lsa_error(
                            FS_OP_READDIR,
                            -ret);
            }

            if (ret == 0) {

                return lsa_error(
                            FS_OP_READDIR,
                            ENOENT);
            }
        }

        dent =
            (struct linux_dirent64 *)
            (iter->buffer +
             iter->offset);

        iter->offset +=
            dent->d_reclen;

        memset(
            entry,
            0,
            sizeof(*entry));

        strncpy(
            entry->name,
            dent->d_name,
            NAME_MAX);

        entry->ino =
            dent->d_ino;

        entry->type =
            lsa_type_from_dtype(
                    dent->d_type);

        iter->cookie.value =
            (uint64_t)dent->d_off;

        return FS_OK;
    }
}

/* ============================================================
 * seek
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_seek(
                lsa_dir_iter_t *iter,
                lsa_dir_cookie_t cookie)
{
    if (iter == NULL) {

        return lsa_error(
                    FS_OP_READDIR,
                    EINVAL);
    }

    if (lseek(
            iter->dirfd,
            (off_t)cookie.value,
            SEEK_SET) < 0) {

        FS_LOG_DUMP_ERROR(
                "seek cookie failed");

        return lsa_error(
                    FS_OP_READDIR,
                    errno);
    }

    iter->cookie = cookie;

    iter->offset = 0;

    iter->bytes = 0;

    iter->eof = false;

    return FS_OK;
}

/* ============================================================
 * next_plus
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_next_plus(
                lsa_dir_iter_t *iter,
                lsa_dirent_plus_t *entry)
{
    lsa_ret_t err;

    if (iter == NULL ||
        entry == NULL) {

        return lsa_error(
                    FS_OP_READDIRPLUS,
                    EINVAL);
    }

    err = lsa_dir_iter_next(
                iter,
                &entry->entry);

    if (err != FS_OK) {
        return err;
    }

    if (fstatat(
            iter->dirfd,
            entry->entry.name,
            &entry->st,
            AT_SYMLINK_NOFOLLOW) < 0) {

        FS_LOG_DUMP_ERROR(
                "fstatat failed: "
                "name=%s errno=%d(%s)",
                entry->entry.name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_READDIRPLUS,
                    errno);
    }

    return FS_OK;
}