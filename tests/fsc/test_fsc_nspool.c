#include "fsc_test_common.h"

static int test_nspool_alloc_free_and_repeat_deinit(void)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    nspool_deinit();
    TEST_ASSERT_TRUE(nspool_alloc() == NULL);
    nspool_free(NULL);

    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    ns = nspool_alloc();
    TEST_ASSERT_TRUE(ns != NULL);

    /*
     * nspool_free() 会先 deinit namespace，再把内存交还给 mempool。
     * 这里故意传入一个未 init 的 namespace，验证清零对象也能安全释放。
     */
    nspool_free(ns);
    nspool_deinit();
    nspool_deinit();
    return 0;
}


static int test_nspool_reports_exhaustion(void)
{
    enum
    {
        TEST_NSPOOL_ALLOC_CAP = 1024
    };
    fsc_namespace_t *allocated[TEST_NSPOOL_ALLOC_CAP];
    fsc_namespace_t *extra;
    fs_error_t err;
    size_t allocated_nr;
    size_t i;

    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    allocated_nr = 0U;
    while (allocated_nr < TEST_NSPOOL_ALLOC_CAP)
    {
        extra = nspool_alloc();
        if (extra == NULL)
        {
            break;
        }
        allocated[allocated_nr] = extra;
        allocated_nr++;
    }
    TEST_ASSERT_TRUE(allocated_nr > 0U);
    extra = nspool_alloc();
    TEST_ASSERT_TRUE(extra == NULL);

    for (i = 0; i < allocated_nr; i++)
    {
        nspool_free(allocated[i]);
    }
    nspool_deinit();
    return 0;
}


const test_case_t FSC_NSPOOL_CASES[] = {
        TEST_CASE(
                UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NSPOOL, 0x1),
                UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NSPOOL, 0x1, 0x001),
                test_nspool_alloc_free_and_repeat_deinit, "NSPool 独立生命周期",
                "未初始化 alloc、NULL free、初始化后 alloc/free 和重复 deinit",
                "未初始化申请返回 NULL，释放和重复销毁安全"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NSPOOL, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NSPOOL, 0x1, 0x002),
                  test_nspool_reports_exhaustion, "NSPool 容量耗尽",
                  "连续申请完默认 namespace 池后再申请一次",
                  "额外申请返回 NULL，已申请对象可全部释放"),
};

const size_t FSC_NSPOOL_CASE_COUNT =
        sizeof(FSC_NSPOOL_CASES) / sizeof(FSC_NSPOOL_CASES[0]);
