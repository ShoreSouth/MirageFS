#include <errno.h>
#include <string.h>
#include <sys/xattr.h>

#include "lsa/internal/lsa_internal.h"
#include "common/fs_common.h"
#include "lsa/internal/lsa_error.h"

/* ============================================================
 * getxattr
 * ============================================================
 */

lsa_ret_t lsa_getxattr(
                int fd,
                const char *name,
                void *value,
                size_t size,
                size_t *actual)
{
    ssize_t ret;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, name=%s, size=%zu",
                     fd, name ? name : "(null)", size);

    if (name == NULL ||
        actual == NULL) {

        err = lsa_error(FS_OP_GETXATTR, EINVAL);
        FS_LOG_DUMP_ERROR("getxattr: invalid argument, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    ret = fgetxattr(
                fd,
                name,
                value,
                size);

    if (ret < 0) {
        err = lsa_error(FS_OP_GETXATTR, errno);
        FS_LOG_DUMP_ERROR("getxattr failed: fd=%d, name=%s, err=%s (0x%x)",
                          fd, name, fs_error_str(err), err);
        return err;
    }

    *actual = (size_t)ret;

    FS_LOG_DUMP_INFO("exit: ok, actual=%zu", (size_t)ret);
    return FS_OK;
}

/* ============================================================
 * setxattr
 * ============================================================
 */

lsa_ret_t lsa_setxattr(
                int fd,
                const char *name,
                const void *value,
                size_t size,
                fs_flags_t flags)
{
    int xattr_flags = 0;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, name=%s, size=%zu, flags=0x%x",
                     fd, name ? name : "(null)", size, flags);

    if (name == NULL) {
        err = lsa_error(FS_OP_SETXATTR, EINVAL);
        FS_LOG_DUMP_ERROR("setxattr: invalid argument (name is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (fs_flag_test(flags, FS_FLAG_REPLACE) &&
        fs_flag_test(flags, FS_FLAG_EXCLUSIVE)) {

        err = lsa_error(FS_OP_SETXATTR, EINVAL);
        FS_LOG_DUMP_ERROR("setxattr: invalid flags (REPLACE and EXCLUSIVE "
                          "are mutually exclusive), err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    if (fs_flag_test(
            flags,
            FS_FLAG_REPLACE)) {

        xattr_flags |= XATTR_REPLACE;
    }

    if (fs_flag_test(
                flags,
                FS_FLAG_EXCLUSIVE)) {

        xattr_flags |= XATTR_CREATE;
    }

    if (fsetxattr(
                fd,
                name,
                value,
                size,
                xattr_flags) < 0) {
        err = lsa_error(FS_OP_SETXATTR, errno);
        FS_LOG_DUMP_ERROR("setxattr failed: fd=%d, name=%s, err=%s (0x%x)",
                          fd, name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

/* ============================================================
 * listxattr
 * ============================================================
 */

lsa_ret_t lsa_listxattr(
                int fd,
                char *list,
                size_t size,
                size_t *actual)
{
    ssize_t ret;
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, size=%zu", fd, size);

    if (actual == NULL) {
        err = lsa_error(FS_OP_LISTXATTR, EINVAL);
        FS_LOG_DUMP_ERROR("listxattr: invalid argument (actual is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    ret = flistxattr(
                fd,
                list,
                size);

    if (ret < 0) {
        err = lsa_error(FS_OP_LISTXATTR, errno);
        FS_LOG_DUMP_ERROR("listxattr failed: fd=%d, err=%s (0x%x)",
                          fd, fs_error_str(err), err);
        return err;
    }

    *actual = (size_t)ret;

    FS_LOG_DUMP_INFO("exit: ok, actual=%zu", (size_t)ret);
    return FS_OK;
}

/* ============================================================
 * removexattr
 * ============================================================
 */

lsa_ret_t lsa_removexattr(
                int fd,
                const char *name)
{
    lsa_ret_t err;

    FS_LOG_DUMP_INFO("enter: fd=%d, name=%s",
                     fd, name ? name : "(null)");

    if (name == NULL) {
        err = lsa_error(FS_OP_REMOVEXATTR, EINVAL);
        FS_LOG_DUMP_ERROR("removexattr: invalid argument (name is NULL), "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    if (fremovexattr(
                fd,
                name) < 0) {
        err = lsa_error(FS_OP_REMOVEXATTR, errno);
        FS_LOG_DUMP_ERROR("removexattr failed: fd=%d, name=%s, "
                          "err=%s (0x%x)", fd, name, fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}
