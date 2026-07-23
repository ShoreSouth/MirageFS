#include "fsc_test_common.h"

static int test_fsc_module_init_deinit_default_path(void)
{
    fs_error_t err;
    fuid_t root_fuid;
    obj_handle_t handle;
    FILE *root_file;

    (void)rmdir("./miragefs.root");
    (void)unlink("./miragefs.root");

    root_file = fopen("./miragefs.root", "w");
    TEST_ASSERT_TRUE(root_file != NULL);
    TEST_ASSERT_EQ_INT(fclose(root_file), 0);
    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_init();
    TEST_ASSERT_TRUE(fs_failed(err));
    object_deinit();
    TEST_ASSERT_EQ_INT(unlink("./miragefs.root"), 0);

    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fsc_sysroot_is_active());

    err = fsc_sysroot_get_fuid(&root_fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_sysroot_get_handle(&handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_dir(&root_fuid));
    TEST_ASSERT_TRUE(handle.len > 0U);

    fsc_deinit();
    object_deinit();
    (void)rmdir("./miragefs.root");
    return 0;
}


static int test_fsc_module_init_fault_rollbacks(void)
{
    static const char *faults[] = { "fsid", "nspool", "fsmgr" };
    fs_error_t err;
    size_t i;

    for (i = 0; i < sizeof(faults) / sizeof(faults[0]); i++) {
        (void)rmdir("./miragefs.root");
        (void)unlink("./miragefs.root");
        TEST_ASSERT_EQ_INT(setenv("MIRAGEFS_FSC_INIT_FAIL", faults[i], 1),
                           0);
        err = object_init();
        TEST_ASSERT_EQ_INT(FS_OK, err);
        err = fsc_init();
        TEST_ASSERT_TRUE(fs_failed(err));
        object_deinit();
        TEST_ASSERT_EQ_INT(unsetenv("MIRAGEFS_FSC_INIT_FAIL"), 0);
        (void)rmdir("./miragefs.root");
    }

    return 0;
}



const test_case_t FSC_INIT_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_INIT,
                             0x1),
                  UT_CASE_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_INIT,
                             0x1,
                             0x001),
                  test_fsc_module_init_deinit_default_path,
                  "FSC 模块总入口生命周期",
                  "调用 fsc_init/fsc_deinit 默认路径并读取 sysroot 信息",
                  "模块启动成功，默认 sysroot 可用，deinit 后临时目录可清理"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_INIT,
                             0x1),
                  UT_CASE_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_INIT,
                             0x1,
                             0x002),
                  test_fsc_module_init_fault_rollbacks,
                  "FSC 初始化回滚",
                  "测试编译开关下注入 fsid/nspool/fsmgr 初始化失败",
                  "fsc_init 返回失败且已创建的 sysroot/子模块被清理"),
};

const size_t FSC_INIT_CASE_COUNT = sizeof(FSC_INIT_CASES) / sizeof(FSC_INIT_CASES[0]);
