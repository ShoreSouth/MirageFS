#include "fsc_test_common.h"

static uint32_t g_fstable_reclaim_count;

static void test_fstable_reclaim_count(fsc_namespace_t *ns)
{
    if (ns != NULL)
    {
        g_fstable_reclaim_count++;
    }
}

static int test_fstable_insert_lookup_remove_by_two_indexes(void)
{
    fs_error_t err;
    fsc_table_t table;
    fsc_namespace_t ns;
    fsc_namespace_t *removed = NULL;
    fsc_fsid_t fsid = ((fsc_fsid_t)1 << 32) | 3U;
    fuid_t root = test_fsc_make_root_fuid(fsid);
    obj_handle_t handle = test_fsc_make_root_handle();

    err = fsc_namespace_init(&ns, fsid, "alpha", &root, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_namespace_change_state(&ns, FSC_NAMESPACE_STATE_ACTIVE);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fstable_init(&table, 8);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(0, fstable_count(&table));

    err = fstable_insert(&table, &ns);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1, fstable_count(&table));
    TEST_ASSERT_TRUE(fstable_lookup_fsid(&table, fsid) == &ns);
    TEST_ASSERT_TRUE(fstable_lookup_name(&table, "alpha") == &ns);
    TEST_ASSERT_TRUE(fstable_exists_fsid(&table, fsid));
    TEST_ASSERT_TRUE(fstable_exists_name(&table, "alpha"));

    err = fstable_insert(&table, &ns);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EEXIST, fs_err_errno(err));

    err = fstable_remove(&table, fsid, &removed);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(removed == &ns);
    TEST_ASSERT_EQ_INT(0, fstable_count(&table));
    TEST_ASSERT_FALSE(fstable_exists_name(&table, "alpha"));

    fstable_deinit(&table, NULL);
    return 0;
}


static int test_fstable_rejects_invalid_inputs(void)
{
    fs_error_t err;
    fsc_table_t table;
    fsc_fsid_t fsid = ((fsc_fsid_t)1 << 32) | 4U;

    err = fstable_init(NULL, 8);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fstable_init(&table, 8);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fstable_insert(&table, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fstable_remove(&table, fsid, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));

    TEST_ASSERT_TRUE(fstable_lookup_fsid(NULL, fsid) == NULL);
    TEST_ASSERT_TRUE(fstable_lookup_name(&table, "") == NULL);
    TEST_ASSERT_EQ_INT(0, fstable_count(NULL));

    fstable_deinit(&table, NULL);
    return 0;
}


static int test_fstable_reclaims_remaining_entries_on_deinit(void)
{
    fs_error_t err;
    fsc_table_t table;
    fsc_namespace_t ns_a;
    fsc_namespace_t ns_b;
    fsc_fsid_t fsid_a = ((fsc_fsid_t)1 << 32) | 13U;
    fsc_fsid_t fsid_b = ((fsc_fsid_t)1 << 32) | 14U;
    fuid_t root_a = test_fsc_make_root_fuid(fsid_a);
    fuid_t root_b = test_fsc_make_root_fuid(fsid_b);
    obj_handle_t handle = test_fsc_make_root_handle();

    err = fstable_init(&table, 4);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_namespace_init(&ns_a, fsid_a, "keep-a", &root_a, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_namespace_change_state(&ns_a, FSC_NAMESPACE_STATE_ACTIVE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_namespace_init(&ns_b, fsid_b, "keep-b", &root_b, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_namespace_change_state(&ns_b, FSC_NAMESPACE_STATE_ACTIVE);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fstable_insert(&table, &ns_a);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fstable_insert(&table, &ns_b);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fstable_remove(NULL, fsid_a, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fstable_remove(&table, fsid_a, NULL);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_FALSE(fstable_exists_fsid(&table, fsid_a));
    TEST_ASSERT_TRUE(fstable_exists_fsid(&table, fsid_b));

    g_fstable_reclaim_count = 0U;
    fstable_deinit(&table, test_fstable_reclaim_count);
    TEST_ASSERT_EQ_INT(1U, g_fstable_reclaim_count);
    fstable_deinit(NULL, test_fstable_reclaim_count);
    return 0;
}


static int test_fstable_rejects_zero_bucket_and_invalid_namespace(void)
{
    fs_error_t err;
    fsc_table_t table;
    fsc_namespace_t invalid_ns;

    err = fstable_init(&table, 0);
    TEST_ASSERT_EQ_INT(FS_MODULE_COMMON, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fstable_init(&table, 4);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    memset(&invalid_ns, 0, sizeof(invalid_ns));
    err = fstable_insert(&table, &invalid_ns);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    fstable_deinit(&table, NULL);
    return 0;
}


const test_case_t FSC_FSTABLE_CASES[] = {
        TEST_CASE(
                UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSTABLE, 0x1),
                UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSTABLE, 0x1, 0x001),
                test_fstable_insert_lookup_remove_by_two_indexes,
                "FSTable 双索引", "插入 ACTIVE namespace 并通过 fsid/name 查找",
                "双索引命中，重复插入 EEXIST，删除后计数归零"),
        TEST_CASE(
                UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSTABLE, 0x1),
                UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSTABLE, 0x1, 0x002),
                test_fstable_rejects_invalid_inputs, "FSTable 参数校验",
                "NULL table/ns、删除缺失 fsid、非法 name",
                "返回 FSC EINVAL/ENOENT 或安全 NULL"),
        TEST_CASE(
                UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSTABLE, 0x1),
                UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSTABLE, 0x1, 0x003),
                test_fstable_reclaims_remaining_entries_on_deinit,
                "FSTable deinit 回收剩余 entry",
                "插入两个 namespace，删除一个后 deinit 表并传入回收回调",
                "剩余 entry 被回收一次，NULL table deinit 安全返回"),
        TEST_CASE(
                UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSTABLE, 0x1),
                UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSTABLE, 0x1, 0x004),
                test_fstable_rejects_zero_bucket_and_invalid_namespace,
                "FSTable 初始化和 namespace 校验",
                "使用 0 bucket 初始化，并插入未初始化 namespace",
                "非法 hash 参数和非法 namespace 都被拒绝"),
};

const size_t FSC_FSTABLE_CASE_COUNT =
        sizeof(FSC_FSTABLE_CASES) / sizeof(FSC_FSTABLE_CASES[0]);
