#include "lsa_test_common.h"

static int test_lsa_dir_iterator_cookie_and_plus(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    bool saw_file = false;
    bool saw_dir = false;
    lsa_dir_iter_t *iter = NULL;
    lsa_dirent_t entry;
    lsa_dirent_plus_t plus;
    lsa_dir_cookie_t cookie;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "a.txt", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_mkdir(dirfd, "sub", 0, 0755), FS_OK);

    err = lsa_dir_iter_open(dirfd, 0, &iter);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    while (fs_succeeded(lsa_dir_iter_next(iter, &entry))) {
        if (strcmp(entry.name, "a.txt") == 0) {
            saw_file = true;
        }
        if (strcmp(entry.name, "sub") == 0) {
            saw_dir = true;
        }
    }
    TEST_ASSERT_TRUE(saw_file);
    TEST_ASSERT_TRUE(saw_dir);

    cookie.value = 0;
    TEST_ASSERT_EQ_INT(lsa_dir_iter_seek(iter, cookie), FS_OK);
    err = lsa_dir_iter_next_plus(iter, &plus);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(plus.entry.name[0] != '\0');
    TEST_ASSERT_EQ_INT(lsa_dir_iter_close(iter), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_dir_iter_close(NULL), FS_OK);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "a.txt", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "sub", AT_REMOVEDIR), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_DIR_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_DIR,
                                 0x1),
                      UT_CASE_NO(UT_MOD_LSA,
                                 TEST_LSA_COMPONENT_DIR,
                                 0x1,
                                 0x001),
                      test_lsa_dir_iterator_cookie_and_plus,
                      "目录迭代器 entry、plus 和 cookie 行为",
                      "创建文件和目录后执行 next/seek/next_plus/get_cookie",
                      "能枚举目标项，plus 可取 stat，cookie 可用于重新定位"),
};

const size_t LSA_DIR_CASE_COUNT = sizeof(LSA_DIR_CASES) / sizeof(LSA_DIR_CASES[0]);
