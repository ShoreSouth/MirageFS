#include "lsa_test_common.h"

static int test_lsa_attr_and_fs_ops(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    struct stat st;
    struct statx stx;
    struct statfs fsst;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(
            test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "attr.txt", &fd), 0);

    TEST_ASSERT_EQ_INT(lsa_fstat(fd, &st), FS_OK);
    TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
    memset(&stx, 0, sizeof(stx));
    TEST_ASSERT_EQ_INT(lsa_fstatx(fd, &stx), FS_OK);
    TEST_ASSERT_TRUE((stx.stx_mask & STATX_TYPE) != 0U);
    TEST_ASSERT_TRUE(S_ISREG(stx.stx_mode));
    TEST_ASSERT_EQ_INT(lsa_fchmod(fd, 0600), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_fchown(fd, (uid_t)-1, (gid_t)-1), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_faccess(fd, R_OK | W_OK), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_fstatat(dirfd, "attr.txt", 0, &st), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_statfs(fd, &fsst), FS_OK);
    err = lsa_syncfs(fd);
    TEST_ASSERT_TRUE((err == FS_OK) || fs_failed(err));

    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "attr.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_ATTR_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA, TEST_LSA_COMPONENT_ATTR, 0x1),
                  UT_CASE_NO(UT_MOD_LSA, TEST_LSA_COMPONENT_ATTR, 0x1, 0x001),
                  test_lsa_attr_and_fs_ops, "属性和文件系统级操作真实后端验证",
                  "执行 fstat/fchmod/fchown/faccess/fstatat/statfs/syncfs",
                  "属性与文件系统信息可读取，允许 syncfs 返回结构化失败"),
};

const size_t LSA_ATTR_CASE_COUNT =
        sizeof(LSA_ATTR_CASES) / sizeof(LSA_ATTR_CASES[0]);
