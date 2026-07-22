#include "framework/test_framework.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "namei/include/namei.h"
#include "object/fuid/fuid.h"
#include "runtime/include/runtime.h"


typedef enum test_namei_component {
    TEST_NAMEI_COMPONENT_LIFECYCLE = 0x01,
    TEST_NAMEI_COMPONENT_CTX = 0x02,
    TEST_NAMEI_COMPONENT_LOOKUP = 0x03,
    TEST_NAMEI_COMPONENT_FLOW = 0x04,
} test_namei_component_t;

static void test_namei_cleanup_root(void)
{
    int rc;

    rc = system("rm -rf -- './miragefs.root'");
    (void)rc;
}

static int test_namei_lifecycle_is_repeatable(void)
{
    fs_error_t err;

    err = namei_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = namei_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    namei_deinit();
    namei_deinit();
    return 0;
}

static int test_namei_ctx_make_copies_root_and_cwd(void)
{
    namei_ctx_t ctx;
    fuid_t root = fuid_make(1, 10, 1, FUID_TYPE_DIR);
    fuid_t cwd = fuid_make(1, 20, 1, FUID_TYPE_DIR);

    namei_ctx_make(&ctx, &root, &cwd);

    TEST_ASSERT_TRUE(fuid_equal(&ctx.root_fuid, &root));
    TEST_ASSERT_TRUE(fuid_equal(&ctx.cwd_fuid, &cwd));
    return 0;
}

static int test_namei_lookup_rejects_null_ctx(void)
{
    fs_error_t err;
    fuid_t out;

    err = namei_lookup(NULL, "/", 0, &out);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static int test_namei_rejects_bad_context_and_outputs(void)
{
    namei_ctx_t ctx;
    fuid_t root = fuid_make(1, 10, 1, FUID_TYPE_FILE);
    fuid_t cwd = fuid_make(2, 20, 1, FUID_TYPE_DIR);
    fuid_t out;
    fops_object_result_t plus;
    namei_parent_result_t parent;
    fs_error_t err;

    namei_ctx_make(&ctx, &root, &cwd);
    err = namei_lookup(&ctx, "/", 0, &out);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    root = fuid_make(1, 10, 1, FUID_TYPE_DIR);
    namei_ctx_make(&ctx, &root, &cwd);
    err = namei_lookup(&ctx, "/", 0, &out);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = namei_lookup(&ctx, "/", 0, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = namei_lookup_plus(&ctx, "/", 0, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = namei_lookup_parent(&ctx, "/", 0, &parent);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = namei_lookup_parent(&ctx, "/dir/", 0, &parent);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    err = namei_lookup_parent(&ctx, "/leaf", 0, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_NAMEI, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    (void)plus;
    return 0;
}

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

static const test_case_t TEST_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_NAMEI,
                         TEST_NAMEI_COMPONENT_LIFECYCLE,
                         0x1),
              UT_CASE_NO(UT_MOD_NAMEI,
                         TEST_NAMEI_COMPONENT_LIFECYCLE,
                         0x1,
                         0x001),
              test_namei_lifecycle_is_repeatable,
              "NAMEI 生命周期",
              "重复 init/deinit",
              "初始化成功，重复反初始化不崩溃"),
    TEST_CASE(UT_LIST_NO(UT_MOD_NAMEI,
                         TEST_NAMEI_COMPONENT_CTX,
                         0x1),
              UT_CASE_NO(UT_MOD_NAMEI,
                         TEST_NAMEI_COMPONENT_CTX,
                         0x1,
                         0x001),
              test_namei_ctx_make_copies_root_and_cwd,
              "NAMEI 上下文构造",
              "传入 root/cwd FUID",
              "ctx 正确保存 root 和 cwd"),
    TEST_CASE(UT_LIST_NO(UT_MOD_NAMEI,
                         TEST_NAMEI_COMPONENT_LOOKUP,
                         0x1),
              UT_CASE_NO(UT_MOD_NAMEI,
                         TEST_NAMEI_COMPONENT_LOOKUP,
                         0x1,
                         0x001),
              test_namei_lookup_rejects_null_ctx,
              "NAMEI lookup 空上下文",
              "lookup 传入 NULL ctx",
              "返回 NAMEI 模块 EINVAL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_NAMEI,
                         TEST_NAMEI_COMPONENT_LOOKUP,
                         0x1),
              UT_CASE_NO(UT_MOD_NAMEI,
                         TEST_NAMEI_COMPONENT_LOOKUP,
                         0x1,
                         0x002),
              test_namei_rejects_bad_context_and_outputs,
              "NAMEI 上下文和输出参数边界",
              "注入非法 root/cwd、空输出和非法 parent 路径",
              "返回 NAMEI 模块 EINVAL"),
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

int main(void)
{
    return test_run_suite("namei", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
