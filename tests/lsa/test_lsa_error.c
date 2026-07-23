#include "lsa_test_common.h"

static int test_lsa_invalid_argument_matrix(void)
{
    char byte = 0;
    size_t actual = 99;
    struct stat st;
    struct statfs fsst;
    lsa_dir_iter_t *iter = NULL;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_read(-1, NULL, 1, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_read_full(-1, NULL, 1, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_write(-1, NULL, 1, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_write_full(-1, NULL, 1, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_pread(-1, NULL, 1, 0, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_pwrite(-1, NULL, 1, 0, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_fstat(-1, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_fstatat(AT_FDCWD,
                                                    NULL,
                                                    0,
                                                    &st),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_statfs(-1, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_readlink(AT_FDCWD,
                                                     NULL,
                                                     &byte,
                                                     1,
                                                     &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_listxattr(-1, NULL, 0, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_getxattr(-1,
                                                     NULL,
                                                     &byte,
                                                     1,
                                                     &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_setxattr(-1, NULL, &byte, 1, 0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_removexattr(-1, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_dir_iter_open(-1, 1, &iter),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_dir_iter_next(NULL, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_dir_iter_next_plus(NULL, NULL),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_statfs(-1, &fsst);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EBADF), 0);
    return 0;
}


static int test_lsa_syscall_failure_matrix(void)
{
    char dir_path[PATH_MAX];
    char byte = 'x';
    int dirfd = -1;
    size_t actual = 0;
    off_t offset = 0;
    struct stat st;
    struct statfs fsst;
    lsa_dirent_t entry;
    lsa_dir_iter_t *iter = NULL;

    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_close(-1),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_read(-1, &byte, 1, &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_read_full(-1,
                                                      &byte,
                                                      1,
                                                      &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_write(-1, &byte, 1, &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_write_full(-1,
                                                       &byte,
                                                       1,
                                                       &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_pread(-1,
                                                  &byte,
                                                  1,
                                                  0,
                                                  &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_pwrite(-1,
                                                   &byte,
                                                   1,
                                                   0,
                                                   &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_failed(lsa_lseek(-1, 0, SEEK_SET, &offset)),
                       0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_fsync(-1), FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_ftruncate(-1, 1),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_fstat(-1, &st),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_fchmod(-1, 0600),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_fchown(-1,
                                                  (uid_t)-1,
                                                  (gid_t)-1),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_failed(lsa_faccess(-1, R_OK)), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_statfs(-1, &fsst),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_syncfs(-1), FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_failed(lsa_listxattr(-1,
                                                       &byte,
                                                       1,
                                                       &actual)),
                       0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_failed(lsa_removexattr(-1, "user.none")),
                       0);

    TEST_ASSERT_EQ_INT(test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(lsa_dir_iter_open(dirfd, 0, &iter), FS_OK);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    dirfd = -1;
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_dir_iter_next(iter, &entry),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_failed(lsa_dir_iter_seek(iter,
                                                           (lsa_dir_cookie_t){
                                                               0,
                                                           })),
                       0);
    TEST_ASSERT_EQ_INT(lsa_dir_iter_close(iter), FS_OK);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_ERROR_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_ERROR,
                                 0x1),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_ERROR,
                                 0x1,
                                 0x001),
                      test_lsa_invalid_argument_matrix,
                      "LSA 公共 API 空参数和非法 fd 防御矩阵",
                      "向 read/write/stat/xattr/readdir 等接口注入非法输入",
                      "在 syscall 前或 syscall 失败后返回 LSA 模块结构化错误"),
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_ERROR,
                                 0x2),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_ERROR,
                                 0x2,
                                 0x001),
                      test_lsa_syscall_failure_matrix,
                      "LSA syscall 失败路径错误映射矩阵",
                      "使用非法 fd 和失效目录迭代器触发 EBADF/失败分支",
                      "所有失败都映射为 LSA 模块结构化错误"),
};

const size_t LSA_ERROR_CASE_COUNT = sizeof(LSA_ERROR_CASES) / sizeof(LSA_ERROR_CASES[0]);
