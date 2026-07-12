#include "framework/test_framework.h"

#include "common/fs_common.h"

static int test_error_layout_round_trip(void)
{
    fs_error_t err = FS_ERR(FS_SEV_ERROR,
                            FS_MODULE_COMMON,
                            FS_COMMON_SUB_PATH,
                            FS_ERRNO_EINVAL);

    TEST_ASSERT_EQ_INT(fs_err_severity(err), FS_SEV_ERROR);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_COMMON);
    TEST_ASSERT_EQ_INT(fs_err_sub(err), FS_COMMON_SUB_PATH);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), FS_ERRNO_EINVAL);
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_FALSE(fs_succeeded(err));

    return 0;
}

static int test_path_join_safe_normalizes_slash(void)
{
    char path[64];
    fs_error_t err = fs_path_join_safe(path, sizeof(path), "/tmp/", "mirage");

    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_STR_EQ(path, "/tmp/mirage");

    return 0;
}

static int test_path_join_rejects_too_small_buffer(void)
{
    char path[4];
    fs_error_t err = fs_path_join(path, sizeof(path), "/abc", "def");

    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_COMMON);
    TEST_ASSERT_EQ_INT(fs_err_sub(err), FS_COMMON_SUB_PATH);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), FS_ERRNO_ENOSPC);

    return 0;
}

static int test_path_normalize_removes_dotdot(void)
{
    char path[64];
    fs_error_t err = fs_path_normalize(path, sizeof(path), "/a//b/./c/../d");

    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_STR_EQ(path, "/a/b/d");

    return 0;
}

int main(void)
{
    const test_case_t cases[] = {
        TEST_CASE(test_error_layout_round_trip,
                  "构造 fs_error_t 并逐字段解码",
                  "无故障注入",
                  "severity/module/sub/errno 与构造值完全一致"),
        TEST_CASE(test_path_join_safe_normalizes_slash,
                  "左侧路径已经以斜杠结尾时拼接路径",
                  "无故障注入",
                  "结果只保留一个路径分隔符"),
        TEST_CASE(test_path_join_rejects_too_small_buffer,
                  "使用故意过小的目标缓冲区拼接路径",
                  "目标缓冲区空间不足",
                  "返回 COMMON/PATH/ENOSPC"),
        TEST_CASE(test_path_normalize_removes_dotdot,
                  "规范化包含重复斜杠、点和点点的路径",
                  "输入包含 ./ 和 ../ 边界片段",
                  "返回折叠后的规范路径"),
    };

    return test_run_suite("common", cases, sizeof(cases) / sizeof(cases[0]));
}
