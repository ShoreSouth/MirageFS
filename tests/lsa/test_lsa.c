#include "lsa_test_common.h"

static int run_lsa_suite(const char *name, const test_case_t *cases,
                         size_t count)
{
    return test_run_suite(name, cases, count);
}

int main(void)
{
    int failed;

    /*
     * 入口只保留 lsa 组件清单和执行顺序；具体 case 放在
     * 对应组件测试文件中，方便按模块职责阅读和维护。
     */
    failed = 0;
    failed += run_lsa_suite("lsa/file", LSA_FILE_CASES, LSA_FILE_CASE_COUNT);
    failed += run_lsa_suite("lsa/error", LSA_ERROR_CASES, LSA_ERROR_CASE_COUNT);
    failed += run_lsa_suite("lsa/lookup", LSA_LOOKUP_CASES,
                            LSA_LOOKUP_CASE_COUNT);
    failed += run_lsa_suite("lsa/create", LSA_CREATE_CASES,
                            LSA_CREATE_CASE_COUNT);
    failed += run_lsa_suite("lsa/namespace", LSA_NAMESPACE_CASES,
                            LSA_NAMESPACE_CASE_COUNT);
    failed += run_lsa_suite("lsa/dir", LSA_DIR_CASES, LSA_DIR_CASE_COUNT);
    failed += run_lsa_suite("lsa/attr", LSA_ATTR_CASES, LSA_ATTR_CASE_COUNT);
    failed += run_lsa_suite("lsa/xattr", LSA_XATTR_CASES, LSA_XATTR_CASE_COUNT);
    failed += run_lsa_suite("lsa/handle", LSA_HANDLE_CASES,
                            LSA_HANDLE_CASE_COUNT);
    return failed == 0 ? 0 : 1;
}
