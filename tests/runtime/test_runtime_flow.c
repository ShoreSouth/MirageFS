#include "runtime_test_common.h"

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
    (void)snprintf(namespace_name, sizeof(namespace_name), "rt_%ld",
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
    err = runtime_create("/dir/file.txt", &create_attr,
                         FS_FLAG_EXCLUSIVE | FS_FLAG_REGULAR, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_file(&out.fuid));

    file = NULL;
    err = runtime_open("/dir/file.txt", FS_FLAG_READ | FS_FLAG_WRITE, &file);
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
    err = runtime_symlink("file.txt", "/dir/link.txt", FS_FLAG_EXCLUSIVE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    memset(link_buf, 0, sizeof(link_buf));
    err = runtime_readlink("/dir/link.txt", FS_FLAG_NONE, link_buf,
                           sizeof(link_buf), &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(link_buf, "file.txt");
    err = runtime_lookup("/dir/link.txt", FS_FLAG_NOFOLLOW, &looked_up);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_symlink(&looked_up));

    err = runtime_readdir("/dir", FS_FLAG_DIRECTORY, entries, 8U, &entry_nr,
                          &eof);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(entry_nr >= 2U);
    err = runtime_readdirplus("/dir", FS_FLAG_DIRECTORY, plus_entries, 8U,
                              &entry_nr, &eof);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(entry_nr >= 2U);

    err = runtime_setxattr("/dir/file.txt", "user.rt", "v", 1U, FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    memset(xattr_buf, 0, sizeof(xattr_buf));
    err = runtime_getxattr("/dir/file.txt", "user.rt", xattr_buf,
                           sizeof(xattr_buf), &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1U, actual);
    err = runtime_listxattr("/dir/file.txt", xattr_list, sizeof(xattr_list),
                            &actual);
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

    err = runtime_link("/dir/file.txt", "/dir/hard.txt", FS_FLAG_EXCLUSIVE,
                       &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_rename("/dir/hard.txt", "/dir/renamed.txt", FS_FLAG_NONE);
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

static int test_runtime_namespace_management_round_trip(void)
{
    fops_create_attr_t create_attr;
    fops_object_result_t out;
    char names[4][FSC_NAMESPACE_NAME_MAX];
    uint32_t actual;
    fs_error_t err;

    runtime_deinit();
    test_runtime_cleanup_root();

    err = runtime_init(NULL);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_fs_create("alpha", NULL);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_fs_create("beta", NULL);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    memset(names, 0, sizeof(names));
    actual = 0U;
    err = runtime_fs_list(names, 4U, &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(2U, actual);

    err = runtime_fs_rename("alpha", "renamed");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_fs_enter("renamed");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(runtime_fs_current(), "renamed");

    memset(&create_attr, 0, sizeof(create_attr));
    create_attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    create_attr.mode = FS_MODE_FILE_DEFAULT;
    err = runtime_mkdir("/dir", NULL, FS_FLAG_DIRECTORY, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_mkdir("/dir/sub", NULL, FS_FLAG_DIRECTORY, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_create("/dir/sub/file.txt", &create_attr, FS_FLAG_REGULAR,
                         &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_symlink("sub/file.txt", "/dir/link.txt", FS_FLAG_NONE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_fs_destroy("renamed");
    TEST_ASSERT_EQ_INT(FS_MODULE_LSA, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOTEMPTY, fs_err_errno(err));

    err = runtime_fs_destroy_tree("renamed");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_FALSE(runtime_fs_is_active());
    TEST_ASSERT_EQ_INT(access("./miragefs.root/renamed", F_OK), -1);

    err = runtime_fs_destroy_tree("beta");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(0U, fsmgr_count());
    TEST_ASSERT_EQ_INT(0U, objmgr_count());

    runtime_deinit();
    test_runtime_cleanup_root();
    return 0;
}


const test_case_t RUNTIME_FLOW_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_FLOW, 0x1),
                  UT_CASE_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_FLOW, 0x1,
                             0x001),
                  test_runtime_namespace_file_flow_round_trip,
                  "Runtime 真实会话文件系统回环",
                  "自动创建/进入 namespace 后执行路径解析、读写、链接、目录和 "
                  "xattr",
                  "Runtime/NAMEI/FOPS 主链路保持一致，清理后 namespace 可销毁"),
        TEST_CASE(UT_LIST_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_FLOW, 0x1),
                  UT_CASE_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_FLOW, 0x1,
                             0x002),
                  test_runtime_namespace_management_round_trip,
                  "Runtime namespace 管理回环",
                  "创建、列出、重命名、进入并递归删除非空 namespace",
                  "删除文件系统走上层递归，普通 destroy 保持 ENOTEMPTY 防护"),
};

const size_t RUNTIME_FLOW_CASE_COUNT =
        sizeof(RUNTIME_FLOW_CASES) / sizeof(RUNTIME_FLOW_CASES[0]);
