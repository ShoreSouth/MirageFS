#include "runtime_test_common.h"

static int test_runtime_sub_names_cover_valid_and_invalid_values(void)
{
    TEST_ASSERT_STR_EQ("NONE", runtime_sub_name(RUNTIME_SUB_NONE));
    TEST_ASSERT_STR_EQ("INIT", runtime_sub_name(RUNTIME_SUB_INIT));
    TEST_ASSERT_STR_EQ("SESSION", runtime_sub_name(RUNTIME_SUB_SESSION));
    TEST_ASSERT_STR_EQ("NAMESPACE", runtime_sub_name(RUNTIME_SUB_NAMESPACE));
    TEST_ASSERT_STR_EQ("CTX", runtime_sub_name(RUNTIME_SUB_CTX));
    TEST_ASSERT_STR_EQ("PATH", runtime_sub_name(RUNTIME_SUB_PATH));
    TEST_ASSERT_STR_EQ("OP", runtime_sub_name(RUNTIME_SUB_OP));
    TEST_ASSERT_STR_EQ("HANDLE", runtime_sub_name(RUNTIME_SUB_HANDLE));
    TEST_ASSERT_STR_EQ("UNKNOWN", runtime_sub_name(RUNTIME_SUB_MAX));
    TEST_ASSERT_TRUE(runtime_sub_valid(RUNTIME_SUB_HANDLE));
    TEST_ASSERT_FALSE(runtime_sub_valid(RUNTIME_SUB_MAX));
    return 0;
}


const test_case_t RUNTIME_SUB_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_SUB, 0x1),
                  UT_CASE_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_SUB, 0x1,
                             0x001),
                  test_runtime_sub_names_cover_valid_and_invalid_values,
                  "Runtime sub-error 名称表",
                  "遍历 Runtime 子错误枚举和非法边界",
                  "合法枚举返回名称，非法值返回 UNKNOWN 且 valid=false"),
};

const size_t RUNTIME_SUB_CASE_COUNT =
        sizeof(RUNTIME_SUB_CASES) / sizeof(RUNTIME_SUB_CASES[0]);
