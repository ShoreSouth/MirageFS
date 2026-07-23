#include "runtime_test_common.h"

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


const test_case_t RUNTIME_SESSION_CASES[] = {
        TEST_CASE(
                UT_LIST_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_SESSION, 0x1),
                UT_CASE_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_SESSION, 0x1,
                           0x001),
                test_runtime_requires_init_for_session_ops,
                "Runtime 未初始化保护", "未 init 时调用 getcwd/leave",
                "返回 RUNTIME 模块 EINVAL"),
        TEST_CASE(
                UT_LIST_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_SESSION, 0x1),
                UT_CASE_NO(UT_MOD_RUNTIME, TEST_RUNTIME_COMPONENT_SESSION, 0x1,
                           0x002),
                test_runtime_initialized_without_session_rejects_ctx_ops,
                "Runtime 未进入 namespace 保护",
                "已 init 但未 fs use 时读取 ctx/root/cwd",
                "返回 RUNTIME/SESSION/ENOENT"),
};

const size_t RUNTIME_SESSION_CASE_COUNT =
        sizeof(RUNTIME_SESSION_CASES) / sizeof(RUNTIME_SESSION_CASES[0]);
