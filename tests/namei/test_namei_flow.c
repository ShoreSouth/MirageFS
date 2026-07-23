#include "namei_test_common.h"

static int test_namei_real_namespace_walk_and_ops_edges(void)
{
    runtime_config_t cfg;
    namei_ctx_t ctx;
    namei_ctx_t loop_ctx;
    fops_create_attr_t attr;
    fops_object_result_t out;
    fops_object_result_t plus;
    fops_dirent_t entries[8];
    uint32_t entry_nr;
    bool eof;
    namei_readdir_args_t readdir_args;
    namei_parent_result_t parent;
    fuid_t fuid;
    fuid_t symlink_fuid;
    char namespace_name[32];
    char target_buf[64];
    size_t actual;
    fs_error_t err;

    runtime_deinit();
    test_namei_cleanup_root();
    memset(&cfg, 0, sizeof(cfg));
    (void)snprintf(namespace_name,
                   sizeof(namespace_name),
                   "ni_%ld",
                   (long)getpid());
    cfg.default_namespace = namespace_name;
    cfg.auto_create = true;
    cfg.auto_use = true;
    err = runtime_init(&cfg);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = runtime_get_ctx(&ctx);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    /* NAMEI 测试通过 runtime 建真实 namespace，但直接调用 namei API。 */
    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    attr.mode = FS_MODE_FILE_DEFAULT;

    err = namei_mkdir(&ctx, "/dir", NULL, FS_FLAG_NONE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_create(&ctx, "rel.txt", &attr, FS_FLAG_EXCLUSIVE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_create(&ctx, "/dir/target", &attr, FS_FLAG_EXCLUSIVE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_symlink(&ctx, "target", "/dir/link", FS_FLAG_EXCLUSIVE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_symlink(&ctx, "loop", "/dir/loop", FS_FLAG_EXCLUSIVE, &out);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    /* 覆盖绝对路径、相对路径、点目录、软链接跟随和 final NOFOLLOW。 */
    err = namei_lookup(&ctx, "/", FS_FLAG_DIRECTORY, &fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_dir(&fuid));
    err = namei_lookup(&ctx, ".", FS_FLAG_DIRECTORY, &fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_lookup(&ctx, "..", FS_FLAG_DIRECTORY, &fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_lookup_plus(&ctx, "/dir/target", FS_FLAG_NONE, &plus);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(FS_TYPE_REG, plus.attr.type);
    err = namei_lookup(&ctx, "/dir/link", FS_FLAG_NOFOLLOW, &symlink_fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_symlink(&symlink_fuid));
    err = namei_lookup(&ctx, "/dir/link", FS_FLAG_REGULAR, &fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fuid_is_file(&fuid));
    err = namei_lookup(&ctx, "/dir/target/child", FS_FLAG_NONE, &fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOTDIR, fs_err_errno(err));

    loop_ctx = ctx;
    loop_ctx.max_symlink_depth = 1U;
    /* 降低 symlink 深度上限，强制自环链接进入 ELOOP 错误路径。 */
    err = namei_lookup(&loop_ctx, "/dir/loop", FS_FLAG_NONE, &fuid);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ELOOP, fs_err_errno(err));

    err = namei_lookup_parent(&ctx, "/dir/target", FS_FLAG_NONE, &parent);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(parent.name, "target");
    err = namei_lookup_parent(&ctx, "rel.txt", FS_FLAG_NONE, &parent);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(parent.name, "rel.txt");

    memset(target_buf, 0, sizeof(target_buf));
    err = namei_readlink(&ctx,
                         "/dir/link",
                         FS_FLAG_NONE,
                         target_buf,
                         sizeof(target_buf),
                         &actual);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(target_buf, "target");

    memset(&readdir_args, 0, sizeof(readdir_args));
    readdir_args.entries = entries;
    readdir_args.entry_cap = 8U;
    readdir_args.out_entry_nr = &entry_nr;
    readdir_args.out_eof = &eof;
    err = namei_readdir(&ctx, "/dir", FS_FLAG_NONE, &readdir_args);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(entry_nr >= 3U);
    /* readdir 参数结构由 NAMEI 负责校验，不能把 NULL 传进 FOPS。 */
    err = namei_readdir(&ctx, "/dir", FS_FLAG_NONE, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = namei_unlink(&ctx, "/dir/link", FS_FLAG_NOFOLLOW);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_unlink(&ctx, "/dir/loop", FS_FLAG_NOFOLLOW);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_unlink(&ctx, "/dir/target", FS_FLAG_REGULAR);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_unlink(&ctx, "rel.txt", FS_FLAG_REGULAR);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = namei_rmdir(&ctx, "/dir", FS_FLAG_DIRECTORY);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = runtime_fs_destroy(namespace_name);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    runtime_deinit();
    test_namei_cleanup_root();
    return 0;
}


const test_case_t NAMEI_FLOW_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_NAMEI,
                             TEST_NAMEI_COMPONENT_FLOW,
                             0x1),
                  UT_CASE_NO(UT_MOD_NAMEI,
                             TEST_NAMEI_COMPONENT_FLOW,
                             0x1,
                             0x001),
                  test_namei_real_namespace_walk_and_ops_edges,
                  "NAMEI 真实 namespace 路径解析回环",
                  "创建目录/文件/软链接后覆盖绝对、相对、点点、NOFOLLOW、ELOOP 和 readdir",
                  "路径解析和对象操作按预期成功，非法路径返回结构化错误"),
};

const size_t NAMEI_FLOW_CASE_COUNT = sizeof(NAMEI_FLOW_CASES) / sizeof(NAMEI_FLOW_CASES[0]);
