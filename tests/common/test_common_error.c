#include "common_test_common.h"

static int test_error_layout_round_trip(void)
{
    fs_error_t err = FS_ERR(FS_SEV_ERROR, FS_MODULE_COMMON, FS_COMMON_SUB_PATH,
                            FS_ERRNO_EINVAL);

    TEST_ASSERT_EQ_INT(FS_SEV_ERROR, fs_err_severity(err));
    TEST_ASSERT_EQ_INT(FS_MODULE_COMMON, fs_err_module(err));
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_FALSE(fs_succeeded(err));
    return 0;
}


static int test_error_helpers_format_unknown_and_ok_values(void)
{
    fs_error_t err;
    const char *text1;
    const char *text2;

    TEST_ASSERT_STR_EQ("OK", fs_error_str(FS_OK));
    TEST_ASSERT_STR_EQ("INFO", fs_severity_name(FS_SEV_INFO));
    TEST_ASSERT_STR_EQ("WARN", fs_severity_name(FS_SEV_WARN));
    TEST_ASSERT_STR_EQ("ERROR", fs_severity_name(FS_SEV_ERROR));
    TEST_ASSERT_STR_EQ("FATAL", fs_severity_name(FS_SEV_FATAL));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_severity_name((fs_err_severity_t)99));

    TEST_ASSERT_TRUE(fs_errno_valid(FS_ERRNO_EINVAL));
    TEST_ASSERT_FALSE(fs_errno_valid(9999));
    TEST_ASSERT_STR_EQ("EINVAL", fs_errno_name(FS_ERRNO_EINVAL));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_errno_name((fs_errno_t)9999));
    TEST_ASSERT_STR_EQ("Unknown errno", fs_errno_desc((fs_errno_t)9999));

    err = FS_ERR(FS_SEV_WARN, FS_MODULE_MAX, 0xffU, (fs_errno_t)9999);
    text1 = fs_error_str(err);
    text2 = fs_error_str(err);
    TEST_ASSERT_TRUE(text1 != NULL);
    TEST_ASSERT_TRUE(text2 != NULL);
    TEST_ASSERT_TRUE(text1 != text2);
    return 0;
}


const test_case_t COMMON_ERROR_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_ERROR, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_ERROR, 0x1,
                             0x001),
                  test_error_layout_round_trip, "错误码布局往返",
                  "构造 fs_error_t 并逐字段解码",
                  "severity/module/sub/errno 与构造值完全一致"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_ERROR, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_ERROR, 0x1,
                             0x002),
                  test_error_helpers_format_unknown_and_ok_values,
                  "错误 helper 格式化和未知值",
                  "覆盖 OK、severity 名称、errno 名称/描述和未知模块格式化",
                  "合法值返回名称，未知值返回 UNKNOWN，错误字符串使用轮转缓冲"),
};

const size_t COMMON_ERROR_CASE_COUNT =
        sizeof(COMMON_ERROR_CASES) / sizeof(COMMON_ERROR_CASES[0]);
