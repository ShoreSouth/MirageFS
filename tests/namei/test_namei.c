#include "framework/test_framework.h"

#include <errno.h>

#include "common/fs_common.h"
#include "namei/include/namei.h"
#include "object/fuid/fuid.h"


typedef enum test_namei_component {
    TEST_NAMEI_COMPONENT_LIFECYCLE = 0x01,
    TEST_NAMEI_COMPONENT_CTX = 0x02,
    TEST_NAMEI_COMPONENT_LOOKUP = 0x03,
} test_namei_component_t;

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
};

int main(void)
{
    return test_run_suite("namei", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
