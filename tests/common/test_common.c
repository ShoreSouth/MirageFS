#include "framework/test_framework.h"

#include "common/fs_common.h"


typedef enum test_common_component {
    TEST_COMMON_COMPONENT_ERROR = 0x01,
    TEST_COMMON_COMPONENT_MODULE = 0x02,
    TEST_COMMON_COMPONENT_FLAG = 0x03,
    TEST_COMMON_COMPONENT_PATH = 0x04,
} test_common_component_t;

static int test_error_layout_round_trip(void)
{
    fs_error_t err = FS_ERR(FS_SEV_ERROR,
                            FS_MODULE_COMMON,
                            FS_COMMON_SUB_PATH,
                            FS_ERRNO_EINVAL);

    TEST_ASSERT_EQ_INT(FS_SEV_ERROR, fs_err_severity(err));
    TEST_ASSERT_EQ_INT(FS_MODULE_COMMON, fs_err_module(err));
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_FALSE(fs_succeeded(err));
    return 0;
}

static int test_module_and_op_helpers_handle_valid_and_invalid_values(void)
{
    TEST_ASSERT_TRUE(fs_module_valid(FS_MODULE_COMMON));
    TEST_ASSERT_FALSE(fs_module_valid(FS_MODULE_NONE));
    TEST_ASSERT_FALSE(fs_module_valid(FS_MODULE_MAX));
    TEST_ASSERT_STR_EQ("COMMON", fs_module_name(FS_MODULE_COMMON));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_module_name(FS_MODULE_MAX));

    TEST_ASSERT_TRUE(fs_op_valid(FS_OP_LOOKUP));
    TEST_ASSERT_FALSE(fs_op_valid(FS_OP_MAX));
    TEST_ASSERT_STR_EQ("LOOKUP", fs_op_name(FS_OP_LOOKUP));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_op_name(FS_OP_MAX));
    return 0;
}

static int test_flag_helper_detects_set_and_missing_bits(void)
{
    fs_flags_t flags = FS_FLAG_READ | FS_FLAG_WRITE | FS_FLAG_SYNC;

    TEST_ASSERT_TRUE(fs_flag_test(flags, FS_FLAG_READ));
    TEST_ASSERT_TRUE(fs_flag_test(flags, FS_FLAG_WRITE));
    TEST_ASSERT_FALSE(fs_flag_test(flags, FS_FLAG_DIRECTORY));
    TEST_ASSERT_FALSE(fs_flag_test(FS_FLAG_NONE, FS_FLAG_READ));
    return 0;
}

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

int main(void)
{
    const test_case_t cases[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_ERROR,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_ERROR,
                         0x1,
                         0x001),
              test_error_layout_round_trip,
              "错误码布局往返",
              "构造 fs_error_t 并逐字段解码",
              "severity/module/sub/errno 与构造值完全一致"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_MODULE,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_MODULE,
                         0x1,
                         0x001),
              test_module_and_op_helpers_handle_valid_and_invalid_values,
              "模块和操作名 helper",
              "传入合法枚举、NONE/MAX 边界值",
              "合法值返回名称，非法值返回 UNKNOWN 或 false"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_FLAG,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_FLAG,
                         0x1,
                         0x001),
              test_flag_helper_detects_set_and_missing_bits,
              "flag 位检测",
              "构造 READ/WRITE/SYNC 组合并检测缺失位",
              "已设置 flag 为 true，未设置 flag 为 false"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x001),
              test_path_join_safe_normalizes_slash,
              "路径拼接 slash 归一",
              "左侧路径已经以斜杠结尾",
              "结果只保留一个路径分隔符"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x002),
              test_path_join_rejects_too_small_buffer,
              "路径拼接小 buffer",
              "目标缓冲区空间不足",
              "返回 COMMON/PATH/ENOSPC"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x003),
              test_path_normalize_removes_dotdot,
              "路径 normalize 折叠 dotdot",
              "输入包含重复斜杠、点和点点片段",
              "返回折叠后的规范路径"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x004),
              test_path_normalize_collapses_leading_dotdot,
              "路径 normalize 折叠前导 dotdot",
              "输入以 ../.. 开头的相对路径",
              "当前实现折叠为剩余相对路径"),
    };

    return test_run_suite("common", cases, sizeof(cases) / sizeof(cases[0]));
}
