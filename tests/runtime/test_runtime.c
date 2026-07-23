#include "runtime_test_common.h"

static int run_runtime_suite(const char *name,
                          const test_case_t *cases,
                          size_t count)
{
    return test_run_suite(name, cases, count);
}

int main(void)
{
    int failed;

    /*
     * 入口只保留 runtime 组件清单和执行顺序；具体 case 放在
     * 对应组件测试文件中，方便按模块职责阅读和维护。
     */
    failed = 0;
    failed += run_runtime_suite("runtime/sub",
                               RUNTIME_SUB_CASES,
                               RUNTIME_SUB_CASE_COUNT);
    failed += run_runtime_suite("runtime/lifecycle",
                               RUNTIME_LIFECYCLE_CASES,
                               RUNTIME_LIFECYCLE_CASE_COUNT);
    failed += run_runtime_suite("runtime/session",
                               RUNTIME_SESSION_CASES,
                               RUNTIME_SESSION_CASE_COUNT);
    failed += run_runtime_suite("runtime/getter",
                               RUNTIME_GETTER_CASES,
                               RUNTIME_GETTER_CASE_COUNT);
    failed += run_runtime_suite("runtime/flow",
                               RUNTIME_FLOW_CASES,
                               RUNTIME_FLOW_CASE_COUNT);
    return failed == 0 ? 0 : 1;
}
