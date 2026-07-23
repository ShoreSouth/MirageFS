#include "lsa_test_common.h"

static int test_lsa_create_write_read_round_trip(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int filefd = -1;
    size_t actual = 0;
    off_t offset = -1;
    char buf[16];
    fs_error_t err;

    TEST_ASSERT_EQ_INT(
            test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "hello.txt", &filefd), 0);

    err = lsa_write_full(filefd, "abc", 3, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);

    err = lsa_lseek(filefd, 0, SEEK_SET, &offset);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(offset, 0);

    memset(buf, 0, sizeof(buf));
    err = lsa_read_full(filefd, buf, 8, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);
    TEST_ASSERT_STR_EQ(buf, "abc");

    err = lsa_pwrite(filefd, "Z", 1, 1, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 1);

    memset(buf, 0, sizeof(buf));
    err = lsa_pread(filefd, buf, 3, 0, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);
    TEST_ASSERT_STR_EQ(buf, "aZc");

    err = lsa_ftruncate(filefd, 2);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = lsa_fsync(filefd);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    TEST_ASSERT_EQ_INT(lsa_close(filefd), FS_OK);
    filefd = -1;
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "hello.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_FILE_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA, TEST_LSA_COMPONENT_FILE, 0x1),
                  UT_CASE_NO(UT_MOD_LSA, TEST_LSA_COMPONENT_FILE, 0x1, 0x001),
                  test_lsa_create_write_read_round_trip,
                  "在项目 output 临时目录创建文件并完成读写回环",
                  "使用真实 LSA 后端执行 write/read/pwrite/pread/truncate",
                  "所有 IO 路径返回 FS_OK，内容和实际字节数符合预期"),
};

const size_t LSA_FILE_CASE_COUNT =
        sizeof(LSA_FILE_CASES) / sizeof(LSA_FILE_CASES[0]);
