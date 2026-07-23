#include "lsa_test_common.h"

static int test_lsa_xattr_flag_matrix(void)
{
    char dir_path[PATH_MAX];
    char list[128];
    char value[16];
    int dirfd = -1;
    int fd = -1;
    size_t actual = 0;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "xattr.txt", &fd), 0);

    err = lsa_setxattr(fd,
                       "user.miragefs.test",
                       "one",
                       3,
                       FS_FLAG_EXCLUSIVE);
    if (fs_failed(err)) {
        TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
        TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
        TEST_ASSERT_EQ_INT(unlinkat(dirfd, "xattr.txt", 0), 0);
        TEST_ASSERT_EQ_INT(close(dirfd), 0);
        test_lsa_cleanup_tmpdir(dir_path);
        return 0;
    }

    memset(value, 0, sizeof(value));
    TEST_ASSERT_EQ_INT(lsa_getxattr(fd,
                                    "user.miragefs.test",
                                    value,
                                    sizeof(value),
                                    &actual),
                       FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);
    TEST_ASSERT_STR_EQ(value, "one");

    err = lsa_setxattr(fd,
                       "user.miragefs.test",
                       "two",
                       3,
                       FS_FLAG_EXCLUSIVE);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EEXIST), 0);
    TEST_ASSERT_EQ_INT(lsa_setxattr(fd,
                                    "user.miragefs.test",
                                    "two",
                                    3,
                                    FS_FLAG_REPLACE),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_listxattr(fd, list, sizeof(list), &actual), FS_OK);
    TEST_ASSERT_TRUE(actual > 0U);
    TEST_ASSERT_EQ_INT(lsa_removexattr(fd, "user.miragefs.test"), FS_OK);
    err = lsa_getxattr(fd,
                       "user.miragefs.test",
                       value,
                       sizeof(value),
                       &actual);
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_setxattr(fd,
                                                     "user.bad",
                                                     "x",
                                                     1,
                                                     FS_FLAG_REPLACE |
                                                     FS_FLAG_EXCLUSIVE),
                                        FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "xattr.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_XATTR_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_XATTR,
                                 0x1),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_XATTR,
                                 0x1,
                                 0x001),
                      test_lsa_xattr_flag_matrix,
                      "xattr 创建、替换、列表和删除矩阵",
                      "对 user xattr 注入 EXCLUSIVE/REPLACE/互斥 flag",
                      "支持 xattr 时完整回环，不支持时仍返回 LSA 结构化错误"),
};

const size_t LSA_XATTR_CASE_COUNT = sizeof(LSA_XATTR_CASES) / sizeof(LSA_XATTR_CASES[0]);
