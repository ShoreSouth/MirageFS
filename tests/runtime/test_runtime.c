#include "framework/test_framework.h"

#include <errno.h>

#include "common/fs_common.h"
#include "runtime/include/runtime.h"


typedef enum test_runtime_component {
    TEST_RUNTIME_COMPONENT_LIFECYCLE = 0x01,
    TEST_RUNTIME_COMPONENT_SESSION = 0x02,
    TEST_RUNTIME_COMPONENT_GETTER = 0x03,
} test_runtime_component_t;

static int test_runtime_initial_state_is_not_initialized(void)
{
    runtime_deinit();

    TEST_ASSERT_FALSE(runtime_is_initialized());
    TEST_ASSERT_FALSE(runtime_fs_is_active());
    TEST_ASSERT_TRUE(runtime_fs_current() == NULL);
    return 0;
}

static int test_runtime_requires_init_for_session_ops(void)
{
    fs_error_t err;
    char cwd[8];

    runtime_deinit();

    err = runtime_getcwd(cwd, sizeof(cwd));
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = runtime_fs_leave();
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static int test_runtime_getters_reject_null_outputs(void)
{
    fs_error_t err;

    err = runtime_get_root(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = runtime_get_cwd(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static const test_case_t TEST_CASES[] = {
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
                         TEST_RUNTIME_COMPONENT_SESSION,
                         0x1),
              UT_CASE_NO(UT_MOD_RUNTIME,
                         TEST_RUNTIME_COMPONENT_SESSION,
                         0x1,
                         0x001),
              test_runtime_requires_init_for_session_ops,
              "Runtime 未初始化保护",
              "未 init 时调用 getcwd/leave",
              "返回 RUNTIME 模块 EINVAL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_RUNTIME,
                         TEST_RUNTIME_COMPONENT_GETTER,
                         0x1),
              UT_CASE_NO(UT_MOD_RUNTIME,
                         TEST_RUNTIME_COMPONENT_GETTER,
                         0x1,
                         0x001),
              test_runtime_getters_reject_null_outputs,
              "Runtime getter 空输出",
              "root/cwd getter 传入 NULL",
              "返回 RUNTIME 模块 EINVAL"),
};

int main(void)
{
    return test_run_suite("runtime", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
