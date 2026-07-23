#include "fsc_test_common.h"

static int test_sysroot_custom_path_lifecycle_and_getters(void)
{
    fs_error_t err;
    fuid_t root_fuid;
    obj_handle_t handle;
    const char *path = "../output/tests/fsc/sysroot";

    test_fsc_prepare_temp_dir();
    fsc_sysroot_deinit();
    TEST_ASSERT_FALSE(fsc_sysroot_is_active());
    TEST_ASSERT_TRUE(fsc_sysroot_get_path() == NULL);

    err = fsc_sysroot_get_fuid(&root_fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsc_sysroot_get_handle(&handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fsc_sysroot_init(path);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fsc_sysroot_is_active());
    TEST_ASSERT_TRUE(fsc_sysroot_get_path() != NULL);

    err = fsc_sysroot_get_fuid(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsc_sysroot_get_fuid(&root_fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_dir(&root_fuid));

    err = fsc_sysroot_get_handle(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsc_sysroot_get_handle(&handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(handle.len > 0U);

    fsc_sysroot_deinit();
    TEST_ASSERT_FALSE(fsc_sysroot_is_active());
    test_fsc_cleanup_sysroot(path);
    return 0;
}


static int test_sysroot_rejects_empty_and_accepts_absolute_path(void)
{
    fs_error_t err;
    char cwd[FSC_SYSROOT_PATH_MAX];
    char abs_path[FSC_SYSROOT_PATH_MAX];
    const char *got_path;

    test_fsc_prepare_temp_dir();
    fsc_sysroot_deinit();
    err = fsc_sysroot_init("");
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    TEST_ASSERT_FALSE(fsc_sysroot_is_active());

    TEST_ASSERT_TRUE(getcwd(cwd, sizeof(cwd)) != NULL);
    TEST_ASSERT_TRUE(snprintf(abs_path, sizeof(abs_path),
                              "%s/../output/tests/fsc/sysroot-abs",
                              cwd) < (int)sizeof(abs_path));
    err = fsc_sysroot_init(abs_path);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    got_path = fsc_sysroot_get_path();
    TEST_ASSERT_TRUE(got_path != NULL);
    TEST_ASSERT_STR_EQ(got_path, abs_path);
    fsc_sysroot_deinit();
    test_fsc_cleanup_sysroot(abs_path);
    return 0;
}


const test_case_t FSC_SYSROOT_CASES[] = {
        TEST_CASE(
                UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_SYSROOT, 0x1),
                UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_SYSROOT, 0x1, 0x001),
                test_sysroot_custom_path_lifecycle_and_getters,
                "Sysroot 自定义路径生命周期",
                "使用 output/tests/fsc/sysroot 启动并注入未初始化/NULL getter",
                "启动后 active 且 FUID/handle 可读，非法 getter 返回 EINVAL"),
        TEST_CASE(
                UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_SYSROOT, 0x1),
                UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_SYSROOT, 0x1, 0x002),
                test_sysroot_rejects_empty_and_accepts_absolute_path,
                "Sysroot 空路径和绝对路径",
                "先传入空路径，再传入当前目录拼出的绝对路径",
                "空路径返回 EINVAL，绝对路径按原样保存并可反初始化"),
};

const size_t FSC_SYSROOT_CASE_COUNT =
        sizeof(FSC_SYSROOT_CASES) / sizeof(FSC_SYSROOT_CASES[0]);
