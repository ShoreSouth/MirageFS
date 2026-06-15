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

    if (name == NULL ||
        actual == NULL) {

        FS_LOG_DUMP_ERROR(
                "getxattr invalid argument");

        return lsa_error(
                    FS_OP_GETXATTR,
                    EINVAL);
    }

    ret = fgetxattr(
                fd,
                name,
                value,
                size);

    if (ret < 0) {

        FS_LOG_DUMP_ERROR(
                "getxattr failed: "
                "fd=%d "
                "name=%s "
                "errno=%d(%s)",
                fd,
                name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_GETXATTR,
                    errno);
    }

    *actual = (size_t)ret;

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

    if (name == NULL) {

        FS_LOG_DUMP_ERROR(
                "setxattr invalid argument");

        return lsa_error(
                    FS_OP_SETXATTR,
                    EINVAL);
    }

    if (fs_flag_test(flags, FS_FLAG_REPLACE) &&
    fs_flag_test(flags, FS_FLAG_EXCLUSIVE)) {

        FS_LOG_DUMP_ERROR(
                "setxattr invalid flags");

        return lsa_error(
                    FS_OP_SETXATTR,
                    EINVAL);
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

        FS_LOG_DUMP_ERROR(
                "setxattr failed: "
                "fd=%d "
                "name=%s "
                "errno=%d(%s)",
                fd,
                name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_SETXATTR,
                    errno);
    }

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

    if (actual == NULL) {

        FS_LOG_DUMP_ERROR(
                "listxattr invalid argument");

        return lsa_error(
                    FS_OP_LISTXATTR,
                    EINVAL);
    }

    ret = flistxattr(
                fd,
                list,
                size);

    if (ret < 0) {

        FS_LOG_DUMP_ERROR(
                "listxattr failed: "
                "fd=%d "
                "errno=%d(%s)",
                fd,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_LISTXATTR,
                    errno);
    }

    *actual = (size_t)ret;

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
    if (name == NULL) {

        FS_LOG_DUMP_ERROR(
                "removexattr invalid argument");

        return lsa_error(
                    FS_OP_REMOVEXATTR,
                    EINVAL);
    }

    if (fremovexattr(
                fd,
                name) < 0) {

        FS_LOG_DUMP_ERROR(
                "removexattr failed: "
                "fd=%d "
                "name=%s "
                "errno=%d(%s)",
                fd,
                name,
                errno,
                strerror(errno));

        return lsa_error(
                    FS_OP_REMOVEXATTR,
                    errno);
    }

    return FS_OK;
}