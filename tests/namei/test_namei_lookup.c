#include "namei_test_common.h"

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


const test_case_t NAMEI_LOOKUP_CASES[] = {
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
};

const size_t NAMEI_LOOKUP_CASE_COUNT = sizeof(NAMEI_LOOKUP_CASES) / sizeof(NAMEI_LOOKUP_CASES[0]);
