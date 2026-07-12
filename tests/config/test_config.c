#include "framework/test_framework.h"

#include <stdbool.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "config/fs_config.h"

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

static const test_case_t TEST_CASES[] = {
    TEST_CASE(test_config_init_loads_default_values,
              "配置初始化",
              "清空全局配置后重新初始化",
              "恢复默认内存池、worker、trace/debug 开关"),
    TEST_CASE(test_config_init_is_idempotent,
              "配置重复初始化",
              "连续调用 fs_config_init",
              "默认配置保持稳定"),
};

int main(void)
{
    return test_run_suite("config", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
