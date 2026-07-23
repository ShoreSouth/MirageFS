#include "common_test_common.h"

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


const test_case_t COMMON_MODULE_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_MODULE, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_MODULE, 0x1,
                             0x001),
                  test_module_and_op_helpers_handle_valid_and_invalid_values,
                  "模块和操作名 helper", "传入合法枚举、NONE/MAX 边界值",
                  "合法值返回名称，非法值返回 UNKNOWN 或 false"),
};

const size_t COMMON_MODULE_CASE_COUNT =
        sizeof(COMMON_MODULE_CASES) / sizeof(COMMON_MODULE_CASES[0]);
