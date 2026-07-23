#include "msh_test_common.h"

static int test_msh_meta_dispatch_and_main_errors(void)
{
    msh_context_t ctx;
    char *bad_argv[] = { "msh", "--bad" };

    memset(&ctx, 0, sizeof(ctx));
    /* meta 命令不依赖 runtime 会话；quit 只改变 context 的退出标志。 */
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

    /* 动态 namespace 名称避免并行或重复执行时撞到上一次测试残留。 */
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

    /* 命令层只断言返回码，具体语义由 runtime/namei/fops 对应 UT 覆盖。 */
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
    /* 未初始化、未进入 namespace、缺参数和过多参数分别覆盖命令早退路径。 */
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

    /* 超长输入需要被消费到行尾，否则下一轮 REPL 会读到残留片段。 */
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


const test_case_t MSH_COMMAND_CASES[] = {
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

const size_t MSH_COMMAND_CASE_COUNT = sizeof(MSH_COMMAND_CASES) / sizeof(MSH_COMMAND_CASES[0]);
