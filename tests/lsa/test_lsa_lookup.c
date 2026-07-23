#include "lsa_test_common.h"

static int test_lsa_lookup_flag_matrix(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "plain.txt", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    fd = -1;
    TEST_ASSERT_EQ_INT(lsa_mkdir(dirfd, "dir", FS_FLAG_DIRECTORY, 0755),
                       FS_OK);

    err = lsa_lookup(dirfd, "plain.txt", FS_FLAG_REGULAR, &fd);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(close(fd), 0);
    fd = -1;

    err = lsa_lookup(dirfd, "plain.txt", FS_FLAG_DIRECTORY, &fd);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_ENOTDIR), 0);
    err = lsa_lookup(dirfd, "dir", FS_FLAG_REGULAR, &fd);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EISDIR), 0);
    err = lsa_lookup(dirfd,
                     "plain.txt",
                     FS_FLAG_DIRECTORY | FS_FLAG_REGULAR,
                     &fd);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EINVAL), 0);
    err = lsa_lookup(dirfd, "plain.txt", 0x80000000U, &fd);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "plain.txt", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "dir", AT_REMOVEDIR), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_LOOKUP_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_LOOKUP,
                                 0x1),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_LOOKUP,
                                 0x1,
                                 0x001),
                      test_lsa_lookup_flag_matrix,
                      "lookup 类型约束和未知 flag 矩阵",
                      "对文件和目录分别注入 DIRECTORY/REGULAR/未知 flag",
                      "合法约束成功，类型冲突映射为 ENOTDIR/EISDIR/EINVAL"),
};

const size_t LSA_LOOKUP_CASE_COUNT = sizeof(LSA_LOOKUP_CASES) / sizeof(LSA_LOOKUP_CASES[0]);
