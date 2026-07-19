#include "fops_test_common.h"

static int run_fops_suite(const char *name,
                          const test_case_t *cases,
                          size_t count)
{
    return test_run_suite(name, cases, count);
}

int main(void)
{
    int failed;

    failed = 0;
    failed += run_fops_suite("fops/core",
                             FOPS_CORE_CASES,
                             FOPS_CORE_CASE_COUNT);
    failed += run_fops_suite("fops/dispatch",
                             FOPS_DISPATCH_CASES,
                             FOPS_DISPATCH_CASE_COUNT);
    failed += run_fops_suite("fops/validate",
                             FOPS_VALIDATE_CASES,
                             FOPS_VALIDATE_CASE_COUNT);
    failed += run_fops_suite("fops/create",
                             FOPS_CREATE_CASES,
                             FOPS_CREATE_CASE_COUNT);
    failed += run_fops_suite("fops/file",
                             FOPS_FILE_CASES,
                             FOPS_FILE_CASE_COUNT);
    failed += run_fops_suite("fops/attr",
                             FOPS_ATTR_CASES,
                             FOPS_ATTR_CASE_COUNT);
    failed += run_fops_suite("fops/identity",
                             FOPS_IDENTITY_CASES,
                             FOPS_IDENTITY_CASE_COUNT);
    failed += run_fops_suite("fops/handle",
                             FOPS_HANDLE_CASES,
                             FOPS_HANDLE_CASE_COUNT);
    failed += run_fops_suite("fops/ops",
                             FOPS_OPS_CASES,
                             FOPS_OPS_CASE_COUNT);
    failed += run_fops_suite("fops/boundary",
                             FOPS_BOUNDARY_CASES,
                             FOPS_BOUNDARY_CASE_COUNT);
    return failed == 0 ? 0 : 1;
}
