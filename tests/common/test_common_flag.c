#include "common_test_common.h"

static int test_flag_helper_detects_set_and_missing_bits(void)
{
    fs_flags_t flags = FS_FLAG_READ | FS_FLAG_WRITE | FS_FLAG_SYNC;

    TEST_ASSERT_TRUE(fs_flag_test(flags, FS_FLAG_READ));
    TEST_ASSERT_TRUE(fs_flag_test(flags, FS_FLAG_WRITE));
    TEST_ASSERT_FALSE(fs_flag_test(flags, FS_FLAG_DIRECTORY));
    TEST_ASSERT_FALSE(fs_flag_test(FS_FLAG_NONE, FS_FLAG_READ));
    return 0;
}


const test_case_t COMMON_FLAG_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                             TEST_COMMON_COMPONENT_FLAG,
                             0x1),
                  UT_CASE_NO(UT_MOD_COMMON,
                             TEST_COMMON_COMPONENT_FLAG,
                             0x1,
                             0x001),
                  test_flag_helper_detects_set_and_missing_bits,
                  "flag 位检测",
                  "构造 READ/WRITE/SYNC 组合并检测缺失位",
                  "已设置 flag 为 true，未设置 flag 为 false"),
};

const size_t COMMON_FLAG_CASE_COUNT = sizeof(COMMON_FLAG_CASES) / sizeof(COMMON_FLAG_CASES[0]);
