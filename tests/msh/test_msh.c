#include "framework/test_framework.h"

#include <string.h>

#include "msh/internal/msh_internal.h"

static int test_msh_parse_splits_arguments_and_quotes(void)
{
    char line[] = "create \"hello world\" /tmp/file";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(0, msh_parse_line(line, &args));
    TEST_ASSERT_EQ_INT(3, args.argc);
    TEST_ASSERT_STR_EQ("create", args.argv[0]);
    TEST_ASSERT_STR_EQ("hello world", args.argv[1]);
    TEST_ASSERT_STR_EQ("/tmp/file", args.argv[2]);
    return 0;
}

static int test_msh_parse_ignores_comment_after_spaces(void)
{
    char line[] = "   # comment";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(0, msh_parse_line(line, &args));
    TEST_ASSERT_EQ_INT(0, args.argc);
    return 0;
}

static int test_msh_arg_or_default_handles_missing_arg(void)
{
    char line[] = "fs use";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(0, msh_parse_line(line, &args));
    TEST_ASSERT_STR_EQ("use", msh_arg_or_default(&args, 1, "default"));
    TEST_ASSERT_STR_EQ("default", msh_arg_or_default(&args, 2, "default"));
    TEST_ASSERT_STR_EQ("default", msh_arg_or_default(NULL, 0, "default"));
    return 0;
}

static const test_case_t TEST_CASES[] = {
    TEST_CASE(test_msh_parse_splits_arguments_and_quotes,
              "MSH 命令解析",
              "带双引号参数的命令行",
              "argc/argv 保留 quoted 参数整体"),
    TEST_CASE(test_msh_parse_ignores_comment_after_spaces,
              "MSH 注释行解析",
              "空白后跟 # 注释",
              "解析为 0 个参数"),
    TEST_CASE(test_msh_arg_or_default_handles_missing_arg,
              "MSH 参数默认值",
              "读取存在参数、越界参数和 NULL args",
              "存在参数原样返回，缺失参数返回 fallback"),
};

int main(void)
{
    return test_run_suite("msh", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
