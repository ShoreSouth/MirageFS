#include "common_test_common.h"

static int test_path_join_safe_normalizes_slash(void)
{
    char path[64];
    fs_error_t err = fs_path_join_safe(path, sizeof(path), "/tmp/", "mirage");

    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ("/tmp/mirage", path);
    return 0;
}


static int test_path_join_rejects_too_small_buffer(void)
{
    char path[4];
    fs_error_t err = fs_path_join(path, sizeof(path), "/abc", "def");

    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(FS_MODULE_COMMON, fs_err_module(err));
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_ENOSPC, fs_err_errno(err));
    return 0;
}


static int test_path_normalize_removes_dotdot(void)
{
    char path[64];
    fs_error_t err = fs_path_normalize(path, sizeof(path), "/a//b/./c/../d");

    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ("/a/b/d", path);
    return 0;
}


static int test_path_normalize_collapses_leading_dotdot(void)
{
    char path[64];
    fs_error_t err = fs_path_normalize(path, sizeof(path), "../../escape");

    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ("escape", path);
    return 0;
}


static int test_path_helpers_cover_split_query_and_mkdir(void)
{
    char path[128];
    fs_error_t err;
    int rc;

    err = fs_path_dirname(path, sizeof(path), "/a/b/c");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ("/a/b", path);
    err = fs_path_dirname(path, sizeof(path), "leaf");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(".", path);
    err = fs_path_dirname(path, 2U, "/abc/def");
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_ENOSPC, fs_err_errno(err));

    TEST_ASSERT_STR_EQ("c", fs_path_basename("/a/b/c"));
    TEST_ASSERT_STR_EQ("leaf", fs_path_basename("leaf"));
    TEST_ASSERT_TRUE(fs_path_basename(NULL) == NULL);
    TEST_ASSERT_TRUE(fs_path_is_absolute("/a"));
    TEST_ASSERT_FALSE(fs_path_is_absolute("a"));
    TEST_ASSERT_TRUE(fs_path_is_empty(NULL));
    TEST_ASSERT_TRUE(fs_path_is_empty(""));
    TEST_ASSERT_FALSE(fs_path_is_empty("a"));

    rc = system("rm -rf -- '../output/tests/common/path-tree'");
    (void)rc;
    err = fs_path_mkdir_recursive("../output/tests/common/path-tree/a/b", 0775);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fs_path_exists("../output/tests/common/path-tree/a/b"));
    TEST_ASSERT_FALSE(fs_path_exists("../output/tests/common/missing"));
    err = fs_path_mkdir_recursive("", 0775);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    rc = system("rm -rf -- '../output/tests/common/path-tree'");
    (void)rc;
    return 0;
}


const test_case_t COMMON_PATH_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1,
                             0x001),
                  test_path_join_safe_normalizes_slash, "路径拼接 slash 归一",
                  "左侧路径已经以斜杠结尾", "结果只保留一个路径分隔符"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1,
                             0x002),
                  test_path_join_rejects_too_small_buffer, "路径拼接小 buffer",
                  "目标缓冲区空间不足", "返回 COMMON/PATH/ENOSPC"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1,
                             0x003),
                  test_path_normalize_removes_dotdot,
                  "路径 normalize 折叠 dotdot",
                  "输入包含重复斜杠、点和点点片段", "返回折叠后的规范路径"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1,
                             0x004),
                  test_path_normalize_collapses_leading_dotdot,
                  "路径 normalize 折叠前导 dotdot",
                  "输入以 ../.. 开头的相对路径", "当前实现折叠为剩余相对路径"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_PATH, 0x1,
                             0x005),
                  test_path_helpers_cover_split_query_and_mkdir,
                  "路径拆分、查询和递归建目录",
                  "覆盖 dirname/basename/absolute/empty/exists/mkdir_recursive "
                  "边界",
                  "路径 helper 返回预期结果，非法路径返回 COMMON/PATH 错误"),
};

const size_t COMMON_PATH_CASE_COUNT =
        sizeof(COMMON_PATH_CASES) / sizeof(COMMON_PATH_CASES[0]);
