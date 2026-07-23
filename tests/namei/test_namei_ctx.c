#include "namei_test_common.h"

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


const test_case_t NAMEI_CTX_CASES[] = {
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
};

const size_t NAMEI_CTX_CASE_COUNT = sizeof(NAMEI_CTX_CASES) / sizeof(NAMEI_CTX_CASES[0]);
