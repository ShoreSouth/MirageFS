#include "framework/test_framework.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "runtime/internal/runtime_internal.h"
#include "runtime/internal/runtime_sub.h"
#include "runtime/include/runtime.h"


typedef enum test_runtime_component {
    TEST_RUNTIME_COMPONENT_LIFECYCLE = 0x01,
    TEST_RUNTIME_COMPONENT_SESSION = 0x02,
    TEST_RUNTIME_COMPONENT_GETTER = 0x03,
    TEST_RUNTIME_COMPONENT_FLOW = 0x04,
    TEST_RUNTIME_COMPONENT_SUB = 0x05,
} test_runtime_component_t;

static void test_runtime_cleanup_root(void)
{
    int rc;

    rc = system("rm -rf -- './miragefs.root'");
    (void)rc;
}

static int test_runtime_initial_state_is_not_initialized(void)
{
    runtime_deinit();

    TEST_ASSERT_FALSE(runtime_is_initialized());
    TEST_ASSERT_FALSE(runtime_fs_is_active());
    TEST_ASSERT_TRUE(runtime_fs_current() == NULL);
    return 0;
}

static int test_runtime_sub_names_cover_valid_and_invalid_values(void)
{
    TEST_ASSERT_STR_EQ("NONE", runtime_sub_name(RUNTIME_SUB_NONE));
    TEST_ASSERT_STR_EQ("INIT", runtime_sub_name(RUNTIME_SUB_INIT));
    TEST_ASSERT_STR_EQ("SESSION", runtime_sub_name(RUNTIME_SUB_SESSION));
    TEST_ASSERT_STR_EQ("NAMESPACE", runtime_sub_name(RUNTIME_SUB_NAMESPACE));
    TEST_ASSERT_STR_EQ("CTX", runtime_sub_name(RUNTIME_SUB_CTX));
    TEST_ASSERT_STR_EQ("PATH", runtime_sub_name(RUNTIME_SUB_PATH));
    TEST_ASSERT_STR_EQ("OP", runtime_sub_name(RUNTIME_SUB_OP));
    TEST_ASSERT_STR_EQ("HANDLE", runtime_sub_name(RUNTIME_SUB_HANDLE));
    TEST_ASSERT_STR_EQ("UNKNOWN", runtime_sub_name(RUNTIME_SUB_MAX));
    TEST_ASSERT_TRUE(runtime_sub_valid(RUNTIME_SUB_HANDLE));
    TEST_ASSERT_FALSE(runtime_sub_valid(RUNTIME_SUB_MAX));
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

static int test_runtime_initialized_without_session_rejects_ctx_ops(void)
{
    fops_create_attr_t create_attr;
    fops_object_result_t object_out;
    fops_dirent_t entries[2];
    fops_dirent_plus_t plus_entries[2];
    fops_attr_t attr;
    fops_file_t *file;
    namei_ctx_t ctx;
    fuid_t fuid;
    namei_parent_result_t parent;
    char cwd[8];
    char link_buf[8];
    size_t actual;
    uint32_t entry_nr;
    bool eof;
    fs_error_t err;

    runtime_deinit();
    test_runtime_cleanup_root();
    err = runtime_init(NULL);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_get_ctx(&ctx);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_get_root(&fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_get_cwd(&fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_getcwd(cwd, sizeof(cwd));
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_lookup("/missing", FS_FLAG_NONE, &fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_lookup_plus("/missing", FS_FLAG_NONE, &object_out);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_lookup_parent("/missing", FS_FLAG_NONE, &parent);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));

    memset(&create_attr, 0, sizeof(create_attr));
    create_attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    create_attr.mode = 0644;
    err = runtime_create("/missing", &create_attr, FS_FLAG_NONE, &object_out);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_mkdir("/missing", NULL, FS_FLAG_NONE, &object_out);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_unlink("/missing", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_rmdir("/missing", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_rename("/old", "/new", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_symlink("target", "/link", FS_FLAG_NONE, &object_out);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_readlink("/link", FS_FLAG_NONE, link_buf, sizeof(link_buf),
                           &actual);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_readdir("/missing", FS_FLAG_NONE, entries, 2U, &entry_nr,
                          &eof);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_readdirplus("/missing", FS_FLAG_NONE, plus_entries, 2U,
                              &entry_nr, &eof);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    err = runtime_getattr("/missing", FS_FLAG_NONE, &attr);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));
    file = NULL;
    err = runtime_open("/missing", FS_FLAG_READ, &file);
    TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module(err));
    TEST_ASSERT_EQ_INT(RUNTIME_SUB_SESSION, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));

    runtime_deinit();
    test_runtime_cleanup_root();
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


#define TEST_RUNTIME_EXPECT_ERRNO(err, expected_errno) \
    do { \
        TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module((err))); \
        TEST_ASSERT_EQ_INT((expected_errno), fs_err_errno((err))); \
    } while (0)

static int test_runtime_namespace_file_flow_round_trip(void)
{
    runtime_config_t cfg;
    fops_create_attr_t create_attr;
    fops_setattr_t setattr;
    fops_object_result_t out;
    fops_object_result_t plus;
    fops_file_t *file;
    fops_dirent_t entries[8];
    fops_dirent_plus_t plus_entries[8];
    fops_attr_t attr;
    fops_statfs_t statfs_buf;
    obj_handle_t handle;
    fuid_t root;
    fuid_t cwd;
    fuid_t looked_up;
    namei_parent_result_t parent;
    namei_ctx_t ctx;
    char namespace_name[32];
    char cwd_buf[32];
    char read_buf[16];
    char link_buf[64];
    char xattr_buf[64];
    char xattr_list[128];
    size_t actual;
    uint32_t entry_nr;
    bool eof;
    fs_error_t err;

    runtime_deinit();
    test_runtime_cleanup_root();

    memset(&cfg, 0, sizeof(cfg));
    (void)snprintf(namespace_name,
                   sizeof(namespace_name),
                   "rt_%ld",
                   (long)getpid());
    cfg.default_namespace = namespace_name;
    cfg.auto_create = true;
    cfg.auto_use = true;

    err = runtime_init(&cfg);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(runtime_is_initialized());
    TEST_ASSERT_TRUE(runtime_fs_is_active());
    TEST_ASSERT_STR_EQ(runtime_fs_current(), namespace_name);

    err = runtime_getcwd(cwd_buf, sizeof(cwd_buf));
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(cwd_buf, "/");
    err = runtime_getcwd(cwd_buf, 1U);
    TEST_RUNTIME_EXPECT_ERRNO(err, ENAMETOOLONG);
    err = runtime_get_root(&root);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_get_cwd(&cwd);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_equal(&root, &cwd));
    err = runtime_get_ctx(&ctx);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_fs_create(namespace_name, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EEXIST, fs_err_errno(err));
    err = runtime_fs_use("missing");
    TEST_RUNTIME_EXPECT_ERRNO(err, ENOENT);

    memset(&create_attr, 0, sizeof(create_attr));
    create_attr.valid_mask = FOPS_CREATE_ATTR_MODE | FOPS_CREATE_ATTR_SIZE;
    create_attr.mode = 0600;
    create_attr.size = 0U;

    /*
     * 先建立目录和普通文件，后续所有 NAMEI/RUNTIME 操作都走真实路径。
     */
    err = runtime_mkdir("/dir", NULL, FS_FLAG_DIRECTORY, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_dir(&out.fuid));
    err = runtime_create("/dir/file.txt",
                         &create_attr,
                         FS_FLAG_EXCLUSIVE | FS_FLAG_REGULAR,
                         &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_file(&out.fuid));

    file = NULL;
    err = runtime_open("/dir/file.txt",
                       FS_FLAG_READ | FS_FLAG_WRITE,
                       &file);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(file != NULL);
    err = runtime_write(file, "hello", 5U, &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(5U, actual);
    err = runtime_close(file);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    file = NULL;
    err = runtime_open("/dir/file.txt", FS_FLAG_READ, &file);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    memset(read_buf, 0, sizeof(read_buf));
    err = runtime_read(file, read_buf, 5U, &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(5U, actual);
    TEST_ASSERT_STR_EQ(read_buf, "hello");
    err = runtime_close(file);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_lookup("/dir/file.txt", FS_FLAG_REGULAR, &looked_up);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_file(&looked_up));
    err = runtime_lookup_plus("/dir/file.txt", FS_FLAG_REGULAR, &plus);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(FS_TYPE_REG, plus.attr.type);
    err = runtime_lookup_parent("/dir/file.txt", FS_FLAG_NONE, &parent);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(parent.name, "file.txt");

    err = runtime_getattr("/dir/file.txt", FS_FLAG_REGULAR, &attr);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(FS_TYPE_REG, attr.type);
    TEST_ASSERT_EQ_INT(5U, attr.size);

    memset(&setattr, 0, sizeof(setattr));
    setattr.valid_mask = FOPS_SETATTR_SIZE;
    setattr.size = 3U;
    err = runtime_setattr("/dir/file.txt", &setattr, FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_truncate("/dir/file.txt", 5U, FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_access("/dir/file.txt", F_OK, FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    /*
     * 符号链接同时覆盖 readlink 和 namei_walk 的 final NOFOLLOW 分支。
     */
    err = runtime_symlink("file.txt", "/dir/link.txt", FS_FLAG_EXCLUSIVE,
                          &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    memset(link_buf, 0, sizeof(link_buf));
    err = runtime_readlink("/dir/link.txt", FS_FLAG_NONE, link_buf,
                           sizeof(link_buf), &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(link_buf, "file.txt");
    err = runtime_lookup("/dir/link.txt", FS_FLAG_NOFOLLOW, &looked_up);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_symlink(&looked_up));

    err = runtime_readdir("/dir", FS_FLAG_DIRECTORY, entries, 8U,
                          &entry_nr, &eof);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(entry_nr >= 2U);
    err = runtime_readdirplus("/dir", FS_FLAG_DIRECTORY, plus_entries, 8U,
                              &entry_nr, &eof);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(entry_nr >= 2U);

    err = runtime_setxattr("/dir/file.txt", "user.rt", "v", 1U,
                           FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    memset(xattr_buf, 0, sizeof(xattr_buf));
    err = runtime_getxattr("/dir/file.txt", "user.rt", xattr_buf,
                           sizeof(xattr_buf), &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1U, actual);
    err = runtime_listxattr("/dir/file.txt", xattr_list,
                            sizeof(xattr_list), &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(actual > 0U);
    err = runtime_removexattr("/dir/file.txt", "user.rt");
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_statfs("/dir/file.txt", &statfs_buf);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_syncfs("/dir/file.txt");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_gethandle("/dir/file.txt", &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    file = NULL;
    err = runtime_openhandle(&handle, FS_FLAG_READ, &file);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_close(file);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_link("/dir/file.txt", "/dir/hard.txt",
                       FS_FLAG_EXCLUSIVE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_rename("/dir/hard.txt", "/dir/renamed.txt",
                         FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_mknod("/dir/fifo", FS_TYPE_FIFO, NULL, NULL,
                        FS_FLAG_EXCLUSIVE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_chdir("/dir");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_getcwd(cwd_buf, sizeof(cwd_buf));
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(cwd_buf, "/dir");
    err = runtime_chdir(".");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_getcwd(cwd_buf, sizeof(cwd_buf));
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(cwd_buf, "/dir");
    err = runtime_chdir("/");
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_unlink("/dir", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EISDIR, fs_err_errno(err));
    err = runtime_unlink("/dir/renamed.txt", FS_FLAG_REGULAR);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_unlink("/dir/link.txt", FS_FLAG_NOFOLLOW);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_unlink("/dir/fifo", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_unlink("/dir/file.txt", FS_FLAG_REGULAR);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_rmdir("/dir", FS_FLAG_DIRECTORY);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_fs_destroy(namespace_name);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_FALSE(runtime_fs_is_active());
    runtime_deinit();
    test_runtime_cleanup_root();
    return 0;
}


static const test_case_t TEST_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_RUNTIME,
                         TEST_RUNTIME_COMPONENT_SUB,
                         0x1),
              UT_CASE_NO(UT_MOD_RUNTIME,
                         TEST_RUNTIME_COMPONENT_SUB,
                         0x1,
                         0x001),
              test_runtime_sub_names_cover_valid_and_invalid_values,
              "Runtime sub-error 名称表",
              "遍历 Runtime 子错误枚举和非法边界",
              "合法枚举返回名称，非法值返回 UNKNOWN 且 valid=false"),
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
                         TEST_RUNTIME_COMPONENT_SESSION,
                         0x1),
              UT_CASE_NO(UT_MOD_RUNTIME,
                         TEST_RUNTIME_COMPONENT_SESSION,
                         0x1,
                         0x002),
              test_runtime_initialized_without_session_rejects_ctx_ops,
              "Runtime 未进入 namespace 保护",
              "已 init 但未 fs use 时读取 ctx/root/cwd",
              "返回 RUNTIME/SESSION/ENOENT"),
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

    TEST_CASE(UT_LIST_NO(UT_MOD_RUNTIME,
                         TEST_RUNTIME_COMPONENT_FLOW,
                         0x1),
              UT_CASE_NO(UT_MOD_RUNTIME,
                         TEST_RUNTIME_COMPONENT_FLOW,
                         0x1,
                         0x001),
              test_runtime_namespace_file_flow_round_trip,
              "Runtime 真实会话文件系统回环",
              "自动创建/进入 namespace 后执行路径解析、读写、链接、目录和 xattr",
              "Runtime/NAMEI/FOPS 主链路保持一致，清理后 namespace 可销毁"),
};

int main(void)
{
    return test_run_suite("runtime", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
