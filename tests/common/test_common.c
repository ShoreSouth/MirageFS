#include "common_test_common.h"

static int run_common_suite(const char *name, const test_case_t *cases,
                            size_t count)
{
    return test_run_suite(name, cases, count);
}

int main(void)
{
    int failed;

    /*
     * 入口只保留 common 组件清单和执行顺序；具体 case 放在
     * 对应组件测试文件中，方便按模块职责阅读和维护。
     */
    failed = 0;
    failed += run_common_suite("common/error", COMMON_ERROR_CASES,
                               COMMON_ERROR_CASE_COUNT);
    failed += run_common_suite("common/module", COMMON_MODULE_CASES,
                               COMMON_MODULE_CASE_COUNT);
    failed += run_common_suite("common/flag", COMMON_FLAG_CASES,
                               COMMON_FLAG_CASE_COUNT);
    failed += run_common_suite("common/type", COMMON_TYPE_CASES,
                               COMMON_TYPE_CASE_COUNT);
    failed += run_common_suite("common/path", COMMON_PATH_CASES,
                               COMMON_PATH_CASE_COUNT);
    failed += run_common_suite("common/atomic", COMMON_ATOMIC_CASES,
                               COMMON_ATOMIC_CASE_COUNT);
    failed += run_common_suite("common/lock", COMMON_LOCK_CASES,
                               COMMON_LOCK_CASE_COUNT);
    failed += run_common_suite("common/os", COMMON_OS_CASES,
                               COMMON_OS_CASE_COUNT);
    failed += run_common_suite("common/mempool", COMMON_MEMPOOL_CASES,
                               COMMON_MEMPOOL_CASE_COUNT);
    failed += run_common_suite("common/hash", COMMON_HASH_CASES,
                               COMMON_HASH_CASE_COUNT);
    return failed == 0 ? 0 : 1;
}
