#include "framework/test_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msh/include/msh.h"
#include "msh/internal/msh_internal.h"


typedef enum test_msh_component {
    TEST_MSH_COMPONENT_PARSE = 0x01,
    TEST_MSH_COMPONENT_ARGS = 0x02,
    TEST_MSH_COMPONENT_COMMAND = 0x03,
} test_msh_component_t;

static void test_msh_cleanup_root(void)
{
    int rc;

    rc = system("rm -rf -- './miragefs.root'");
    (void)rc;
}

static int test_msh_run_line(msh_context_t *ctx, const char *line)
{
    char line_buf[MSH_LINE_MAX];
    int written;

    written = snprintf(line_buf, sizeof(line_buf), "%s", line);
    TEST_ASSERT_TRUE((written >= 0) && ((size_t)written < sizeof(line_buf)));
    return msh_run_line(ctx, line_buf);
}

static int test_msh_repl_with_stdin(const char *input, bool interactive)
{
    msh_context_t ctx;
    FILE *tmp;
    int saved_stdin;
    int rc;

    saved_stdin = dup(STDIN_FILENO);
    TEST_ASSERT_TRUE(saved_stdin >= 0);
    tmp = tmpfile();
    TEST_ASSERT_TRUE(tmp != NULL);
    if (input != NULL) {
        TEST_ASSERT_EQ_INT(fwrite(input, 1U, strlen(input), tmp),
                           strlen(input));
    }
    rewind(tmp);
    TEST_ASSERT_EQ_INT(dup2(fileno(tmp), STDIN_FILENO), STDIN_FILENO);
    clearerr(stdin);

    memset(&ctx, 0, sizeof(ctx));
    ctx.interactive = interactive;
    rc = msh_repl(&ctx);

    TEST_ASSERT_EQ_INT(dup2(saved_stdin, STDIN_FILENO), STDIN_FILENO);
    clearerr(stdin);
    close(saved_stdin);
    fclose(tmp);
    return rc;
}

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
    char line[] = "\xef\xbb\xbf" "fs list";
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

static int test_msh_arg_or_default_handles_missing_arg(void)
{
    char line[] = "fs use";
    msh_argv_t args;

    TEST_ASSERT_EQ_INT(msh_parse_line(line, &args), 0);
    TEST_ASSERT_STR_EQ(msh_arg_or_default(&args, 1, "default"), "use");
    TEST_ASSERT_STR_EQ(msh_arg_or_default(&args, 2, "default"), "default");
    TEST_ASSERT_STR_EQ(msh_arg_or_default(NULL, 0, "default"), "default");
    return 0;
}

static int test_msh_meta_dispatch_and_main_errors(void)
{
    msh_context_t ctx;
    char *bad_argv[] = { "msh", "--bad" };

    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "version"), 0);
    TEST_ASSERT_FALSE(ctx.should_exit);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "help"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "quit"), 0);
    TEST_ASSERT_TRUE(ctx.should_exit);

    ctx.should_exit = false;
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "unknown"), 1);
    TEST_ASSERT_EQ_INT(msh_dispatch(NULL, NULL), 0);
    TEST_ASSERT_EQ_INT(msh_main(2, bad_argv), 1);
    return 0;
}

static int test_msh_fs_commands_manage_namespace(void)
{
    msh_context_t ctx;
    char namespace_name[32];
    fs_error_t err;

    runtime_deinit();
    test_msh_cleanup_root();
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ_INT(runtime_init(NULL), FS_OK);
    (void)snprintf(namespace_name,
                   sizeof(namespace_name),
                   "msh_%ld",
                   (long)getpid());

    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs current"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs missing"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs create"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs use"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs leave extra"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs current extra"), 1);

    {
        char command[MSH_LINE_MAX];

        (void)snprintf(command, sizeof(command), "fs create %s",
                       namespace_name);
        TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, command), 0);
        TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, command), 1);
        (void)snprintf(command, sizeof(command), "fs use %s",
                       namespace_name);
        TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, command), 0);
    }

    TEST_ASSERT_TRUE(runtime_fs_is_active());
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs current"), 0);
    msh_print_prompt();
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs leave"), 0);
    TEST_ASSERT_FALSE(runtime_fs_is_active());

    err = runtime_fs_destroy(namespace_name);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    runtime_deinit();
    test_msh_cleanup_root();
    return 0;
}

static int test_msh_file_commands_round_trip(void)
{
    runtime_config_t cfg;
    msh_context_t ctx;
    char namespace_name[32];
    fs_error_t err;

    runtime_deinit();
    test_msh_cleanup_root();
    memset(&ctx, 0, sizeof(ctx));
    memset(&cfg, 0, sizeof(cfg));
    (void)snprintf(namespace_name,
                   sizeof(namespace_name),
                   "msh_%ld",
                   (long)getpid());
    cfg.default_namespace = namespace_name;
    cfg.auto_create = true;
    cfg.auto_use = true;
    TEST_ASSERT_EQ_INT(runtime_init(&cfg), FS_OK);

    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "pwd"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "mkdir /dir"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "touch /dir/file.txt"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "stat /dir/file.txt"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "ls /dir"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "ll /dir"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "ls"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "ll"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "cd /dir/file.txt"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "stat /missing"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "ls /dir/file.txt"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "ll /dir/file.txt"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "rm /dir"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "rmdir /dir/file.txt"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx,
                                         "mv /dir/file.txt /missing/file.txt"),
                       1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "cd /dir"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "pwd"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx,
                                         "mv /dir/file.txt /dir/moved.txt"),
                       0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "rm /dir/moved.txt"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "cd /"), 0);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "rmdir /dir"), 0);

    err = runtime_fs_destroy(namespace_name);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    runtime_deinit();
    test_msh_cleanup_root();
    return 0;
}

static int test_msh_command_error_paths(void)
{
    msh_context_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    runtime_deinit();
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "pwd"), 1);
    TEST_ASSERT_EQ_INT(runtime_init(NULL), FS_OK);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "pwd"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "cd"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "cd a b"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "mkdir"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "touch"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "rm"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "rmdir"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "mv old"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "stat"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "ls a b"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "ll a b"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs use missing"), 1);
    TEST_ASSERT_EQ_INT(test_msh_run_line(&ctx, "fs leave"), 0);
    runtime_deinit();
    test_msh_cleanup_root();
    return 0;
}

static int test_msh_repl_handles_eof_errors_and_long_lines(void)
{
    char long_line[MSH_LINE_MAX + 16U];

    runtime_deinit();
    test_msh_cleanup_root();
    TEST_ASSERT_EQ_INT(msh_repl(NULL), 1);
    TEST_ASSERT_EQ_INT(test_msh_repl_with_stdin("", false), 0);
    TEST_ASSERT_EQ_INT(test_msh_repl_with_stdin("pwd\n", false), 1);
    TEST_ASSERT_EQ_INT(test_msh_repl_with_stdin("unknown\nquit\n", true), 0);

    memset(long_line, 'a', sizeof(long_line));
    long_line[sizeof(long_line) - 1U] = 0;
    TEST_ASSERT_EQ_INT(test_msh_repl_with_stdin(long_line, false), 0);
    return 0;
}

static int test_msh_main_command_argument_paths(void)
{
    char long_arg[MSH_LINE_MAX + 8U];
    char *version_argv[] = { "msh", "-c", "version" };
    char *unknown_argv[] = { "msh", "-c", "unknown" };
    char *long_argv[] = { "msh", "-c", long_arg };

    runtime_deinit();
    test_msh_cleanup_root();
    TEST_ASSERT_EQ_INT(msh_main(3, version_argv), 0);
    runtime_deinit();
    test_msh_cleanup_root();
    TEST_ASSERT_EQ_INT(msh_main(3, unknown_argv), 1);
    runtime_deinit();
    test_msh_cleanup_root();

    memset(long_arg, 'x', sizeof(long_arg));
    long_arg[sizeof(long_arg) - 1U] = 0;
    TEST_ASSERT_EQ_INT(msh_main(3, long_argv), 1);
    runtime_deinit();
    test_msh_cleanup_root();
    return 0;
}

static const test_case_t TEST_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1,
                         0x001),
              test_msh_parse_splits_arguments_and_quotes,
              "MSH 命令解析",
              "带双引号参数的命令行",
              "argc/argv 保留 quoted 参数整体"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1,
                         0x002),
              test_msh_parse_ignores_comment_after_spaces,
              "MSH 注释行解析",
              "空白后跟 # 注释",
              "解析为 0 个参数"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1,
                         0x003),
              test_msh_parse_handles_utf8_bom_prefix,
              "MSH BOM 前缀解析",
              "命令行开头带 UTF-8 BOM",
              "跳过 BOM 后正常解析命令和参数"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1,
                         0x004),
              test_msh_parse_supports_single_quotes,
              "MSH 单引号解析",
              "参数使用单引号包裹空格",
              "单引号内容作为一个 argv"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1,
                         0x005),
              test_msh_parse_rejects_unclosed_quote,
              "MSH 未闭合 quote",
              "命令行缺少结束引号",
              "解析失败并返回非零"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1,
                         0x006),
              test_msh_parse_rejects_too_many_arguments,
              "MSH 参数数量上限",
              "构造超过 MSH_ARG_MAX 的参数列表",
              "解析失败并返回非零"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_PARSE,
                         0x1,
                         0x007),
              test_msh_parse_rejects_null_inputs,
              "MSH 空输入保护",
              "line 或 out 传入 NULL",
              "解析失败并返回非零"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_ARGS,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_ARGS,
                         0x1,
                         0x001),
              test_msh_arg_or_default_handles_missing_arg,
              "MSH 参数默认值",
              "读取存在参数、越界参数和 NULL args",
              "存在参数原样返回，缺失参数返回 fallback"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1,
                         0x001),
              test_msh_meta_dispatch_and_main_errors,
              "MSH meta 命令和入口错误",
              "执行 help/version/quit/unknown，并向 main 注入非法参数",
              "元命令成功，未知命令和非法 argv 返回失败"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1,
                         0x002),
              test_msh_fs_commands_manage_namespace,
              "MSH fs 命令 namespace 生命周期",
              "通过 fs create/use/current/leave 管理真实 namespace",
              "namespace 可创建、进入、查询、退出并销毁"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1,
                         0x003),
              test_msh_file_commands_round_trip,
              "MSH 文件命令回环",
              "进入真实 namespace 后执行 pwd/mkdir/touch/stat/ls/ll/cd/mv/rm/rmdir",
              "文件命令经 runtime 主链路成功完成并清理 namespace"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1,
                         0x004),
              test_msh_command_error_paths,
              "MSH 命令错误路径",
              "注入无会话、缺参数、过多参数、缺失 namespace 和幂等 leave",
              "错误输入返回失败，幂等 leave 成功且 runtime 可正常反初始化"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1,
                         0x005),
              test_msh_repl_handles_eof_errors_and_long_lines,
              "MSH REPL 输入循环边界",
              "注入 EOF、未知命令、交互模式继续执行和超长输入行",
              "REPL 按交互/非交互语义返回，超长行被消费后继续到 EOF"),
    TEST_CASE(UT_LIST_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1),
              UT_CASE_NO(UT_MOD_MSH,
                         TEST_MSH_COMPONENT_COMMAND,
                         0x1,
                         0x006),
              test_msh_main_command_argument_paths,
              "MSH -c 命令入口",
              "通过 msh_main 执行成功命令、未知命令和超长命令参数",
              "-c 成功时返回 0，命令失败或命令过长时返回 1"),
};

int main(void)
{
    return test_run_suite("msh", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
