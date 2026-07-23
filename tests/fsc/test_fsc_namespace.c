#include "fsc_test_common.h"

static int test_namespace_init_state_and_deinit(void)
{
    fs_error_t err;
    fsc_namespace_t ns;
    fsc_fsid_t fsid = ((fsc_fsid_t)1 << 32) | 1U;
    fuid_t root = test_fsc_make_root_fuid(fsid);
    obj_handle_t handle = test_fsc_make_root_handle();

    TEST_ASSERT_TRUE(fsc_namespace_name_is_valid("demo"));
    TEST_ASSERT_FALSE(fsc_namespace_name_is_valid(""));
    TEST_ASSERT_FALSE(fsc_namespace_name_is_valid(NULL));

    err = fsc_namespace_init(&ns, fsid, "demo", &root, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fsc_namespace_is_valid(&ns));
    TEST_ASSERT_EQ_INT(FSC_NAMESPACE_STATE_INIT, fsc_namespace_state(&ns));

    err = fsc_namespace_change_state(&ns, FSC_NAMESPACE_STATE_ACTIVE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(FSC_NAMESPACE_STATE_ACTIVE, fsc_namespace_state(&ns));

    err = fsc_namespace_change_state(&ns, FSC_NAMESPACE_STATE_INIT);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    fsc_namespace_dump(&ns);
    fsc_namespace_dump(NULL);
    fsc_namespace_deinit(&ns);
    fsc_namespace_deinit(NULL);
    TEST_ASSERT_FALSE(fsc_namespace_is_valid(&ns));
    return 0;
}


static int test_namespace_rejects_invalid_root_fuid(void)
{
    fs_error_t err;
    fsc_namespace_t ns;
    fsc_fsid_t fsid = ((fsc_fsid_t)1 << 32) | 2U;
    fuid_t wrong_type = fuid_make(fsid, 1, 1, FUID_TYPE_FILE);
    fuid_t wrong_fsid = fuid_make(fsid + 1U, 1, 1, FUID_TYPE_DIR);
    obj_handle_t handle = test_fsc_make_root_handle();

    err = fsc_namespace_init(&ns, fsid, "bad", &wrong_type, &handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fsc_namespace_init(&ns, fsid, "bad", &wrong_fsid, &handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}


static int test_namespace_rejects_invalid_inputs_and_deleting_edges(void)
{
    fs_error_t err;
    fsc_namespace_t ns;
    fsc_fsid_t fsid = ((fsc_fsid_t)1 << 32) | 12U;
    fuid_t root = test_fsc_make_root_fuid(fsid);
    obj_handle_t handle = test_fsc_make_root_handle();
    char long_name[FSC_NAMESPACE_NAME_MAX + 1U];

    memset(long_name, 'n', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = 0;

    TEST_ASSERT_FALSE(fsc_namespace_name_is_valid(long_name));
    TEST_ASSERT_EQ_INT(FSC_NAMESPACE_STATE_INVALID, fsc_namespace_state(NULL));
    TEST_ASSERT_TRUE(fsc_namespace_state_can_transit(
            FSC_NAMESPACE_STATE_INIT, FSC_NAMESPACE_STATE_ACTIVE));
    TEST_ASSERT_TRUE(fsc_namespace_state_can_transit(
            FSC_NAMESPACE_STATE_ACTIVE, FSC_NAMESPACE_STATE_DELETING));
    TEST_ASSERT_FALSE(fsc_namespace_state_can_transit(
            FSC_NAMESPACE_STATE_DELETING, FSC_NAMESPACE_STATE_ACTIVE));

    err = fsc_namespace_init(NULL, fsid, "bad", &root, &handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsc_namespace_init(&ns, FSID_INVALID, "bad", &root, &handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsc_namespace_init(&ns, fsid, long_name, &root, &handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsc_namespace_init(&ns, fsid, "bad", NULL, &handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsc_namespace_init(&ns, fsid, "bad", &root, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsc_namespace_change_state(NULL, FSC_NAMESPACE_STATE_ACTIVE);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fsc_namespace_init(&ns, fsid, "edge", &root, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_namespace_change_state(&ns, FSC_NAMESPACE_STATE_ACTIVE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_namespace_change_state(&ns, FSC_NAMESPACE_STATE_DELETING);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fsc_namespace_is_valid(&ns));
    return 0;
}


const test_case_t FSC_NAMESPACE_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NAMESPACE, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NAMESPACE, 0x1,
                             0x001),
                  test_namespace_init_state_and_deinit,
                  "Namespace 初始化和状态机",
                  "初始化后执行 INIT->ACTIVE，再尝试回退",
                  "合法迁移成功，非法迁移返回 EINVAL，deinit 后无效"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NAMESPACE, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NAMESPACE, 0x1,
                             0x002),
                  test_namespace_rejects_invalid_root_fuid,
                  "Namespace root 校验", "注入非目录 root 和 fsid 不匹配 root",
                  "返回 FSC/NAMESPACE/EINVAL"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NAMESPACE, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_NAMESPACE, 0x1,
                             0x003),
                  test_namespace_rejects_invalid_inputs_and_deleting_edges,
                  "Namespace 参数和删除态边界",
                  "注入 NULL、非法 fsid、超长名称以及 ACTIVE->DELETING 状态",
                  "非法输入返回 EINVAL，DELETING 仍是合法生命周期状态"),
};

const size_t FSC_NAMESPACE_CASE_COUNT =
        sizeof(FSC_NAMESPACE_CASES) / sizeof(FSC_NAMESPACE_CASES[0]);
