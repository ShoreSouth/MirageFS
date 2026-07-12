#include "framework/test_framework.h"

#include <errno.h>
#include <string.h>

#include "common/fs_common.h"
#include "fops/include/fops.h"
#include "fops/include/fops_types.h"

static int test_fops_lifecycle_is_repeatable(void)
{
    fs_error_t err;

    err = fops_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fops_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    fops_deinit();
    fops_deinit();
    return 0;
}

static int test_fops_dispatch_rejects_null_args(void)
{
    fs_error_t err = fops_dispatch(NULL);

    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static int test_fops_dispatch_rejects_invalid_op(void)
{
    fs_error_t err;
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_MAX;

    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static const test_case_t TEST_CASES[] = {
    TEST_CASE(test_fops_lifecycle_is_repeatable,
              "FOPS 生命周期",
              "重复 init/deinit",
              "初始化成功，重复反初始化不崩溃"),
    TEST_CASE(test_fops_dispatch_rejects_null_args,
              "FOPS dispatch 空参数",
              "fops_dispatch 传入 NULL",
              "返回 FOPS 模块 EINVAL"),
    TEST_CASE(test_fops_dispatch_rejects_invalid_op,
              "FOPS dispatch 非法 op",
              "op 设置为 FS_OP_MAX",
              "统一参数校验拒绝非法操作码"),
};

int main(void)
{
    return test_run_suite("fops", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
