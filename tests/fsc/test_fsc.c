#include "fsc_test_common.h"

static int run_fsc_suite(const char *name, const test_case_t *cases,
                         size_t count)
{
    return test_run_suite(name, cases, count);
}

int main(void)
{
    int failed;

    /*
     * 入口只保留 fsc 组件清单和执行顺序；具体 case 放在
     * 对应组件测试文件中，方便按模块职责阅读和维护。
     */
    failed = 0;
    failed += run_fsc_suite("fsc/error", FSC_ERROR_CASES, FSC_ERROR_CASE_COUNT);
    failed += run_fsc_suite("fsc/fsid", FSC_FSID_CASES, FSC_FSID_CASE_COUNT);
    failed += run_fsc_suite("fsc/namespace", FSC_NAMESPACE_CASES,
                            FSC_NAMESPACE_CASE_COUNT);
    failed += run_fsc_suite("fsc/fstable", FSC_FSTABLE_CASES,
                            FSC_FSTABLE_CASE_COUNT);
    failed += run_fsc_suite("fsc/nspool", FSC_NSPOOL_CASES,
                            FSC_NSPOOL_CASE_COUNT);
    failed += run_fsc_suite("fsc/sysroot", FSC_SYSROOT_CASES,
                            FSC_SYSROOT_CASE_COUNT);
    failed += run_fsc_suite("fsc/fsmgr", FSC_FSMGR_CASES, FSC_FSMGR_CASE_COUNT);
    failed += run_fsc_suite("fsc/init", FSC_INIT_CASES, FSC_INIT_CASE_COUNT);
    return failed == 0 ? 0 : 1;
}
