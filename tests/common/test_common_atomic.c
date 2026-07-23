#include "common_test_common.h"

static int test_atomic_wrappers_update_and_compare_values(void)
{
    fs_atomic32_t atom32;
    fs_atomic64_t atom64;
    int32_t expected32;
    int64_t expected64;

    fs_atomic32_init(&atom32, 1);
    TEST_ASSERT_EQ_INT(1, fs_atomic32_load(&atom32));
    fs_atomic32_store(&atom32, 10);
    TEST_ASSERT_EQ_INT(11, fs_atomic32_inc(&atom32));
    TEST_ASSERT_EQ_INT(10, fs_atomic32_dec(&atom32));
    TEST_ASSERT_EQ_INT(15, fs_atomic32_add(&atom32, 5));
    TEST_ASSERT_EQ_INT(12, fs_atomic32_sub(&atom32, 3));
    expected32 = 12;
    TEST_ASSERT_TRUE(fs_atomic32_cas(&atom32, &expected32, 20));
    TEST_ASSERT_EQ_INT(20, fs_atomic32_load(&atom32));
    expected32 = 12;
    TEST_ASSERT_FALSE(fs_atomic32_cas(&atom32, &expected32, 30));
    TEST_ASSERT_EQ_INT(20, expected32);

    fs_atomic64_init(&atom64, 100);
    TEST_ASSERT_EQ_INT(100, fs_atomic64_load(&atom64));
    fs_atomic64_store(&atom64, 1000);
    TEST_ASSERT_EQ_INT(1001, fs_atomic64_inc(&atom64));
    TEST_ASSERT_EQ_INT(1000, fs_atomic64_dec(&atom64));
    TEST_ASSERT_EQ_INT(1025, fs_atomic64_add(&atom64, 25));
    TEST_ASSERT_EQ_INT(1000, fs_atomic64_sub(&atom64, 25));
    expected64 = 1000;
    TEST_ASSERT_TRUE(fs_atomic64_cas(&atom64, &expected64, 2000));
    expected64 = 1000;
    TEST_ASSERT_FALSE(fs_atomic64_cas(&atom64, &expected64, 3000));
    TEST_ASSERT_EQ_INT(2000, expected64);
    return 0;
}


const test_case_t COMMON_ATOMIC_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_ATOMIC, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_ATOMIC, 0x1,
                             0x001),
                  test_atomic_wrappers_update_and_compare_values,
                  "Atomic 包装器读写和 CAS",
                  "对 32/64 位 atomic 执行 init/load/store/inc/dec/add/sub/cas",
                  "返回修改后的值，CAS 成功替换、失败回写 expected"),
};

const size_t COMMON_ATOMIC_CASE_COUNT =
        sizeof(COMMON_ATOMIC_CASES) / sizeof(COMMON_ATOMIC_CASES[0]);
