#include "framework/test_framework.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/include/lsa_api.h"


typedef enum test_lsa_component {
    TEST_LSA_COMPONENT_FILE = 0x01,
    TEST_LSA_COMPONENT_LOOKUP = 0x02,
    TEST_LSA_COMPONENT_CREATE = 0x03,
} test_lsa_component_t;

static int make_tmpdir(char *path, size_t size)
{
    int written = snprintf(path,
                           size,
                           "/tmp/miragefs-lsa-test-%ld-XXXXXX",
                           (long)getpid());

    if (written < 0 || (size_t)written >= size) {
        return -1;
    }

    return mkdtemp(path) == NULL ? -1 : 0;
}

static int test_lsa_create_write_read_round_trip(void)
{
    char dir_path[128];
    int dirfd;
    int filefd;
    size_t actual;
    char buf[16];
    fs_error_t err;

    TEST_ASSERT_EQ_INT(make_tmpdir(dir_path, sizeof(dir_path)), 0);

    dirfd = open(dir_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    TEST_ASSERT_TRUE(dirfd >= 0);

    err = lsa_create(dirfd, "hello.txt", FS_FLAG_REGULAR, 0644, &filefd);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = lsa_write_full(filefd, "abc", 3, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);

    TEST_ASSERT_EQ_INT(lseek(filefd, 0, SEEK_SET), 0);

    memset(buf, 0, sizeof(buf));
    err = lsa_read_full(filefd, buf, 3, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);
    TEST_ASSERT_STR_EQ(buf, "abc");

    TEST_ASSERT_EQ_INT(close(filefd), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "hello.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    TEST_ASSERT_EQ_INT(rmdir(dir_path), 0);

    return 0;
}

static int test_lsa_lookup_missing_returns_enoent(void)
{
    char dir_path[128];
    int dirfd;
    int fd = -1;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(make_tmpdir(dir_path, sizeof(dir_path)), 0);

    dirfd = open(dir_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    TEST_ASSERT_TRUE(dirfd >= 0);

    err = lsa_lookup(dirfd, "missing", 0, &fd);
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), FS_ERRNO_ENOENT);
    TEST_ASSERT_EQ_INT(fd, -1);

    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    TEST_ASSERT_EQ_INT(rmdir(dir_path), 0);

    return 0;
}

static int test_lsa_create_rejects_conflicting_flags(void)
{
    char dir_path[128];
    int dirfd;
    int fd = -1;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(make_tmpdir(dir_path, sizeof(dir_path)), 0);

    dirfd = open(dir_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    TEST_ASSERT_TRUE(dirfd >= 0);

    err = lsa_create(dirfd,
                     "bad.txt",
                     FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE,
                     0644,
                     &fd);
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), FS_ERRNO_EINVAL);
    TEST_ASSERT_EQ_INT(fd, -1);

    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    TEST_ASSERT_EQ_INT(rmdir(dir_path), 0);

    return 0;
}

int main(void)
{
    const test_case_t cases[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                         TEST_LSA_COMPONENT_FILE,
                         0x1),
              UT_CASE_NO(UT_MOD_LSA,
                         TEST_LSA_COMPONENT_FILE,
                         0x1,
                         0x001),
              test_lsa_create_write_read_round_trip,
              "在真实临时目录中创建文件并完成写读回环",
              "使用真实 tmpdir 后端，不强制 syscall 失败",
              "create/write/read 均返回 FS_OK，且字节内容一致"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                         TEST_LSA_COMPONENT_LOOKUP,
                         0x1),
              UT_CASE_NO(UT_MOD_LSA,
                         TEST_LSA_COMPONENT_LOOKUP,
                         0x1,
                         0x001),
              test_lsa_lookup_missing_returns_enoent,
              "在已存在目录下 lookup 一个不存在的条目",
              "Linux openat 返回 ENOENT",
              "LSA 将失败映射为 FS_MODULE_LSA/ENOENT"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                         TEST_LSA_COMPONENT_CREATE,
                         0x1),
              UT_CASE_NO(UT_MOD_LSA,
                         TEST_LSA_COMPONENT_CREATE,
                         0x1,
                         0x001),
              test_lsa_create_rejects_conflicting_flags,
              "使用互斥的 REPLACE 与 EXCLUSIVE 标志创建文件",
              "调用方注入非法 flag 组合",
              "LSA 在 syscall 前拒绝，并返回 EINVAL"),
    };

    return test_run_suite("lsa", cases, sizeof(cases) / sizeof(cases[0]));
}
