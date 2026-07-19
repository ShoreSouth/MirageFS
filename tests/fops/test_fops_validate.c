#include "fops_test_common.h"

static int test_fops_validate_name_accepts_dot_only_when_allowed(void)
{
    fs_error_t err;
    char long_name[FS_MAX_NAME_LEN + 2U];

    err = fops_validate_name("child", FS_OP_LOOKUP, false);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_validate_name(".", FS_OP_LOOKUP, true);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_validate_name(".", FS_OP_CREATE, false);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    err = fops_validate_name("a/b", FS_OP_CREATE, false);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    memset(long_name, 'x', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = 0;
    err = fops_validate_name(long_name, FS_OP_CREATE, false);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENAMETOOLONG);
    return 0;
}

static int test_fops_validate_flags_rejects_unknown_and_conflict_bits(void)
{
    fs_error_t err;

    err = fops_validate_create_flags(FS_FLAG_REPLACE, FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_validate_create_flags(FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE,
                                     FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    err = fops_validate_mkdir_flags(FS_FLAG_REGULAR, FS_OP_MKDIR);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    err = fops_validate_flags(FS_OP_MAX, FS_FLAG_NONE, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);
    return 0;
}

static int test_fops_validate_specialized_flag_helpers(void)
{
    fs_error_t err;

    err = fops_validate_getattr_flags(FS_FLAG_DIRECTORY, FS_OP_GETATTR);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_validate_readdir_flags(FS_FLAG_DIRECTORY, FS_OP_READDIR);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_validate_readdir_flags(FS_FLAG_DIRECTORY, FS_OP_READDIRPLUS);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    fs_flags_t open_flags = FS_FLAG_READ | FS_FLAG_WRITE | FS_FLAG_APPEND;

    err = fops_validate_open_flags(open_flags, FS_OP_OPEN);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_validate_xattr_flags(FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE,
                                    FS_OP_SETXATTR);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);
    return 0;
}

static int test_fops_check_type_flags_maps_type_mismatch(void)
{
    fs_error_t err;

    err = fops_check_type_flags(FS_TYPE_DIR, FS_FLAG_DIRECTORY, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_check_type_flags(FS_TYPE_REG, FS_FLAG_DIRECTORY, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENOTDIR);

    err = fops_check_type_flags(FS_TYPE_DIR, FS_FLAG_REGULAR, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EISDIR);

    err = fops_check_type_flags(FS_TYPE_UNKNOWN, FS_FLAG_REGULAR, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);
    return 0;
}

static int test_fops_validate_create_attr_bad_mask_size(void)
{
    fs_error_t err;
    fops_create_attr_t attr;

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;

    err = fops_validate_create_attr(&attr, FOPS_CREATE_ATTR_MODE, FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_validate_create_attr(&attr, 0U, FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    attr.valid_mask = FOPS_CREATE_ATTR_SIZE;
    err = fops_apply_create_attr(-1, &attr, false, FS_OP_CREATE);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);
    return 0;
}

const test_case_t FOPS_VALIDATE_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1,
                         0x001),
              test_fops_validate_name_accepts_dot_only_when_allowed,
              "FOPS 名称校验",
              "普通名、点名、带 slash 名称和超长名称",
              "dot 仅在允许时通过，非法名称被拒绝"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1,
                         0x002),
              test_fops_validate_flags_rejects_unknown_and_conflict_bits,
              "FOPS flag 校验",
              "注入互斥 flag、不支持 flag 和非法 op",
              "返回 FOPS/EINVAL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1,
                         0x003),
              test_fops_validate_specialized_flag_helpers,
              "FOPS 专用 flag helper",
              "覆盖 getattr/readdir/open/xattr flag helper",
              "合法组合通过，互斥 xattr flag 被拒绝"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1,
                         0x004),
              test_fops_check_type_flags_maps_type_mismatch,
              "FOPS 类型约束",
              "目录/普通文件 flag 与实际类型不匹配",
              "返回 ENOTDIR/EISDIR/EINVAL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_VALIDATE,
                         0x1,
                         0x005),
              test_fops_validate_create_attr_bad_mask_size,
              "FOPS create attr 校验",
              "attr valid_mask 超出 supported_mask 或 size 不被允许",
              "返回 FOPS/EINVAL")
};

const size_t FOPS_VALIDATE_CASE_COUNT =
        sizeof(FOPS_VALIDATE_CASES) / sizeof(FOPS_VALIDATE_CASES[0]);
