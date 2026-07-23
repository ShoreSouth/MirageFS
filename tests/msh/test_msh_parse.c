#include "msh_test_common.h"

static int test_msh_parse_splits_arguments_and_quotes(void)
{
    char line[] = "create \"hello world\" /tmp/file";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(line, &args), 0);
    TEST_ASSERT_EQ_INT(args.argc, 3);
    TEST_ASSERT_STR_EQ(args.argv[0], "create");
    TEST_ASSERT_STR_EQ(args.argv[1], "hello world");
    TEST_ASSERT_STR_EQ(args.argv[2], "/tmp/file");
    return 0;
}


static int test_msh_parse_ignores_comment_after_spaces(void)
{
    char line[] = "   # comment";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(line, &args), 0);
    TEST_ASSERT_EQ_INT(args.argc, 0);
    return 0;
}


static int test_msh_parse_handles_utf8_bom_prefix(void)
{
    char line[] = "\xef\xbb\xbf"
                  "fs list";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(line, &args), 0);
    TEST_ASSERT_EQ_INT(args.argc, 2);
    TEST_ASSERT_STR_EQ(args.argv[0], "fs");
    TEST_ASSERT_STR_EQ(args.argv[1], "list");
    return 0;
}


static int test_msh_parse_supports_single_quotes(void)
{
    char line[] = "echo 'hello single quote' tail";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(line, &args), 0);
    TEST_ASSERT_EQ_INT(args.argc, 3);
    TEST_ASSERT_STR_EQ(args.argv[0], "echo");
    TEST_ASSERT_STR_EQ(args.argv[1], "hello single quote");
    TEST_ASSERT_STR_EQ(args.argv[2], "tail");
    return 0;
}


static int test_msh_parse_rejects_unclosed_quote(void)
{
    char line[] = "echo \"unterminated";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(line, &args), 1);
    return 0;
}


static int test_msh_parse_rejects_too_many_arguments(void)
{
    char line[] = "a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 a10 a11 a12 a13 a14 a15 "
                  "a16 a17 a18 a19 a20 a21 a22 a23 a24 a25 a26 a27 a28 a29 "
                  "a30 a31 a32";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(line, &args), 1);
    return 0;
}


static int test_msh_parse_rejects_null_inputs(void)
{
    char line[] = "fs list";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(NULL, &args), 1);
    TEST_ASSERT_EQ_INT(msh_parse_line(line, NULL), 1);
    return 0;
}


const test_case_t MSH_PARSE_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1),
                  UT_CASE_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1, 0x001),
                  test_msh_parse_splits_arguments_and_quotes, "MSH 命令解析",
                  "带双引号参数的命令行", "argc/argv 保留 quoted 参数整体"),
        TEST_CASE(UT_LIST_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1),
                  UT_CASE_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1, 0x002),
                  test_msh_parse_ignores_comment_after_spaces, "MSH 注释行解析",
                  "空白后跟 # 注释", "解析为 0 个参数"),
        TEST_CASE(UT_LIST_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1),
                  UT_CASE_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1, 0x003),
                  test_msh_parse_handles_utf8_bom_prefix, "MSH BOM 前缀解析",
                  "命令行开头带 UTF-8 BOM", "跳过 BOM 后正常解析命令和参数"),
        TEST_CASE(UT_LIST_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1),
                  UT_CASE_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1, 0x004),
                  test_msh_parse_supports_single_quotes, "MSH 单引号解析",
                  "参数使用单引号包裹空格", "单引号内容作为一个 argv"),
        TEST_CASE(UT_LIST_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1),
                  UT_CASE_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1, 0x005),
                  test_msh_parse_rejects_unclosed_quote, "MSH 未闭合 quote",
                  "命令行缺少结束引号", "解析失败并返回非零"),
        TEST_CASE(UT_LIST_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1),
                  UT_CASE_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1, 0x006),
                  test_msh_parse_rejects_too_many_arguments, "MSH 参数数量上限",
                  "构造超过 MSH_ARG_MAX 的参数列表", "解析失败并返回非零"),
        TEST_CASE(UT_LIST_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1),
                  UT_CASE_NO(UT_MOD_MSH, TEST_MSH_COMPONENT_PARSE, 0x1, 0x007),
                  test_msh_parse_rejects_null_inputs, "MSH 空输入保护",
                  "line 或 out 传入 NULL", "解析失败并返回非零"),
};

const size_t MSH_PARSE_CASE_COUNT =
        sizeof(MSH_PARSE_CASES) / sizeof(MSH_PARSE_CASES[0]);
