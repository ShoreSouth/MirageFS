#include "framework/test_framework.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "common/fs_common.h"
#include "fsc/fsc_error.h"
#include "fsc/fsc_sub.h"
#include "fsc/fsid/fsid.h"
#include "fsc/namespace/namespace.h"
#include "fsc/fstable/fstable.h"

static obj_handle_t make_root_handle(void)
{
    obj_handle_t handle;

    memset(&handle, 0, sizeof(handle));
    handle.mount_id = 9;
    handle.type = 1;
    handle.len = 4;
    handle.data[0] = 1;
    handle.data[1] = 2;
    handle.data[2] = 3;
    handle.data[3] = 4;
    return handle;
}

static fuid_t make_root_fuid(fsc_fsid_t fsid)
{
    return fuid_make(fsid, 1, 1, FUID_TYPE_DIR);
}

static int test_fsc_error_encodes_module_sub_errno(void)
{
    fs_error_t err = fsc_error(FSC_SUB_FSID, EINVAL);

    TEST_ASSERT_EQ_INT(FS_SEV_ERROR, fs_err_severity(err));
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(FSC_SUB_FSID, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    TEST_ASSERT_TRUE(fsc_sub_valid(FSC_SUB_FSID));
    TEST_ASSERT_STR_EQ("FSID", fsc_sub_name(FSC_SUB_FSID));
    return 0;
}

static int test_fsid_alloc_free_and_stale_guard(void)
{
    fs_error_t err;
    fsc_fsid_t fsid = FSID_INVALID;

    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsid_alloc(&fsid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fsid_is_valid(fsid));
    TEST_ASSERT_TRUE(fsid_hash(fsid) != 0);

    err = fsid_free(fsid);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsid_free(fsid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    fsid_deinit();
    return 0;
}

static int test_fsid_rejects_null_output(void)
{
    fs_error_t err;

    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsid_alloc(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    fsid_deinit();
    return 0;
}

static int test_namespace_init_state_and_deinit(void)
{
    fs_error_t err;
    fsc_namespace_t ns;
    fsc_fsid_t fsid = ((fsc_fsid_t)1 << 32) | 1U;
    fuid_t root = make_root_fuid(fsid);
    obj_handle_t handle = make_root_handle();

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

    fsc_namespace_deinit(&ns);
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
    obj_handle_t handle = make_root_handle();

    err = fsc_namespace_init(&ns, fsid, "bad", &wrong_type, &handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fsc_namespace_init(&ns, fsid, "bad", &wrong_fsid, &handle);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static int test_fstable_insert_lookup_remove_by_two_indexes(void)
{
    fs_error_t err;
    fsc_table_t table;
    fsc_namespace_t ns;
    fsc_namespace_t *removed = NULL;
    fsc_fsid_t fsid = ((fsc_fsid_t)1 << 32) | 3U;
    fuid_t root = make_root_fuid(fsid);
    obj_handle_t handle = make_root_handle();

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

static const test_case_t TEST_CASES[] = {
    TEST_CASE(test_fsc_error_encodes_module_sub_errno, "FSC 错误码布局", "构造 FSID 子模块错误", "severity/module/sub/errno 字段可正确解析"),
    TEST_CASE(test_fsid_alloc_free_and_stale_guard, "FSID 分配释放", "分配后释放，并重复释放旧 FSID", "第一次成功，重复释放被识别为非法/stale"),
    TEST_CASE(test_fsid_rejects_null_output, "FSID 参数校验", "fsid_alloc 传入 NULL 输出参数", "返回 FSC 模块 EINVAL"),
    TEST_CASE(test_namespace_init_state_and_deinit, "Namespace 初始化和状态机", "初始化后执行 INIT->ACTIVE，再尝试回退", "合法迁移成功，非法迁移返回 EINVAL，deinit 后无效"),
    TEST_CASE(test_namespace_rejects_invalid_root_fuid, "Namespace root 校验", "注入非目录 root 和 fsid 不匹配 root", "返回 FSC/NAMESPACE/EINVAL"),
    TEST_CASE(test_fstable_insert_lookup_remove_by_two_indexes, "FSTable 双索引", "插入 ACTIVE namespace 并通过 fsid/name 查找", "双索引命中，重复插入 EEXIST，删除后计数归零"),
    TEST_CASE(test_fstable_rejects_invalid_inputs, "FSTable 参数校验", "NULL table/ns、删除缺失 fsid、非法 name", "返回 FSC EINVAL/ENOENT 或安全 NULL"),
};

int main(void)
{
    return test_run_suite("fsc", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}