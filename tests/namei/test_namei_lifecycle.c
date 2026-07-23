#include "namei_test_common.h"

static int test_namei_lifecycle_is_repeatable(void)
{
    fs_error_t err;

    err = namei_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = namei_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    namei_deinit();
    namei_deinit();
    return 0;
}


const test_case_t NAMEI_LIFECYCLE_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_NAMEI, TEST_NAMEI_COMPONENT_LIFECYCLE, 0x1),
                  UT_CASE_NO(UT_MOD_NAMEI, TEST_NAMEI_COMPONENT_LIFECYCLE, 0x1,
                             0x001),
                  test_namei_lifecycle_is_repeatable, "NAMEI 生命周期",
                  "重复 init/deinit", "初始化成功，重复反初始化不崩溃"),
};

const size_t NAMEI_LIFECYCLE_CASE_COUNT =
        sizeof(NAMEI_LIFECYCLE_CASES) / sizeof(NAMEI_LIFECYCLE_CASES[0]);
