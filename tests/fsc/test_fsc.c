#include "framework/test_framework.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "fsc/fsc_error.h"
#include "fsc/fsc_init.h"
#include "fsc/fsmgr/fsmgr.h"
#include "fsc/fsc_sub.h"
#include "fsc/fsid/fsid.h"
#include "fsc/namespace/namespace.h"
#include "fsc/fstable/fstable.h"
#include "fsc/nspool/nspool.h"
#include "fsc/sysroot/sysroot.h"
#include "object/object_init.h"


typedef enum test_fsc_component {
    TEST_FSC_COMPONENT_ERROR = 0x01,
    TEST_FSC_COMPONENT_FSID = 0x02,
    TEST_FSC_COMPONENT_NAMESPACE = 0x03,
    TEST_FSC_COMPONENT_FSTABLE = 0x04,
    TEST_FSC_COMPONENT_NSPOOL = 0x05,
    TEST_FSC_COMPONENT_SYSROOT = 0x06,
    TEST_FSC_COMPONENT_FSMGR = 0x07,
    TEST_FSC_COMPONENT_INIT = 0x08,
} test_fsc_component_t;

static uint32_t g_fstable_reclaim_count;

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

static void test_fstable_reclaim_count(fsc_namespace_t *ns)
{
    if (ns != NULL) {
        g_fstable_reclaim_count++;
    }
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
    TEST_ASSERT_FALSE(fsc_sub_valid(FSC_SUB_MAX));
    TEST_ASSERT_STR_EQ("UNKNOWN", fsc_sub_name(FSC_SUB_MAX));
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

static int test_fsid_reports_exhaustion_and_invalid_free(void)
{
    enum { TEST_FSID_MAX_ALLOC = 4096 };
    fsc_fsid_t fsids[TEST_FSID_MAX_ALLOC];
    fsc_fsid_t extra = FSID_INVALID;
    fs_error_t err;
    size_t i;

    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsid_free(FSID_INVALID);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    for (i = 0; i < TEST_FSID_MAX_ALLOC; i++) {
        err = fsid_alloc(&fsids[i]);
        TEST_ASSERT_EQ_INT(FS_OK, err);
    }

    err = fsid_alloc(&extra);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOSPC, fs_err_errno(err));

    for (i = 0; i < TEST_FSID_MAX_ALLOC; i++) {
        err = fsid_free(fsids[i]);
        TEST_ASSERT_EQ_INT(FS_OK, err);
    }
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
    obj_handle_t handle = make_root_handle();

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
    fuid_t root = make_root_fuid(fsid);
    obj_handle_t handle = make_root_handle();
    char long_name[FSC_NAMESPACE_NAME_MAX + 1U];

    memset(long_name, 'n', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = 0;

    TEST_ASSERT_FALSE(fsc_namespace_name_is_valid(long_name));
    TEST_ASSERT_EQ_INT(FSC_NAMESPACE_STATE_INVALID,
                       fsc_namespace_state(NULL));
    TEST_ASSERT_TRUE(fsc_namespace_state_can_transit(
                    FSC_NAMESPACE_STATE_INIT,
                    FSC_NAMESPACE_STATE_ACTIVE));
    TEST_ASSERT_TRUE(fsc_namespace_state_can_transit(
                    FSC_NAMESPACE_STATE_ACTIVE,
                    FSC_NAMESPACE_STATE_DELETING));
    TEST_ASSERT_FALSE(fsc_namespace_state_can_transit(
                    FSC_NAMESPACE_STATE_DELETING,
                    FSC_NAMESPACE_STATE_ACTIVE));

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

static int test_fstable_reclaims_remaining_entries_on_deinit(void)
{
    fs_error_t err;
    fsc_table_t table;
    fsc_namespace_t ns_a;
    fsc_namespace_t ns_b;
    fsc_fsid_t fsid_a = ((fsc_fsid_t)1 << 32) | 13U;
    fsc_fsid_t fsid_b = ((fsc_fsid_t)1 << 32) | 14U;
    fuid_t root_a = make_root_fuid(fsid_a);
    fuid_t root_b = make_root_fuid(fsid_b);
    obj_handle_t handle = make_root_handle();

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


static void test_fsc_prepare_temp_dir(void)
{
    (void)mkdir("../output", 0755);
    (void)mkdir("../output/tests", 0755);
    (void)mkdir("../output/tests/fsc", 0755);
}

static void test_fsc_cleanup_sysroot(const char *path)
{
    if (path != NULL) {
        (void)rmdir(path);
    }

    (void)rmdir("../output/tests/fsc");
    (void)rmdir("../output/tests");
    (void)rmdir("../output");
}

static int test_nspool_alloc_free_and_repeat_deinit(void)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    nspool_deinit();
    TEST_ASSERT_TRUE(nspool_alloc() == NULL);
    nspool_free(NULL);

    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    ns = nspool_alloc();
    TEST_ASSERT_TRUE(ns != NULL);

    /*
     * nspool_free() 会先 deinit namespace，再把内存交还给 mempool。
     * 这里故意传入一个未 init 的 namespace，验证清零对象也能安全释放。
     */
    nspool_free(ns);
    nspool_deinit();
    nspool_deinit();
    return 0;
}

static int test_nspool_reports_exhaustion(void)
{
    enum { TEST_NSPOOL_ALLOC_CAP = 1024 };
    fsc_namespace_t *allocated[TEST_NSPOOL_ALLOC_CAP];
    fsc_namespace_t *extra;
    fs_error_t err;
    size_t allocated_nr;
    size_t i;

    err = nspool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    allocated_nr = 0U;
    while (allocated_nr < TEST_NSPOOL_ALLOC_CAP) {
        extra = nspool_alloc();
        if (extra == NULL) {
            break;
        }
        allocated[allocated_nr] = extra;
        allocated_nr++;
    }
    TEST_ASSERT_TRUE(allocated_nr > 0U);
    extra = nspool_alloc();
    TEST_ASSERT_TRUE(extra == NULL);

    for (i = 0; i < allocated_nr; i++) {
        nspool_free(allocated[i]);
    }
    nspool_deinit();
    return 0;
}

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
    TEST_ASSERT_TRUE(snprintf(abs_path,
                              sizeof(abs_path),
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
    enum { TEST_NSPOOL_ALLOC_CAP = 1024 };
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
    while (held_nr < TEST_NSPOOL_ALLOC_CAP) {
        extra = nspool_alloc();
        if (extra == NULL) {
            break;
        }
        held[held_nr] = extra;
        held_nr++;
    }
    TEST_ASSERT_TRUE(held_nr > 0U);
    TEST_ASSERT_TRUE(nspool_alloc() == NULL);

    err = fsmgr_create("pool_full", &root_fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOMEM, fs_err_errno(err));
    TEST_ASSERT_EQ_INT(0, fsmgr_count());
    TEST_ASSERT_FALSE(fsmgr_exists("pool_full"));

    for (i = 0; i < held_nr; i++) {
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


static const test_case_t TEST_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_ERROR,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_ERROR,
                         0x1,
                         0x001),
              test_fsc_error_encodes_module_sub_errno,
              "FSC 错误码布局",
              "构造 FSID 子模块错误",
              "severity/module/sub/errno 字段可正确解析"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSID,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSID,
                         0x1,
                         0x001),
              test_fsid_alloc_free_and_stale_guard,
              "FSID 分配释放",
              "分配后释放，并重复释放旧 FSID",
              "第一次成功，重复释放被识别为非法/stale"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSID,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSID,
                         0x1,
                         0x002),
              test_fsid_rejects_null_output,
              "FSID 参数校验",
              "fsid_alloc 传入 NULL 输出参数",
              "返回 FSC 模块 EINVAL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSID,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSID,
                         0x1,
                         0x003),
              test_fsid_reports_exhaustion_and_invalid_free,
              "FSID 容量边界",
              "释放非法 FSID，并分配满所有 slot 后继续申请",
              "非法释放返回 EINVAL，容量耗尽返回 ENOSPC"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NAMESPACE,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NAMESPACE,
                         0x1,
                         0x001),
              test_namespace_init_state_and_deinit,
              "Namespace 初始化和状态机",
              "初始化后执行 INIT->ACTIVE，再尝试回退",
              "合法迁移成功，非法迁移返回 EINVAL，deinit 后无效"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NAMESPACE,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NAMESPACE,
                         0x1,
                         0x002),
              test_namespace_rejects_invalid_root_fuid,
              "Namespace root 校验",
              "注入非目录 root 和 fsid 不匹配 root",
              "返回 FSC/NAMESPACE/EINVAL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NAMESPACE,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NAMESPACE,
                         0x1,
                         0x003),
              test_namespace_rejects_invalid_inputs_and_deleting_edges,
              "Namespace 参数和删除态边界",
              "注入 NULL、非法 fsid、超长名称以及 ACTIVE->DELETING 状态",
              "非法输入返回 EINVAL，DELETING 仍是合法生命周期状态"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSTABLE,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSTABLE,
                         0x1,
                         0x001),
              test_fstable_insert_lookup_remove_by_two_indexes,
              "FSTable 双索引",
              "插入 ACTIVE namespace 并通过 fsid/name 查找",
              "双索引命中，重复插入 EEXIST，删除后计数归零"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSTABLE,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSTABLE,
                         0x1,
                         0x002),
              test_fstable_rejects_invalid_inputs,
              "FSTable 参数校验",
              "NULL table/ns、删除缺失 fsid、非法 name",
              "返回 FSC EINVAL/ENOENT 或安全 NULL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSTABLE,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSTABLE,
                         0x1,
                         0x003),
              test_fstable_reclaims_remaining_entries_on_deinit,
              "FSTable deinit 回收剩余 entry",
              "插入两个 namespace，删除一个后 deinit 表并传入回收回调",
              "剩余 entry 被回收一次，NULL table deinit 安全返回"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSTABLE,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSTABLE,
                         0x1,
                         0x004),
              test_fstable_rejects_zero_bucket_and_invalid_namespace,
              "FSTable 初始化和 namespace 校验",
              "使用 0 bucket 初始化，并插入未初始化 namespace",
              "非法 hash 参数和非法 namespace 都被拒绝"),

    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NSPOOL,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NSPOOL,
                         0x1,
                         0x001),
              test_nspool_alloc_free_and_repeat_deinit,
              "NSPool 独立生命周期",
              "未初始化 alloc、NULL free、初始化后 alloc/free 和重复 deinit",
              "未初始化申请返回 NULL，释放和重复销毁安全"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NSPOOL,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_NSPOOL,
                         0x1,
                         0x002),
              test_nspool_reports_exhaustion,
              "NSPool 容量耗尽",
              "连续申请完默认 namespace 池后再申请一次",
              "额外申请返回 NULL，已申请对象可全部释放"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_SYSROOT,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_SYSROOT,
                         0x1,
                         0x001),
              test_sysroot_custom_path_lifecycle_and_getters,
              "Sysroot 自定义路径生命周期",
              "使用 output/tests/fsc/sysroot 启动并注入未初始化/NULL getter",
              "启动后 active 且 FUID/handle 可读，非法 getter 返回 EINVAL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_SYSROOT,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_SYSROOT,
                         0x1,
                         0x002),
              test_sysroot_rejects_empty_and_accepts_absolute_path,
              "Sysroot 空路径和绝对路径",
              "先传入空路径，再传入当前目录拼出的绝对路径",
              "空路径返回 EINVAL，绝对路径按原样保存并可反初始化"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSMGR,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSMGR,
                         0x1,
                         0x001),
              test_fsmgr_create_lookup_destroy_real_namespace,
              "FSMgr 真实 namespace 回环",
              "在真实 sysroot 下 create/lookup/getter/destroy，并注入重复和 busy",
              "命名空间生命周期完整，重复创建 EEXIST，busy 删除 EBUSY"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSMGR,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSMGR,
                         0x1,
                         0x002),
              test_fsmgr_deinit_reclaims_registered_namespace,
              "FSMgr deinit 回收注册 namespace",
              "创建 namespace 后不显式 destroy，直接 deinit manager",
              "manager 释放表内 namespace 并把计数归零"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSMGR,
                         0x1),
              UT_CASE_NO(UT_MOD_FSC,
                         TEST_FSC_COMPONENT_FSMGR,
                         0x1,
                         0x003),
              test_fsmgr_create_rolls_back_when_nspool_exhausted,
              "FSMgr create 回滚",
              "提前耗尽 NSPool 后创建 namespace",
              "创建失败返回 ENOMEM，目录、FSID 和对象 key 被回滚"),
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

int main(void)
{
    return test_run_suite("fsc", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
