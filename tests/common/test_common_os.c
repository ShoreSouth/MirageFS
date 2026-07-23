#include "common_test_common.h"

static int test_os_helpers_return_cached_names_and_times(void)
{
    const char *thread_name;
    const char *thread_name_cached;
    const char *process_name;
    const char *time_text;

    TEST_ASSERT_TRUE(fs_get_tid() > 0U);
    thread_name = fs_get_thread_name();
    thread_name_cached = fs_get_thread_name();
    process_name = fs_get_process_name();
    time_text = fs_time_str();

    TEST_ASSERT_TRUE(thread_name != NULL);
    TEST_ASSERT_TRUE(thread_name[0] != '\0');
    TEST_ASSERT_TRUE(thread_name == thread_name_cached);
    TEST_ASSERT_TRUE(process_name != NULL);
    TEST_ASSERT_TRUE(process_name[0] != '\0');
    TEST_ASSERT_TRUE(fs_get_time_s() > 0U);
    TEST_ASSERT_TRUE(fs_get_time_ms() > 0U);
    TEST_ASSERT_TRUE(fs_get_time_us() > 0U);
    TEST_ASSERT_TRUE(fs_get_time_ns() < 1000000000ULL);
    TEST_ASSERT_TRUE(fs_get_monotonic_s() > 0U);
    TEST_ASSERT_TRUE(fs_get_monotonic_ms() > 0U);
    TEST_ASSERT_TRUE(fs_get_monotonic_us() > 0U);
    TEST_ASSERT_TRUE(fs_get_monotonic_ns() < 1000000000ULL);
    TEST_ASSERT_TRUE(time_text != NULL);
    TEST_ASSERT_TRUE(time_text[0] != '\0');
    return 0;
}


const test_case_t COMMON_OS_CASES[] = {
        TEST_CASE(
                UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_OS, 0x1),
                UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_OS, 0x1, 0x001),
                test_os_helpers_return_cached_names_and_times,
                "OS helper 线程进程名和时间",
                "读取线程名、进程名、实时时间、单调时间和格式化时间",
                "返回非空名称和合理时间值，并命中线程名缓存"),
};

const size_t COMMON_OS_CASE_COUNT =
        sizeof(COMMON_OS_CASES) / sizeof(COMMON_OS_CASES[0]);
