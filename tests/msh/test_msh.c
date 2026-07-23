#include "msh_test_common.h"

static int run_msh_suite(const char *name,
                          const test_case_t *cases,
                          size_t count)
{
    return test_run_suite(name, cases, count);
}

int main(void)
{
    int failed;

    /*
     * 入口只保留 msh 组件清单和执行顺序；具体 case 放在
     * 对应组件测试文件中，方便按模块职责阅读和维护。
     */
    failed = 0;
    failed += run_msh_suite("msh/parse",
                               MSH_PARSE_CASES,
                               MSH_PARSE_CASE_COUNT);
    failed += run_msh_suite("msh/args",
                               MSH_ARGS_CASES,
                               MSH_ARGS_CASE_COUNT);
    failed += run_msh_suite("msh/command",
                               MSH_COMMAND_CASES,
                               MSH_COMMAND_CASE_COUNT);
    return failed == 0 ? 0 : 1;
}
