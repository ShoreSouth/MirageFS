#include "common/path/fs_path.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

fs_error_t fs_path_join(char *dst, size_t size, const char *a, const char *b)
{
    if (!dst || !a || !b)
        return fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_EINVAL);

    if (snprintf(dst, size, "%s/%s", a, b) >= (int)size)
        return fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_ENOSPC);

    return FS_OK;
}

fs_error_t fs_path_join_safe(char *dst, size_t size, const char *a,
                             const char *b)
{
    if (!dst || !a || !b)
        return fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_EINVAL);

    if (a[0] == '\0')
        return snprintf(dst, size, "%s", b) < (int)size
                       ? FS_OK
                       : fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_ENOSPC);

    if (b[0] == '\0')
        return snprintf(dst, size, "%s", a) < (int)size
                       ? FS_OK
                       : fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_ENOSPC);

    int need_slash = (a[strlen(a) - 1] != '/');

    if (need_slash)
        return snprintf(dst, size, "%s/%s", a, b) < (int)size
                       ? FS_OK
                       : fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_ENOSPC);
    else
        return snprintf(dst, size, "%s%s", a, b) < (int)size
                       ? FS_OK
                       : fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_ENOSPC);
}

fs_error_t fs_path_normalize(char *dst, size_t size, const char *src)
{
    if (!dst || !src)
        return fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_EINVAL);

    char buf[512];
    snprintf(buf, sizeof(buf), "%s", src);

    char *stack[128];
    int top = 0;

    char *saveptr;
    char *token = strtok_r(buf, "/", &saveptr);

    while (token)
    {
        if (strcmp(token, ".") == 0)
        {
            // skip
        }
        else if (strcmp(token, "..") == 0)
        {
            if (top > 0)
                top--;
        }
        else
        {
            stack[top++] = token;
        }
        token = strtok_r(NULL, "/", &saveptr);
    }

    dst[0] = '\0';

    if (src[0] == '/')
        strncat(dst, "/", size - strlen(dst) - 1);

    for (int i = 0; i < top; i++)
    {
        strncat(dst, stack[i], size - strlen(dst) - 1);
        if (i != top - 1)
            strncat(dst, "/", size - strlen(dst) - 1);
    }

    if (dst[0] == '\0')
        snprintf(dst, size, ".");

    return FS_OK;
}

fs_error_t fs_path_dirname(char *dst, size_t size, const char *path)
{
    if (!dst || !path)
        return fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_EINVAL);

    const char *slash = strrchr(path, '/');

    if (!slash)
    {
        snprintf(dst, size, ".");
        return FS_OK;
    }

    size_t len = slash - path;

    if (len == 0)
        len = 1;

    if (len >= size)
        return fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_ENOSPC);

    memcpy(dst, path, len);
    dst[len] = '\0';

    return FS_OK;
}

const char *fs_path_basename(const char *path)
{
    if (!path)
        return NULL;

    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

bool fs_path_is_absolute(const char *path)
{
    return path && path[0] == '/';
}

bool fs_path_is_empty(const char *path)
{
    return (!path || path[0] == '\0');
}

bool fs_path_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

fs_error_t fs_path_mkdir_recursive(const char *path, int mode)
{
    if (!path || path[0] == '\0')
        return fs_common_error(FS_COMMON_SUB_PATH, FS_ERRNO_EINVAL);

    char buf[512];
    snprintf(buf, sizeof(buf), "%s", path);

    for (char *p = buf + 1; *p; ++p)
    {
        if (*p == '/')
        {
            *p = '\0';

            if (mkdir(buf, mode) != 0)
            {
                if (errno != EEXIST)
                    return fs_common_error(FS_COMMON_SUB_PATH, errno);
            }

            *p = '/';
        }
    }

    if (mkdir(buf, mode) != 0)
    {
        if (errno != EEXIST)
            return fs_common_error(FS_COMMON_SUB_PATH, errno);
    }

    return FS_OK;
}
