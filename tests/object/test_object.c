#include "object_test_common.h"

static int run_object_suite(const char *name,
                            const test_case_t *cases,
                            size_t count)
{
    return test_run_suite(name, cases, count);
}

int main(void)
{
    int failed;

    failed = 0;
    failed += run_object_suite("object/fuid",
                               OBJECT_FUID_CASES,
                               OBJECT_FUID_CASE_COUNT);
    failed += run_object_suite("object/objkey",
                               OBJECT_OBJKEY_CASES,
                               OBJECT_OBJKEY_CASE_COUNT);
    failed += run_object_suite("object/objmeta",
                               OBJECT_OBJMETA_CASES,
                               OBJECT_OBJMETA_CASE_COUNT);
    failed += run_object_suite("object/objruntime",
                               OBJECT_OBJRUNTIME_CASES,
                               OBJECT_OBJRUNTIME_CASE_COUNT);
    failed += run_object_suite("object/objtable",
                               OBJECT_OBJTABLE_CASES,
                               OBJECT_OBJTABLE_CASE_COUNT);
    failed += run_object_suite("object/objpool",
                               OBJECT_OBJPOOL_CASES,
                               OBJECT_OBJPOOL_CASE_COUNT);
    failed += run_object_suite("object/objmgr",
                               OBJECT_OBJMGR_CASES,
                               OBJECT_OBJMGR_CASE_COUNT);
    return failed == 0 ? 0 : 1;
}
