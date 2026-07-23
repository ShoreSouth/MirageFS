#include "lsa_test_common.h"

static int test_lsa_namespace_round_trip(void)
{
    char dir_path[PATH_MAX];
    char link_buf[64];
    int dirfd = -1;
    int fd = -1;
    size_t actual = 0;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(lsa_mkdir(dirfd, "sub", FS_FLAG_DIRECTORY, 0755),
                       FS_OK);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "source.txt", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    fd = -1;

    /* 按真实 namespace 操作顺序串起来，验证 inode 关系和路径名变化。 */
    TEST_ASSERT_EQ_INT(lsa_link(dirfd, "source.txt", dirfd, "hard.txt", 0),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_rename(dirfd, "hard.txt", dirfd, "moved.txt", 0),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_symlink("source.txt", dirfd, "sym", 0), FS_OK);

    memset(link_buf, 0, sizeof(link_buf));
    err = lsa_readlink(dirfd, "sym", link_buf, sizeof(link_buf), &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_STR_EQ(link_buf, "source.txt");
    TEST_ASSERT_EQ_INT(actual, 10);

    /* FIFO 覆盖 mknod 的无设备参数路径，随后按普通 namespace 清理。 */
    TEST_ASSERT_EQ_INT(lsa_mknod(dirfd, "pipe", FS_TYPE_FIFO, 0644, NULL),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "pipe", 0),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "sym", FS_FLAG_NOFOLLOW), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "moved.txt", FS_FLAG_REGULAR),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "source.txt", FS_FLAG_REGULAR),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_rmdir(dirfd, "sub", FS_FLAG_DIRECTORY), FS_OK);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


static int test_lsa_namespace_error_matrix(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    lsa_device_t device = { 1, 7 };
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(lsa_mkdir(dirfd, "adir", 0, 0755), FS_OK);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "afile", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);

    /* 这里集中验证 syscall 前的参数校验和 syscall 后的类型冲突映射。 */
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_mkdir(dirfd, NULL, 0, 0755),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_mkdir(dirfd, "bad", 0x80000000U,
                                                  0755),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_unlink(dirfd, NULL, 0),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_unlink(dirfd, "adir", FS_FLAG_REGULAR);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EISDIR), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_rmdir(dirfd, "afile", 0),
                                        FS_ERRNO_ENOTDIR), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_rename(dirfd,
                                                   NULL,
                                                   dirfd,
                                                   "x",
                                                   0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_link(dirfd,
                                                 NULL,
                                                 dirfd,
                                                 "x",
                                                 0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_symlink(NULL, dirfd, "s", 0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_mknod(dirfd,
                                                  NULL,
                                                  FS_TYPE_FIFO,
                                                  0644,
                                                  NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_mknod(dirfd,
                                                  "blk",
                                                  FS_TYPE_BLK,
                                                  0644,
                                                  NULL),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_mknod(dirfd, "unknown", FS_TYPE_REG, 0644, &device);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "afile", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "adir", AT_REMOVEDIR), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


static int test_lsa_namespace_failure_edges(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    lsa_device_t device = { 1, 7 };
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "src", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);

    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_lookup(dirfd, NULL, 0, &fd),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_lookup(dirfd, "src", 0, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_create(dirfd,
                                                   NULL,
                                                   0,
                                                   0644,
                                                   &fd),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_create(dirfd,
                                                   "bad",
                                                   0,
                                                   0644,
                                                   NULL),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_create(dirfd,
                     "append.txt",
                     FS_FLAG_APPEND | FS_FLAG_SYNC | FS_FLAG_NOFOLLOW,
                     0644,
                     &fd);
    /* 合法补充 flag 应透传到底层 open，不应被误判为非法组合。 */
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);

    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_rmdir(dirfd, NULL, 0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_rmdir(dirfd,
                                                  "missing",
                                                  0x80000000U),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_failed(lsa_rename(dirfd,
                                                    "missing",
                                                    dirfd,
                                                    "dst",
                                                    0)),
                       0);
    TEST_ASSERT_EQ_INT(lsa_symlink("src", dirfd, "sym", 0), FS_OK);
    TEST_ASSERT_EQ_INT(test_lsa_assert_failed(lsa_symlink("src",
                                                     dirfd,
                                                     "sym",
                                                     0)),
                       0);
    TEST_ASSERT_EQ_INT(test_lsa_assert_failed(lsa_readlink(dirfd,
                                                      "missing-link",
                                                      dir_path,
                                                      sizeof(dir_path),
                                                      &(size_t){ 0 })),
                       0);
    err = lsa_mknod(dirfd, "blk", FS_TYPE_BLK, 0644, &device);
    TEST_ASSERT_TRUE((err == FS_OK) || fs_failed(err));
    if (err == FS_OK) {
        TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "blk", 0), FS_OK);
    }
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(lsa_mknod(dirfd,
                                                  "chr",
                                                  FS_TYPE_CHR,
                                                  0644,
                                                  NULL),
                                        FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "sym", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "append.txt", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "src", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_NAMESPACE_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_NAMESPACE,
                                 0x1),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_NAMESPACE,
                                 0x1,
                                 0x001),
                      test_lsa_namespace_round_trip,
                      "namespace 操作真实后端回环",
                      "执行 mkdir/link/rename/symlink/readlink/mknod/unlink/rmdir",
                      "命名空间变更可观察且最终临时目录可完整清理"),
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_NAMESPACE,
                                 0x2),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_NAMESPACE,
                                 0x2,
                                 0x001),
                      test_lsa_namespace_error_matrix,
                      "namespace 操作非法参数和类型冲突矩阵",
                      "注入 NULL 名称、未知 flag、目录当文件删除和非法 mknod",
                      "LSA 返回 EINVAL/EISDIR/ENOTDIR 等结构化错误"),
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_NAMESPACE,
                                 0x3),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_NAMESPACE,
                                 0x3,
                                 0x001),
                      test_lsa_namespace_failure_edges,
                      "namespace syscall 失败和补充 flag 路径",
                      "注入 NULL lookup/create、缺失 rename、重复 symlink 和设备节点",
                      "非法输入早退，真实 syscall 失败返回 LSA 结构化错误"),
};

const size_t LSA_NAMESPACE_CASE_COUNT = sizeof(LSA_NAMESPACE_CASES) / sizeof(LSA_NAMESPACE_CASES[0]);
