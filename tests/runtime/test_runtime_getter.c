#include "runtime_test_common.h"

static int test_runtime_getters_reject_null_outputs(void)
{
    fs_error_t err;

    err = runtime_get_root(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = runtime_get_cwd(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = runtime_get_ctx(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_CTX, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = runtime_getcwd(NULL, 8U);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = runtime_getcwd((char *)&err, 0U);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = runtime_dispatch(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_OP, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = runtime_update_cwd_path(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = runtime_update_cwd_path("");
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}


const test_case_t RUNTIME_GETTER_CASES[] = {
        TEST_CASE(
                UT_LIST_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_GETTER, 0x1),
                UT_CASE_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_GETTER, 0x1,
                           0x001),
                test_runtime_getters_reject_null_outputs,
                "Runtime getter 空输出", "root/cwd getter 传入 NULL",
                "返回 RUNTIME 模块 EINVAL"),
};

const size_t RUNTIME_GETTER_CASE_COUNT =
        sizeof(RUNTIME_GETTER_CASES) / sizeof(RUNTIME_GETTER_CASES[0]);
