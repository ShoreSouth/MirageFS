#include "runtime_test_common.h"

static int test_runtime_initial_state_is_not_initialized(void)
{
    runtime_deinit();

    TEST_ASSERT_FALSE(runtime_is_initialized());
    TEST_ASSERT_FALSE(runtime_fs_is_active());
    TEST_ASSERT_TRUE(runtime_fs_current() == NULL);
    return 0;
}


static int test_runtime_init_rejects_repeat_and_auto_create_only(void)
{
    runtime_config_t cfg;
    runtime_config_t bad_cfg;
    char namespace_name[32];
    fs_error_t err;

    runtime_deinit();
    test_runtime_cleanup_root();
    memset(&bad_cfg, 0, sizeof(bad_cfg));
    bad_cfg.default_namespace = "missing";
    bad_cfg.auto_create = false;
    bad_cfg.auto_use = true;

    err = runtime_init(&bad_cfg);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_NAMESPACE, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    TEST_ASSERT_FALSE(runtime_is_initialized());
    runtime_deinit();
    test_runtime_cleanup_root();

    memset(&cfg, 0, sizeof(cfg));
    (void)snprintf(namespace_name,
                   sizeof(namespace_name),
                   "rt_auto_%ld",
                   (long)getpid());
    cfg.default_namespace = namespace_name;
    cfg.auto_create = true;
    cfg.auto_use = false;

    err = runtime_init(&cfg);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(runtime_is_initialized());
    TEST_ASSERT_FALSE(runtime_fs_is_active());
    err = runtime_init(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_INIT, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EALREADY, fs_err_errno(err));
    err = runtime_fs_destroy(namespace_name);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    runtime_deinit();
    test_runtime_cleanup_root();
    return 0;
}


const test_case_t RUNTIME_LIFECYCLE_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_RUNTIME,
                             TEST_RUNTIME_COMPONENT_LIFECYCLE,
                             0x1),
                  UT_CASE_NO(UT_MOD_RUNTIME,
                             TEST_RUNTIME_COMPONENT_LIFECYCLE,
                             0x1,
                             0x001),
                  test_runtime_initial_state_is_not_initialized,
                  "Runtime 初始状态",
                  "确保 runtime 处于 deinit 状态",
                  "未初始化且没有活动 namespace"),
    TEST_CASE(UT_LIST_NO(UT_MOD_RUNTIME,
                             TEST_RUNTIME_COMPONENT_LIFECYCLE,
                             0x1),
                  UT_CASE_NO(UT_MOD_RUNTIME,
                             TEST_RUNTIME_COMPONENT_LIFECYCLE,
                             0x1,
                             0x002),
                  test_runtime_init_rejects_repeat_and_auto_create_only,
                  "Runtime 初始化边界",
                  "使用 auto_create 但不 auto_use 启动后重复 init",
                  "namespace 被创建但未进入，重复初始化返回 RUNTIME/INIT/EALREADY"),
};

const size_t RUNTIME_LIFECYCLE_CASE_COUNT = sizeof(RUNTIME_LIFECYCLE_CASES) / sizeof(RUNTIME_LIFECYCLE_CASES[0]);
