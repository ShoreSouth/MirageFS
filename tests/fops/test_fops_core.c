#include "fops_test_common.h"

static int test_fops_lifecycle_is_repeatable(void)
{
    fs_error_t err;

    err = fops_init();
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_init();
    TEST_ASSERT_EQ_INT(err, FS_OK);

    fops_deinit();
    fops_deinit();
    return 0;
}

static int test_fops_dispatch_rejects_null_args(void)
{
    fs_error_t err = fops_dispatch(NULL);

    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);
    return 0;
}

static int test_fops_dispatch_rejects_invalid_op(void)
{
    fs_error_t err;
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_MAX;

    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);
    return 0;
}

static int test_fops_op_spec_get_exposes_lookup_contract(void)
{
    const fops_op_spec_t *lookup_spec;
    const fops_op_spec_t *invalid_spec;

    lookup_spec = fops_op_spec_get(FS_OP_LOOKUP);
    invalid_spec = fops_op_spec_get(FS_OP_MAX);

    TEST_ASSERT_TRUE(lookup_spec != NULL);
    TEST_ASSERT_STR_EQ(lookup_spec->name, "lookup");
    TEST_ASSERT_TRUE(lookup_spec->need_parent);
    TEST_ASSERT_TRUE(lookup_spec->need_name);
    TEST_ASSERT_TRUE(lookup_spec->allow_dot_name);
    TEST_ASSERT_TRUE((lookup_spec->allowed_flags & FS_FLAG_DIRECTORY) != 0U);
    TEST_ASSERT_TRUE(invalid_spec == NULL);
    return 0;
}

static int test_fops_validate_args_missing_fields(void)
{
    fs_error_t err;
    fops_args_t args;
    fuid_t parent_fuid;

    parent_fuid = test_fops_make_fuid(FUID_TYPE_DIR);
    memset(&args, 0, sizeof(args));
    args.op = FS_OP_LOOKUP;
    args.parent_fuid = &parent_fuid;
    args.name = "child";
    args.flags = FS_FLAG_NOFOLLOW;

    err = fops_validate_args(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    args.parent_fuid = NULL;
    err = fops_validate_args(&args);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    args.parent_fuid = &parent_fuid;
    args.name = NULL;
    err = fops_validate_args(&args);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);
    return 0;
}

const test_case_t FOPS_CORE_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_CORE, 0x1),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_CORE, 0x1, 0x001),
                  test_fops_lifecycle_is_repeatable, "FOPS 生命周期",
                  "重复 init/deinit", "初始化成功，重复反初始化不崩溃"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x1),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x1,
                             0x001),
                  test_fops_dispatch_rejects_null_args, "FOPS dispatch 空参数",
                  "fops_dispatch 传入 NULL", "返回 FOPS 模块 EINVAL"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x1),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x1,
                             0x002),
                  test_fops_dispatch_rejects_invalid_op,
                  "FOPS dispatch 非法 op", "op 设置为 FS_OP_MAX",
                  "统一参数校验拒绝非法操作码"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x1),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x1,
                             0x003),
                  test_fops_op_spec_get_exposes_lookup_contract,
                  "FOPS op spec 查询", "查询 LOOKUP 和非法 op",
                  "LOOKUP 约束字段正确，非法 op 返回 NULL"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x1),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x1,
                             0x004),
                  test_fops_validate_args_missing_fields, "FOPS args 校验",
                  "构造合法 lookup 后移除 parent/name",
                  "合法参数通过，缺失必填字段返回 EINVAL")};

const size_t FOPS_CORE_CASE_COUNT =
        sizeof(FOPS_CORE_CASES) / sizeof(FOPS_CORE_CASES[0]);
