#include "msh_test_common.h"

static int test_msh_arg_or_default_handles_missing_arg(void)
{
    char line[] = "fs enter";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(line, &args), 0);
    TEST_ASSERT_STR_EQ(msh_arg_or_default(&args, 1, "default"), "enter");
    TEST_ASSERT_STR_EQ(msh_arg_or_default(&args, 2, "default"), "default");
    TEST_ASSERT_STR_EQ(msh_arg_or_default(NULL, 0, "default"), "default");
    return 0;
}


const test_case_t MSH_ARGS_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_ARGS, 0x1),
                  UT_CASE_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_ARGS, 0x1, 0x001),
                  test_msh_arg_or_default_handles_missing_arg, "MSH 参数默认值",
                  "读取存在参数、越界参数和 NULL args",
                  "存在参数原样返回，缺失参数返回 fallback"),
};

const size_t MSH_ARGS_CASE_COUNT =
        sizeof(MSH_ARGS_CASES) / sizeof(MSH_ARGS_CASES[0]);
