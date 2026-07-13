#include "framework/test_framework.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "common/fs_common.h"
#include "fops/include/fops.h"
#include "fops/include/fops_types.h"
#include "fops/internal/fops_internal.h"

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

static int test_fops_validate_name_accepts_dot_only_when_allowed(void)
{
    fs_error_t err;

    err = fops_validate_name("child", FS_OP_LOOKUP, false);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fops_validate_name(".", FS_OP_LOOKUP, true);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fops_validate_name(".", FS_OP_CREATE, false);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fops_validate_name("a/b", FS_OP_CREATE, false);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static int test_fops_validate_flags_rejects_unknown_and_conflict_bits(void)
{
    fs_error_t err;

    err = fops_validate_create_flags(FS_FLAG_REPLACE, FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fops_validate_create_flags(FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE,
                                     FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = fops_validate_mkdir_flags(FS_FLAG_REGULAR, FS_OP_MKDIR);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static int test_fops_check_type_flags_maps_type_mismatch(void)
{
    fs_error_t err;

    err = fops_check_type_flags(FS_TYPE_DIR, FS_FLAG_DIRECTORY, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fops_check_type_flags(FS_TYPE_REG, FS_FLAG_DIRECTORY, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOTDIR, fs_err_errno(err));

    err = fops_check_type_flags(FS_TYPE_DIR, FS_FLAG_REGULAR, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EISDIR, fs_err_errno(err));
    return 0;
}

static int test_fops_create_mode_masks_permissions(void)
{
    fops_create_attr_t attr;

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    attr.mode = 07777;

    TEST_ASSERT_EQ_INT(07777, fops_create_mode(&attr, 0644));
    TEST_ASSERT_EQ_INT(01644, fops_create_mode(NULL, 01644));
    return 0;
}

static int test_fops_validate_create_attr_rejects_unsupported_mask(void)
{
    fs_error_t err;
    fops_create_attr_t attr;

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;

    err = fops_validate_create_attr(&attr, FOPS_CREATE_ATTR_MODE, FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fops_validate_create_attr(&attr, 0U, FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(FS_MODULE_FOPS, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    return 0;
}

static const test_case_t TEST_CASES[] = {
    TEST_CASE(test_fops_lifecycle_is_repeatable, "FOPS 生命周期", "重复 init/deinit", "初始化成功，重复反初始化不崩溃"),
    TEST_CASE(test_fops_dispatch_rejects_null_args, "FOPS dispatch 空参数", "fops_dispatch 传入 NULL", "返回 FOPS 模块 EINVAL"),
    TEST_CASE(test_fops_dispatch_rejects_invalid_op, "FOPS dispatch 非法 op", "op 设置为 FS_OP_MAX", "统一参数校验拒绝非法操作码"),
    TEST_CASE(test_fops_validate_name_accepts_dot_only_when_allowed, "FOPS 名称校验", "普通名、点名、带 slash 名称", "dot 仅在允许时通过，slash 和非法 dot 被拒绝"),
    TEST_CASE(test_fops_validate_flags_rejects_unknown_and_conflict_bits, "FOPS flag 校验", "注入互斥 flag 和不支持 flag", "返回 FOPS/EINVAL"),
    TEST_CASE(test_fops_check_type_flags_maps_type_mismatch, "FOPS 类型约束", "目录/普通文件 flag 与实际类型不匹配", "返回 ENOTDIR/EISDIR"),
    TEST_CASE(test_fops_create_mode_masks_permissions, "FOPS create mode", "传入带特殊位的 mode/default_mode", "按 FS_PERM_MASK 保留权限相关位"),
    TEST_CASE(test_fops_validate_create_attr_rejects_unsupported_mask, "FOPS create attr mask", "attr valid_mask 超出 supported_mask", "返回 FOPS/EINVAL"),
};

int main(void)
{
    return test_run_suite("fops", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}