#include "lsa_test_common.h"

static int test_lsa_create_flag_matrix(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);

    err = lsa_create(dirfd, "matrix.txt", FS_FLAG_REGULAR, 0644, &fd);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    fd = -1;

    err = lsa_create(dirfd,
                     "matrix.txt",
                     FS_FLAG_EXCLUSIVE,
                     0644,
                     &fd);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EEXIST), 0);

    err = lsa_create(dirfd,
                     "matrix.txt",
                     FS_FLAG_REPLACE | FS_FLAG_TRUNCATE,
                     0644,
                     &fd);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    fd = -1;

    err = lsa_create(dirfd, "bad.txt", 0x80000000U, 0644, &fd);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EINVAL), 0);
    err = lsa_create(dirfd,
                     "bad.txt",
                     FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE,
                     0644,
                     &fd);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "matrix.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_CREATE_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_CREATE,
                                 0x1),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_CREATE,
                                 0x1,
                                 0x001),
                      test_lsa_create_flag_matrix,
                      "create 已存在目标与 flag 组合矩阵",
                      "重复创建、REPLACE/TRUNCATE、未知 flag 和互斥 flag",
                      "EXCLUSIVE 返回 EEXIST，REPLACE 成功，非法组合返回 EINVAL"),
};

const size_t LSA_CREATE_CASE_COUNT = sizeof(LSA_CREATE_CASES) / sizeof(LSA_CREATE_CASES[0]);
