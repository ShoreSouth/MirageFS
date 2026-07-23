#include "framework/test_framework.h"

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "config/fs_config.h"


typedef enum test_config_component
{
    TEST_CONFIG_COMPONENT_CORE = 0x01,
} test_config_component_t;

static int test_config_init_loads_default_values(void)
{
    g_fs_config.mempool_size = 0;
    g_fs_config.worker_nr = 0;
    g_fs_config.trace_enable = false;
    g_fs_config.debug_enable = false;

    fs_config_init();

    TEST_ASSERT_EQ_INT(FS_DEFAULT_MEMPOOL_SIZE, g_fs_config.mempool_size);
    TEST_ASSERT_EQ_INT(FS_DEFAULT_WORKER_NR, g_fs_config.worker_nr);
    TEST_ASSERT_TRUE(g_fs_config.trace_enable);
    TEST_ASSERT_TRUE(g_fs_config.debug_enable);
    return 0;
}

static int test_config_init_is_idempotent(void)
{
    fs_config_init();
    fs_config_init();

    TEST_ASSERT_EQ_INT(FS_DEFAULT_MEMPOOL_SIZE, g_fs_config.mempool_size);
    TEST_ASSERT_EQ_INT(FS_DEFAULT_WORKER_NR, g_fs_config.worker_nr);
    return 0;
}

static int test_config_dump_prints_current_values(void)
{
    fs_config_init();
    fs_config_dump();

    TEST_ASSERT_EQ_INT(FS_DEFAULT_MEMPOOL_SIZE, g_fs_config.mempool_size);
    TEST_ASSERT_EQ_INT(FS_DEFAULT_WORKER_NR, g_fs_config.worker_nr);
    TEST_ASSERT_TRUE(g_fs_config.trace_enable);
    TEST_ASSERT_TRUE(g_fs_config.debug_enable);
    return 0;
}

static const test_case_t TEST_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_CONFIG, TEST_CONFIG_COMPONENT_CORE, 0x1),
                  UT_CASE_NO(UT_MOD_CONFIG, TEST_CONFIG_COMPONENT_CORE, 0x1,
                             0x001),
                  test_config_init_loads_default_values, "配置初始化",
                  "清空全局配置后重新初始化",
                  "恢复默认内存池、worker、trace/debug 开关"),
        TEST_CASE(UT_LIST_NO(UT_MOD_CONFIG, TEST_CONFIG_COMPONENT_CORE, 0x1),
                  UT_CASE_NO(UT_MOD_CONFIG, TEST_CONFIG_COMPONENT_CORE, 0x1,
                             0x002),
                  test_config_init_is_idempotent, "配置重复初始化",
                  "连续调用 fs_config_init", "默认配置保持稳定"),
        TEST_CASE(UT_LIST_NO(UT_MOD_CONFIG, TEST_CONFIG_COMPONENT_CORE, 0x1),
                  UT_CASE_NO(UT_MOD_CONFIG, TEST_CONFIG_COMPONENT_CORE, 0x1,
                             0x003),
                  test_config_dump_prints_current_values, "配置 dump 输出",
                  "初始化默认配置后调用 fs_config_dump",
                  "dump 路径可执行且配置值保持默认"),
};

int main(void)
{
    return test_run_suite("config", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
