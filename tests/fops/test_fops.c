#include "framework/test_framework.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "common/fs_common.h"
#include "fops/include/fops.h"
#include "fops/include/fops_types.h"
#include "fops/internal/fops_internal.h"

static fuid_t make_test_fuid(fuid_type_t type)
{
    fuid_t fuid;

    fuid = fuid_make(7U, 100U, 1U, type);
    fuid.qtreeid = 11U;
    fuid.snapid = 13U;
    fuid.shardid = 17U;
    return fuid;
}

static obj_handle_t make_test_handle(int32_t mount_id)
{
    obj_handle_t handle;

    memset(&handle, 0, sizeof(handle));
    handle.mount_id = mount_id;
    handle.type = 1U;
    handle.len = 4U;
    handle.data[0] = 0xaaU;
    handle.data[1] = 0xbbU;
    handle.data[2] = 0xccU;
    handle.data[3] = 0xddU;
    return handle;
}

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

static int test_fops_validate_args_accepts_valid_lookup_and_rejects_missing_fields(void)
{
    fs_error_t err;
    fops_args_t args;
    fuid_t parent_fuid;

    parent_fuid = make_test_fuid(FUID_TYPE_DIR);
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

    err = fops_validate_open_flags(FS_FLAG_READ | FS_FLAG_WRITE | FS_FLAG_APPEND,
                                   FS_OP_OPEN);
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

static int test_fops_create_mode_masks_permissions(void)
{
    fops_create_attr_t attr;

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    attr.mode = 07777;

    TEST_ASSERT_EQ_INT(fops_create_mode(&attr, 0644), 07777);
    TEST_ASSERT_EQ_INT(fops_create_mode(NULL, 01644), 01644);
    return 0;
}

static int test_fops_validate_create_attr_rejects_unsupported_mask_and_size(void)
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

static int test_fops_linux_open_flags_maps_access_and_modifiers(void)
{
    int flags;

    flags = fops_linux_open_flags(FS_FLAG_READ);
    TEST_ASSERT_TRUE((flags & O_ACCMODE) == O_RDONLY);
    TEST_ASSERT_TRUE((flags & O_CLOEXEC) != 0);

    flags = fops_linux_open_flags(FS_FLAG_WRITE | FS_FLAG_APPEND |
                                  FS_FLAG_TRUNCATE | FS_FLAG_SYNC |
                                  FS_FLAG_DIRECTORY);
    TEST_ASSERT_TRUE((flags & O_ACCMODE) == O_WRONLY);
    TEST_ASSERT_TRUE((flags & O_APPEND) != 0);
    TEST_ASSERT_TRUE((flags & O_TRUNC) != 0);
    TEST_ASSERT_TRUE((flags & O_SYNC) != 0);
    TEST_ASSERT_TRUE((flags & O_DIRECTORY) != 0);

    flags = fops_linux_open_flags(FS_FLAG_READ | FS_FLAG_WRITE);
    TEST_ASSERT_TRUE((flags & O_ACCMODE) == O_RDWR);
    return 0;
}

static int test_fops_file_check_rejects_bad_handles_and_access_modes(void)
{
    fs_error_t err;
    fops_file_t file;
    obj_meta_t meta;

    memset(&file, 0, sizeof(file));
    memset(&meta, 0, sizeof(meta));

    err = fops_file_check(NULL, FS_OP_READ);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EBADF);

    file.meta = &meta;
    file.fd = 3;
    file.flags = FS_FLAG_WRITE;
    err = fops_file_check(&file, FS_OP_READ);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EBADF);

    file.flags = FS_FLAG_READ;
    err = fops_file_check(&file, FS_OP_WRITE);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EBADF);

    file.flags = FS_FLAG_READ | FS_FLAG_WRITE;
    err = fops_file_check(&file, FS_OP_WRITE);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    return 0;
}

static int test_fops_type_and_attr_helpers_convert_linux_stat(void)
{
    struct stat st;
    fops_attr_t attr;

    memset(&st, 0, sizeof(st));
    st.st_mode = S_IFDIR | 0750;
    st.st_uid = 123U;
    st.st_gid = 456U;
    st.st_size = 789;
    st.st_nlink = 2U;
    st.st_atim.tv_sec = 10;
    st.st_mtim.tv_sec = 20;
    st.st_ctim.tv_sec = 30;

    TEST_ASSERT_EQ_INT(fops_type_from_mode(S_IFREG | 0644), FS_TYPE_REG);
    TEST_ASSERT_EQ_INT(fops_type_from_mode(S_IFDIR | 0755), FS_TYPE_DIR);
    TEST_ASSERT_EQ_INT(fops_fuid_type_from_fs_type(FS_TYPE_LNK), FUID_TYPE_SYMLINK);
    TEST_ASSERT_EQ_INT(fops_fuid_type_from_fs_type(FS_TYPE_UNKNOWN), FUID_TYPE_INVALID);

    fops_attr_from_stat(&attr, &st);
    TEST_ASSERT_EQ_INT(attr.type, FS_TYPE_DIR);
    TEST_ASSERT_EQ_INT(attr.mode, (S_IFDIR | 0750));
    TEST_ASSERT_EQ_INT(attr.uid, 123U);
    TEST_ASSERT_EQ_INT(attr.gid, 456U);
    TEST_ASSERT_EQ_INT(attr.size, 789U);
    TEST_ASSERT_EQ_INT(attr.nlink, 2U);
    TEST_ASSERT_EQ_INT(attr.atime_sec, 10U);
    TEST_ASSERT_EQ_INT(attr.mtime_sec, 20U);
    TEST_ASSERT_EQ_INT(attr.ctime_sec, 30U);

    fops_attr_from_stat(NULL, &st);
    fops_attr_from_stat(&attr, NULL);
    return 0;
}

static int test_fops_make_child_fuid_inherits_parent_view(void)
{
    fuid_t parent_fuid;
    fuid_t child_fuid;

    parent_fuid = make_test_fuid(FUID_TYPE_DIR);
    child_fuid = fops_make_child_fuid(&parent_fuid, 200U, 3U, FS_TYPE_REG);

    TEST_ASSERT_EQ_INT(child_fuid.fsid, parent_fuid.fsid);
    TEST_ASSERT_EQ_INT(child_fuid.objectid, 200U);
    TEST_ASSERT_EQ_INT(child_fuid.gen, 3U);
    TEST_ASSERT_EQ_INT(child_fuid.type, FUID_TYPE_FILE);
    TEST_ASSERT_EQ_INT(child_fuid.qtreeid, parent_fuid.qtreeid);
    TEST_ASSERT_EQ_INT(child_fuid.snapid, parent_fuid.snapid);
    TEST_ASSERT_EQ_INT(child_fuid.shardid, parent_fuid.shardid);
    return 0;
}

static int test_fops_handle_from_lsa_checked_validates_mount_boundary(void)
{
    fs_error_t err;
    obj_meta_t parent_meta;
    obj_handle_t out_handle;
    lsa_file_handle_t lsa_handle;

    memset(&parent_meta, 0, sizeof(parent_meta));
    memset(&lsa_handle, 0, sizeof(lsa_handle));
    parent_meta.handle = make_test_handle(42);
    lsa_handle.handle_bytes = 4U;
    lsa_handle.handle_type = 9;
    lsa_handle.data[0] = 1U;
    lsa_handle.data[1] = 2U;
    lsa_handle.data[2] = 3U;
    lsa_handle.data[3] = 4U;

    err = fops_handle_from_lsa_checked(NULL, &lsa_handle, 42, &parent_meta,
                                       FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    err = fops_handle_from_lsa_checked(&out_handle, &lsa_handle, 43,
                                       &parent_meta, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_FOPS);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EXDEV);

    err = fops_handle_from_lsa_checked(&out_handle, &lsa_handle, 42,
                                       &parent_meta, FS_OP_LOOKUP);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(out_handle.mount_id, 42);
    TEST_ASSERT_EQ_INT(out_handle.type, 9);
    TEST_ASSERT_EQ_INT(out_handle.len, 4U);
    TEST_ASSERT_EQ_INT(out_handle.data[2], 3U);
    return 0;
}

static const test_case_t TEST_CASES[] = {
    TEST_CASE(test_fops_lifecycle_is_repeatable, "FOPS 生命周期", "重复 init/deinit", "初始化成功，重复反初始化不崩溃"),
    TEST_CASE(test_fops_dispatch_rejects_null_args, "FOPS dispatch 空参数", "fops_dispatch 传入 NULL", "返回 FOPS 模块 EINVAL"),
    TEST_CASE(test_fops_dispatch_rejects_invalid_op, "FOPS dispatch 非法 op", "op 设置为 FS_OP_MAX", "统一参数校验拒绝非法操作码"),
    TEST_CASE(test_fops_op_spec_get_exposes_lookup_contract, "FOPS op spec 查询", "查询 LOOKUP 和非法 op", "LOOKUP 约束字段正确，非法 op 返回 NULL"),
    TEST_CASE(test_fops_validate_args_accepts_valid_lookup_and_rejects_missing_fields, "FOPS args 校验", "构造合法 lookup 后移除 parent/name", "合法参数通过，缺失必填字段返回 EINVAL"),
    TEST_CASE(test_fops_validate_name_accepts_dot_only_when_allowed, "FOPS 名称校验", "普通名、点名、带 slash 名称和超长名称", "dot 仅在允许时通过，非法名称被拒绝"),
    TEST_CASE(test_fops_validate_flags_rejects_unknown_and_conflict_bits, "FOPS flag 校验", "注入互斥 flag、不支持 flag 和非法 op", "返回 FOPS/EINVAL"),
    TEST_CASE(test_fops_validate_specialized_flag_helpers, "FOPS 专用 flag helper", "覆盖 getattr/readdir/open/xattr flag helper", "合法组合通过，互斥 xattr flag 被拒绝"),
    TEST_CASE(test_fops_check_type_flags_maps_type_mismatch, "FOPS 类型约束", "目录/普通文件 flag 与实际类型不匹配", "返回 ENOTDIR/EISDIR/EINVAL"),
    TEST_CASE(test_fops_create_mode_masks_permissions, "FOPS create mode", "传入带特殊位的 mode/default_mode", "按 FS_PERM_MASK 保留权限相关位"),
    TEST_CASE(test_fops_validate_create_attr_rejects_unsupported_mask_and_size, "FOPS create attr 校验", "attr valid_mask 超出 supported_mask 或 size 不被允许", "返回 FOPS/EINVAL"),
    TEST_CASE(test_fops_linux_open_flags_maps_access_and_modifiers, "FOPS Linux open flag 转换", "构造读写方向和 append/truncate/sync/directory modifier", "转换为预期 Linux O_* flag"),
    TEST_CASE(test_fops_file_check_rejects_bad_handles_and_access_modes, "FOPS file 校验", "NULL file、写只读、读写只写句柄", "非法句柄返回 EBADF，读写句柄通过"),
    TEST_CASE(test_fops_type_and_attr_helpers_convert_linux_stat, "FOPS stat 属性转换", "构造 Linux struct stat", "type/mode/uid/gid/size/time 字段正确映射"),
    TEST_CASE(test_fops_make_child_fuid_inherits_parent_view, "FOPS 子 FUID 构造", "从父目录 FUID 派生普通文件 FUID", "继承 fsid/view 字段并设置 child identity"),
    TEST_CASE(test_fops_handle_from_lsa_checked_validates_mount_boundary, "FOPS handle 转换边界", "NULL 输出、跨 mount、合法 LSA handle", "非法参数返回 EINVAL/EXDEV，合法时完成字段转换"),
};

int main(void)
{
    return test_run_suite("fops", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
