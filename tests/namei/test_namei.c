#include "namei_test_common.h"

static int run_namei_suite(const char *name, const test_case_t *cases,
                           size_t count)
{
    return test_run_suite(name, cases, count);
}

int main(void)
{
    int failed;

    /*
     * 入口只保留 namei 组件清单和执行顺序；具体 case 放在
     * 对应组件测试文件中，方便按模块职责阅读和维护。
     */
    failed = 0;
    failed += run_namei_suite("namei/lifecycle", NAMEI_LIFECYCLE_CASES,
                              NAMEI_LIFECYCLE_CASE_COUNT);
    failed +=
            run_namei_suite("namei/ctx", NAMEI_CTX_CASES, NAMEI_CTX_CASE_COUNT);
    failed += run_namei_suite("namei/lookup", NAMEI_LOOKUP_CASES,
                              NAMEI_LOOKUP_CASE_COUNT);
    failed += run_namei_suite("namei/flow", NAMEI_FLOW_CASES,
                              NAMEI_FLOW_CASE_COUNT);
    return failed == 0 ? 0 : 1;
}
