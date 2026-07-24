#include "fsc_test_common.h"

static int test_fsmgr_create_lookup_destroy_real_namespace(void)
{
    fs_error_t err;
    fuid_t root_fuid;
    fuid_t got_fuid;
    obj_handle_t got_handle;
    fsc_namespace_t *ns;
    FILE *child_file;
    const char *path = "../output/tests/fsc/sysroot";

    test_fsc_prepare_temp_dir();
    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_sysroot_init(path);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsmgr_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    /* create 需要同时返回 root FUID，因此 name/out 任一为空都应早退。 */
    err = fsmgr_create(NULL, &root_fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsmgr_create("alpha", NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fsmgr_create("alpha", &root_fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_dir(&root_fuid));
    TEST_ASSERT_EQ_INT(1, fsmgr_count());
    TEST_ASSERT_TRUE(fsmgr_exists("alpha"));

    ns = fsmgr_lookup("alpha");
    TEST_ASSERT_TRUE(ns != NULL);
    TEST_ASSERT_TRUE(fsmgr_lookup_fsid(root_fuid.fsid) == ns);
    TEST_ASSERT_TRUE(fsmgr_lookup("missing") == NULL);
    /* DELETING namespace 不再对外可见，getter 也要按缺失对象处理。 */
    ns->state = FSC_NAMESPACE_STATE_DELETING;
    TEST_ASSERT_TRUE(fsmgr_lookup("alpha") == NULL);
    TEST_ASSERT_TRUE(fsmgr_lookup_fsid(root_fuid.fsid) == NULL);
    err = fsmgr_get_root_handle(root_fuid.fsid, &got_handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    ns->state = FSC_NAMESPACE_STATE_ACTIVE;

    err = fsmgr_get_root_fuid(root_fuid.fsid, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsmgr_get_root_fuid(root_fuid.fsid, &got_fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_equal(&got_fuid, &root_fuid));

    err = fsmgr_get_root_handle(root_fuid.fsid, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = fsmgr_get_root_handle(root_fuid.fsid, &got_handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(got_handle.len > 0U);

    err = fsmgr_create("alpha", &got_fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EEXIST, fs_err_errno(err));

    /*
     * 直接抬高 namespace refcnt，验证 destroy 会先做 busy 防护，
     * 不会提前删除后端目录或污染 manager 状态。
     */
    fs_atomic32_inc(&ns->refcnt);
    err = fsmgr_destroy(root_fuid.fsid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EBUSY, fs_err_errno(err));
    fs_atomic32_dec(&ns->refcnt);

    /* 后端目录非空时 destroy 失败，manager 里的 namespace 不能被提前移除。 */
    child_file = fopen("../output/tests/fsc/sysroot/alpha/held.txt", "w");
    TEST_ASSERT_TRUE(child_file != NULL);
    TEST_ASSERT_EQ_INT(fclose(child_file), 0);
    err = fsmgr_destroy(root_fuid.fsid);
    TEST_ASSERT_EQ_INT(FS_MODULE_LSA, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOTEMPTY, fs_err_errno(err));
    TEST_ASSERT_EQ_INT(1, fsmgr_count());
    TEST_ASSERT_TRUE(fsmgr_exists("alpha"));
    TEST_ASSERT_EQ_INT(unlink("../output/tests/fsc/sysroot/alpha/held.txt"), 0);

    err = fsmgr_destroy(root_fuid.fsid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(0, fsmgr_count());
    TEST_ASSERT_FALSE(fsmgr_exists("alpha"));

    err = fsmgr_destroy(root_fuid.fsid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = fsmgr_get_root_fuid(root_fuid.fsid, &got_fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));

    fsmgr_deinit();
    nspool_deinit();
    fsid_deinit();
    fsc_sysroot_deinit();
    object_deinit();
    test_fsc_cleanup_sysroot(path);
    return 0;
}


static int test_fsmgr_deinit_reclaims_registered_namespace(void)
{
    fs_error_t err;
    fuid_t root_fuid;
    const char *path = "../output/tests/fsc/sysroot";

    test_fsc_prepare_temp_dir();
    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_sysroot_init(path);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsmgr_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    /* 不显式 destroy 时，fsmgr_deinit 负责回收表内注册的 namespace。 */
    err = fsmgr_create("beta", &root_fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1, fsmgr_count());
    fsmgr_deinit();
    TEST_ASSERT_EQ_INT(0, fsmgr_count());

    nspool_deinit();
    fsid_deinit();
    fsc_sysroot_deinit();
    object_deinit();
    (void)rmdir("../output/tests/fsc/sysroot/beta");
    test_fsc_cleanup_sysroot(path);
    return 0;
}


static int test_fsmgr_create_rolls_back_when_nspool_exhausted(void)
{
    enum
    {
        TEST_NSPOOL_ALLOC_CAP = 1024
    };
    fs_error_t err;
    fuid_t root_fuid;
    fsc_namespace_t *held[TEST_NSPOOL_ALLOC_CAP];
    const char *path = "../output/tests/fsc/sysroot";
    fsc_namespace_t *extra;
    size_t held_nr;
    size_t i;

    test_fsc_prepare_temp_dir();
    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_sysroot_init(path);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsmgr_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    held_nr = 0U;
    while (held_nr < TEST_NSPOOL_ALLOC_CAP)
    {
        extra = nspool_alloc();
        if (extra == NULL)
        {
            break;
        }
        held[held_nr] = extra;
        held_nr++;
    }
    TEST_ASSERT_TRUE(held_nr > 0U);
    TEST_ASSERT_TRUE(nspool_alloc() == NULL);

    /* create 中途拿不到 namespace 时，目录、FSID 和 object key 都要回滚。 */
    err = fsmgr_create("pool_full", &root_fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOMEM, fs_err_errno(err));
    TEST_ASSERT_EQ_INT(0, fsmgr_count());
    TEST_ASSERT_FALSE(fsmgr_exists("pool_full"));

    for (i = 0; i < held_nr; i++)
    {
        nspool_free(held[i]);
    }

    fsmgr_deinit();
    nspool_deinit();
    fsid_deinit();
    fsc_sysroot_deinit();
    object_deinit();
    test_fsc_cleanup_sysroot(path);
    return 0;
}

static int test_fsmgr_list_and_rename_namespace(void)
{
    fs_error_t err;
    fuid_t root_fuid;
    fsc_namespace_t *ns;
    char names[4][FSC_NAMESPACE_NAME_MAX];
    uint32_t actual;
    const char *path = "../output/tests/fsc/sysroot";

    test_fsc_prepare_temp_dir();
    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_sysroot_init(path);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsmgr_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsmgr_create("tree", &root_fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsmgr_create("other", &root_fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    memset(names, 0, sizeof(names));
    actual = 0U;
    err = fsmgr_list(names, 4U, &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(2U, actual);

    err = fsmgr_rename("tree", "renamed");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_FALSE(fsmgr_exists("tree"));
    TEST_ASSERT_TRUE(fsmgr_exists("renamed"));
    TEST_ASSERT_EQ_INT(access("../output/tests/fsc/sysroot/tree", F_OK), -1);
    TEST_ASSERT_EQ_INT(access("../output/tests/fsc/sysroot/renamed", F_OK), 0);

    err = fsmgr_rename("renamed", "other");
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EEXIST, fs_err_errno(err));
    err = fsmgr_rename("missing", "newname");
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));

    ns = fsmgr_lookup("renamed");
    TEST_ASSERT_TRUE(ns != NULL);
    TEST_ASSERT_EQ_INT(mkdir("../output/tests/fsc/sysroot/renamed/sub", 0755),
                       0);
    err = fsmgr_destroy(ns->fsid);
    TEST_ASSERT_EQ_INT(FS_MODULE_LSA, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOTEMPTY, fs_err_errno(err));
    TEST_ASSERT_TRUE(fsmgr_exists("renamed"));
    TEST_ASSERT_EQ_INT(rmdir("../output/tests/fsc/sysroot/renamed/sub"), 0);
    err = fsmgr_destroy(ns->fsid);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    ns = fsmgr_lookup("other");
    TEST_ASSERT_TRUE(ns != NULL);
    err = fsmgr_destroy(ns->fsid);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    fsmgr_deinit();
    nspool_deinit();
    fsid_deinit();
    fsc_sysroot_deinit();
    object_deinit();
    test_fsc_cleanup_sysroot(path);
    return 0;
}

static int test_fsmgr_recover_imports_existing_roots(void)
{
    fs_error_t err;
    char names[4][FSC_NAMESPACE_NAME_MAX];
    uint32_t actual;
    const char *path = "../output/tests/fsc/sysroot";

    test_fsc_prepare_temp_dir();
    if (mkdir(path, 0755) != 0)
    {
        TEST_ASSERT_EQ_INT(errno, EEXIST);
    }
    if (mkdir("../output/tests/fsc/sysroot/persisted", 0755) != 0)
    {
        TEST_ASSERT_EQ_INT(errno, EEXIST);
    }

    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsc_sysroot_init(path);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsmgr_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fsmgr_recover();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    TEST_ASSERT_TRUE(fsmgr_exists("persisted"));
    memset(names, 0, sizeof(names));
    actual = 0U;
    err = fsmgr_list(names, 4U, &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1U, actual);

    err = fsmgr_destroy(fsmgr_lookup("persisted")->fsid);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    fsmgr_deinit();
    nspool_deinit();
    fsid_deinit();
    fsc_sysroot_deinit();
    object_deinit();
    test_fsc_cleanup_sysroot(path);
    return 0;
}


const test_case_t FSC_FSMGR_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1, 0x001),
                  test_fsmgr_create_lookup_destroy_real_namespace,
                  "FSMgr 真实 namespace 回环",
                  "在真实 sysroot 下 "
                  "create/lookup/getter/destroy，并注入重复和 busy",
                  "命名空间生命周期完整，重复创建 EEXIST，busy 删除 EBUSY"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1, 0x002),
                  test_fsmgr_deinit_reclaims_registered_namespace,
                  "FSMgr deinit 回收注册 namespace",
                  "创建 namespace 后不显式 destroy，直接 deinit manager",
                  "manager 释放表内 namespace 并把计数归零"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1, 0x003),
                  test_fsmgr_create_rolls_back_when_nspool_exhausted,
                  "FSMgr create 回滚", "提前耗尽 NSPool 后创建 namespace",
                  "创建失败返回 ENOMEM，目录、FSID 和对象 key 被回滚"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1, 0x004),
                  test_fsmgr_list_and_rename_namespace,
                  "FSMgr list/rename namespace",
                  "创建多个 namespace 后列出、重命名并验证非空 destroy 防护",
                  "索引和后端目录同步更新，普通 destroy 仍拒绝非空目录"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1),
                  UT_CASE_NO(UT_MOD_FSC, TEST_FSC_COMPONENT_FSMGR, 0x1, 0x005),
                  test_fsmgr_recover_imports_existing_roots,
                  "FSMgr recover 导入已有根目录",
                  "sysroot 中预先存在 namespace 根目录后执行 recover",
                  "已有根目录被注册为可见 namespace"),
};

const size_t FSC_FSMGR_CASE_COUNT =
        sizeof(FSC_FSMGR_CASES) / sizeof(FSC_FSMGR_CASES[0]);
