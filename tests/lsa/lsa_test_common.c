#include "lsa_test_common.h"

static int test_lsa_mkdir_if_missing(const char *path)
{
    if (mkdir(path, 0755) == 0)
    {
        return 0;
    }

    return errno == EEXIST ? 0 : -1;
}

int test_lsa_make_tmpdir(char *path, size_t size)
{
    const char *base;
    int written;

    base = "../output/tests/lsa";
    if (access("../output", F_OK) != 0)
    {
        base = "output/tests/lsa";
    }

    if (strcmp(base, "../output/tests/lsa") == 0)
    {
        if ((test_lsa_mkdir_if_missing("../output") != 0) ||
            (test_lsa_mkdir_if_missing("../output/tests") != 0) ||
            (test_lsa_mkdir_if_missing("../output/tests/lsa") != 0))
        {
            return -1;
        }
    }
    else
    {
        if ((test_lsa_mkdir_if_missing("output") != 0) ||
            (test_lsa_mkdir_if_missing("output/tests") != 0) ||
            (test_lsa_mkdir_if_missing("output/tests/lsa") != 0))
        {
            return -1;
        }
    }

    written = snprintf(path, size, "%s/lsa-%ld-XXXXXX", base, (long)getpid());
    if ((written < 0) || ((size_t)written >= size))
    {
        return -1;
    }

    return mkdtemp(path) == NULL ? -1 : 0;
}

void test_lsa_cleanup_tmpdir(const char *path)
{
    char cmd[PATH_MAX + 32];
    int written;

    if ((path == NULL) || (path[0] == '\0'))
    {
        return;
    }

    written = snprintf(cmd, sizeof(cmd), "rm -rf -- '%s'", path);
    if ((written > 0) && ((size_t)written < sizeof(cmd)))
    {
        int rc = system(cmd);
        (void)rc;
    }
}

int test_lsa_open_tmp_root(char *path, size_t size, int *dirfd)
{
    if (test_lsa_make_tmpdir(path, size) != 0)
    {
        return -1;
    }

    *dirfd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (*dirfd < 0)
    {
        test_lsa_cleanup_tmpdir(path);
        return -1;
    }

    return 0;
}

int test_lsa_assert_errno(fs_error_t err, uint8_t expected)
{
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), expected);
    return 0;
}

int test_lsa_assert_failed(fs_error_t err)
{
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    return 0;
}

int test_lsa_create_file(int dirfd, const char *name, int *fd)
{
    fs_error_t err;

    err = lsa_create(dirfd, name, FS_FLAG_REGULAR, 0644, fd);
    if (fs_failed(err))
    {
        return -1;
    }

    return 0;
}
