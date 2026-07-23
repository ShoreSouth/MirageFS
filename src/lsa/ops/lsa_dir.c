#include "lsa/include/lsa_api.h"

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

struct linux_dirent64
{
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

static int lsa_getdents64(int fd, void *buf, size_t size)
{
    return syscall(SYS_getdents64, fd, buf, size);
}

static fs_type_t lsa_type_from_dtype(unsigned char dtype)
{
    switch (dtype)
    {
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

lsa_ret_t lsa_dir_iter_open(int dirfd, uint32_t buffer_size,
                            lsa_dir_iter_t **iter_out)
{
    lsa_dir_iter_t *iter;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: dirfd=%d, buffer_size=%u", dirfd, buffer_size);

    if (iter_out == NULL)
    {
        err = lsa_error(FS_OP_READDIR, EINVAL);
        FS_LOG_DUMP_ERROR("dir_iter_open: invalid argument "
                          "(iter_out is NULL), err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (buffer_size == 0U)
    {
        buffer_size = LSA_DIR_BUFFER_SIZE_DEFAULT;
    }

    if (buffer_size < LSA_DIR_BUFFER_SIZE_MIN)
    {
        err = lsa_error(FS_OP_READDIR, EINVAL);
        FS_LOG_DUMP_ERROR("dir_iter_open: buffer_size too small (%u < %u), "
                          "err=%s (0x%x)",
                          buffer_size, LSA_DIR_BUFFER_SIZE_MIN,
                          fs_error_str(err), err);
        return err;
    }

    iter = calloc(1, sizeof(*iter));
    if (iter == NULL)
    {
        err = lsa_error(FS_OP_READDIR, ENOMEM);
        FS_LOG_DUMP_ERROR("dir_iter_open: calloc failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    iter->buffer = malloc(buffer_size);
    if (iter->buffer == NULL)
    {
        free(iter);

        err = lsa_error(FS_OP_READDIR, ENOMEM);
        FS_LOG_DUMP_ERROR("dir_iter_open: malloc failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    iter->dirfd = dirfd;
    iter->buffer_size = buffer_size;

    *iter_out = iter;

    FS_LOG_DUMP_INFO("exit: ok, iter=%p", (void *)iter);
    return FS_OK;
}

/* ============================================================
 * close
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_close(lsa_dir_iter_t *iter)
{
    FS_LOG_DUMP_INFO("enter: iter=%p", (void *)iter);

    if (iter == NULL)
    {
        FS_LOG_DUMP_INFO("exit: already closed (iter is NULL)");
        return FS_OK;
    }

    free(iter->buffer);
    free(iter);

    FS_LOG_DUMP_INFO("exit: done");
    return FS_OK;
}

/* ============================================================
 * refill
 * ============================================================
 */

static lsa_ret_t lsa_dir_refill(lsa_dir_iter_t *iter)
{
    int ret;

    ret = lsa_getdents64(iter->dirfd, iter->buffer, iter->buffer_size);

    if (ret < 0)
    {
        return lsa_error(FS_OP_READDIR, errno);
    }

    if (ret == 0)
    {
        iter->eof = true;
        return FS_OK;
    }

    iter->offset = 0;
    iter->bytes = (uint32_t)ret;

    return FS_OK;
}

/* ============================================================
 * next
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_next(lsa_dir_iter_t *iter, lsa_dirent_t *entry)
{
    struct linux_dirent64 *dent;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: iter=%p, entry=%p", (void *)iter, (void *)entry);

    if (iter == NULL || entry == NULL)
    {
        err = lsa_error(FS_OP_READDIR, EINVAL);
        FS_LOG_DUMP_ERROR("dir_iter_next: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    while (1)
    {
        if (iter->offset >= iter->bytes)
        {
            err = lsa_dir_refill(iter);
            if (fs_failed(err))
            {
                FS_LOG_DUMP_ERROR("dir_iter_next: getdents64 failed, "
                                  "err=%s (0x%x)",
                                  fs_error_str(err), err);
                return err;
            }

            if (iter->eof)
            {
                return lsa_error(FS_OP_READDIR, ENOENT);
            }
        }

        dent = (struct linux_dirent64 *)(iter->buffer + iter->offset);

        iter->offset += dent->d_reclen;

        memset(entry, 0, sizeof(*entry));

        strncpy(entry->name, dent->d_name, NAME_MAX);

        entry->ino = dent->d_ino;

        entry->type = lsa_type_from_dtype(dent->d_type);

        iter->cookie.value = (uint64_t)dent->d_off;

        FS_LOG_DUMP_INFO("exit: ok, name=%s, ino=%lu", entry->name,
                         (unsigned long)entry->ino);
        return FS_OK;
    }
}

/* ============================================================
 * seek
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_seek(lsa_dir_iter_t *iter, lsa_dir_cookie_t cookie)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: iter=%p, cookie=%lu", (void *)iter,
                     (unsigned long)cookie.value);

    if (iter == NULL)
    {
        err = lsa_error(FS_OP_READDIR, EINVAL);
        FS_LOG_DUMP_ERROR("dir_iter_seek: invalid argument (iter is NULL), "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (lseek(iter->dirfd, (off_t)cookie.value, SEEK_SET) < 0)
    {
        err = lsa_error(FS_OP_READDIR, errno);
        FS_LOG_DUMP_ERROR("dir_iter_seek: lseek failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    iter->cookie = cookie;
    iter->offset = 0;
    iter->bytes = 0;
    iter->eof = false;

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * next_plus
 * ============================================================
 */

lsa_ret_t lsa_dir_iter_next_plus(lsa_dir_iter_t *iter, lsa_dirent_plus_t *entry)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: iter=%p, entry=%p", (void *)iter, (void *)entry);

    if (iter == NULL || entry == NULL)
    {
        err = lsa_error(FS_OP_READDIRPLUS, EINVAL);
        FS_LOG_DUMP_ERROR("dir_iter_next_plus: invalid argument, "
                          "err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = lsa_dir_iter_next(iter, &entry->entry);
    if (fs_failed(err))
    {
        return err;
    }

    if (fstatat(iter->dirfd, entry->entry.name, &entry->st,
                AT_SYMLINK_NOFOLLOW) < 0)
    {
        err = lsa_error(FS_OP_READDIRPLUS, errno);
        FS_LOG_DUMP_ERROR("dir_iter_next_plus: fstatat failed, name=%s, "
                          "err=%s (0x%x)",
                          entry->entry.name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok, name=%s", entry->entry.name);
    return FS_OK;
}
