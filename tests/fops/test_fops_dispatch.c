#include "fops_test_common.h"

#include <stdio.h>

static fs_error_t test_fops_dispatch_create(test_fops_env_t *env,
                                            const char *name,
                                            fops_object_result_t *out)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_CREATE;
    args.parent_fuid = &env->root_fuid;
    args.name = name;
    args.flags = FS_FLAG_EXCLUSIVE | FS_FLAG_REGULAR;
    args.u.create.out = out;
    return fops_dispatch(&args);
}

static fs_error_t test_fops_dispatch_unlink(test_fops_env_t *env,
                                            const char *name)
{
    fops_args_t args;

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_UNLINK;
    args.parent_fuid = &env->root_fuid;
    args.name = name;
    args.flags = FS_FLAG_NONE;
    return fops_dispatch(&args);
}

static int test_fops_dispatch_rejects_bad_request(void)
{
    fops_args_t args;
    fs_error_t err;

    err = fops_dispatch(NULL);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_MAX;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_CREATE;
    args.flags = FS_FLAG_EXCLUSIVE;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EINVAL);

    return 0;
}

static int test_fops_dispatch_file_attr_xattr_round_trip(void)
{
    test_fops_env_t env;
    fops_object_result_t created;
    fops_object_result_t looked_up;
    fops_file_t *file;
    fops_attr_t attr;
    fops_setattr_t setattr;
    fops_statfs_t statfs_buf;
    obj_handle_t handle;
    fops_args_t args;
    char read_buf[8];
    char xattr_buf[16];
    char xattr_list[64];
    size_t actual;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    err = test_fops_dispatch_create(&env, "dispatch.txt", &created);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_file(&created.fuid));

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_LOOKUP;
    args.parent_fuid = &env.root_fuid;
    args.name = "dispatch.txt";
    args.flags = FS_FLAG_REGULAR;
    args.u.lookup.out = &looked_up;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(looked_up.fuid.objectid, created.fuid.objectid);

    file = NULL;
    memset(&args, 0, sizeof(args));
    args.op = FS_OP_OPEN;
    args.fuid = &created.fuid;
    args.flags = FS_FLAG_READ | FS_FLAG_WRITE | FS_FLAG_REGULAR;
    args.u.open.out_file = &file;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(file != NULL);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_WRITE;
    args.u.write.file = file;
    args.u.write.buf = "hello";
    args.u.write.size = 5U;
    args.u.write.actual = &actual;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 5U);

    TEST_ASSERT_EQ_INT(lseek(file->fd, 0, SEEK_SET), 0);
    memset(read_buf, 0, sizeof(read_buf));
    memset(&args, 0, sizeof(args));
    args.op = FS_OP_READ;
    args.u.read.file = file;
    args.u.read.buf = read_buf;
    args.u.read.size = 5U;
    args.u.read.actual = &actual;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 5U);
    TEST_ASSERT_STR_EQ(read_buf, "hello");

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_CLOSE;
    args.u.close.file = file;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&setattr, 0, sizeof(setattr));
    setattr.valid_mask = FOPS_SETATTR_MODE | FOPS_SETATTR_SIZE;
    setattr.mode = 0640;
    setattr.size = 2U;
    memset(&args, 0, sizeof(args));
    args.op = FS_OP_SETATTR;
    args.fuid = &created.fuid;
    args.flags = FS_FLAG_REGULAR;
    args.u.setattr.attr = &setattr;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_GETATTR;
    args.fuid = &created.fuid;
    args.flags = FS_FLAG_REGULAR;
    args.u.getattr.out_attr = &attr;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(attr.size, 2U);
    TEST_ASSERT_EQ_INT(attr.mode & FS_PERM_MASK, 0640);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_ACCESS;
    args.fuid = &created.fuid;
    args.flags = FS_FLAG_REGULAR;
    args.u.access.mask = F_OK;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_GETHANDLE;
    args.fuid = &created.fuid;
    args.u.gethandle.out_handle = &handle;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    file = NULL;
    memset(&args, 0, sizeof(args));
    args.op = FS_OP_OPENHANDLE;
    args.flags = FS_FLAG_READ | FS_FLAG_REGULAR;
    args.u.openhandle.handle = &handle;
    args.u.openhandle.out_file = &file;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(file != NULL);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_CLOSE;
    args.u.close.file = file;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_TRUNCATE;
    args.fuid = &created.fuid;
    args.flags = FS_FLAG_REGULAR;
    args.u.truncate.size = 1U;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_SETXATTR;
    args.fuid = &created.fuid;
    args.name = "user.miragefs.dispatch";
    args.u.setxattr.value = "v1";
    args.u.setxattr.size = 2U;
    err = fops_dispatch(&args);
    if (err == FS_OK)
    {
        memset(xattr_buf, 0, sizeof(xattr_buf));
        memset(&args, 0, sizeof(args));
        args.op = FS_OP_GETXATTR;
        args.fuid = &created.fuid;
        args.name = "user.miragefs.dispatch";
        args.u.getxattr.value = xattr_buf;
        args.u.getxattr.size = sizeof(xattr_buf);
        args.u.getxattr.actual = &actual;
        err = fops_dispatch(&args);
        TEST_ASSERT_EQ_INT(err, FS_OK);
        TEST_ASSERT_EQ_INT(actual, 2U);
        TEST_ASSERT_STR_EQ(xattr_buf, "v1");

        memset(&args, 0, sizeof(args));
        args.op = FS_OP_LISTXATTR;
        args.fuid = &created.fuid;
        args.u.listxattr.list = xattr_list;
        args.u.listxattr.size = sizeof(xattr_list);
        args.u.listxattr.actual = &actual;
        err = fops_dispatch(&args);
        TEST_ASSERT_EQ_INT(err, FS_OK);
        TEST_ASSERT_TRUE(actual > 0U);

        memset(&args, 0, sizeof(args));
        args.op = FS_OP_REMOVEXATTR;
        args.fuid = &created.fuid;
        args.name = "user.miragefs.dispatch";
        err = fops_dispatch(&args);
        TEST_ASSERT_EQ_INT(err, FS_OK);
    }
    else
    {
        TEST_ASSERT_TRUE(fs_err_errno(err) == ENOTSUP ||
                         fs_err_errno(err) == EOPNOTSUPP ||
                         fs_err_errno(err) == EPERM);
    }

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_STATFS;
    args.fuid = &env.root_fuid;
    args.u.statfs.out_statfs = &statfs_buf;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_SYNCFS;
    args.fuid = &env.root_fuid;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = test_fops_dispatch_unlink(&env, "dispatch.txt");
    TEST_ASSERT_EQ_INT(err, FS_OK);
    test_fops_env_teardown(&env);
    return 0;
}

static int test_fops_dispatch_namespace_round_trip(void)
{
    test_fops_env_t env;
    fops_object_result_t dir_out;
    fops_object_result_t fifo_out;
    fops_object_result_t base_out;
    fops_object_result_t link_out;
    fops_object_result_t symlink_out;
    fops_dirent_t entries[8];
    fops_dirent_plus_t plus_entries[8];
    fops_create_attr_t attr;
    fops_args_t args;
    char link_buf[64];
    uint32_t entry_nr;
    size_t actual;
    bool eof;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_MKDIR;
    args.parent_fuid = &env.root_fuid;
    args.name = "dispatch_dir";
    args.flags = FS_FLAG_EXCLUSIVE | FS_FLAG_DIRECTORY;
    args.u.mkdir.out = &dir_out;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_dir(&dir_out.fuid));

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    attr.mode = 0600;
    memset(&args, 0, sizeof(args));
    args.op = FS_OP_MKNOD;
    args.parent_fuid = &env.root_fuid;
    args.name = "dispatch_fifo";
    args.flags = FS_FLAG_EXCLUSIVE;
    args.u.mknod.type = FS_TYPE_FIFO;
    args.u.mknod.attr = &attr;
    args.u.mknod.out = &fifo_out;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(fifo_out.fuid.type, FUID_TYPE_FIFO);

    err = test_fops_dispatch_create(&env, "base.txt", &base_out);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_LINK;
    args.parent_fuid = &env.root_fuid;
    args.name = "base.txt";
    args.flags = FS_FLAG_EXCLUSIVE;
    args.u.link.new_parent_fuid = &env.root_fuid;
    args.u.link.new_name = "linked.txt";
    args.u.link.out = &link_out;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_file(&link_out.fuid));

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_RENAME;
    args.parent_fuid = &env.root_fuid;
    args.name = "linked.txt";
    args.u.rename.new_parent_fuid = &env.root_fuid;
    args.u.rename.new_name = "renamed.txt";
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_SYMLINK;
    args.parent_fuid = &env.root_fuid;
    args.name = "dispatch_link";
    args.flags = FS_FLAG_EXCLUSIVE;
    args.u.symlink.target = "base.txt";
    args.u.symlink.out = &symlink_out;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_symlink(&symlink_out.fuid));

    memset(link_buf, 0, sizeof(link_buf));
    memset(&args, 0, sizeof(args));
    args.op = FS_OP_READLINK;
    args.parent_fuid = &env.root_fuid;
    args.name = "dispatch_link";
    args.u.readlink.buf = link_buf;
    args.u.readlink.size = sizeof(link_buf);
    args.u.readlink.actual = &actual;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, strlen("base.txt"));
    TEST_ASSERT_STR_EQ(link_buf, "base.txt");

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_READDIR;
    args.fuid = &env.root_fuid;
    args.flags = FS_FLAG_DIRECTORY;
    args.u.readdir.entries = entries;
    args.u.readdir.entry_cap = 8U;
    args.u.readdir.out_entry_nr = &entry_nr;
    args.u.readdir.out_eof = &eof;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(entry_nr >= 4U);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_READDIRPLUS;
    args.fuid = &env.root_fuid;
    args.flags = FS_FLAG_DIRECTORY;
    args.u.readdirplus.entries = plus_entries;
    args.u.readdirplus.entry_cap = 8U;
    args.u.readdirplus.out_entry_nr = &entry_nr;
    args.u.readdirplus.out_eof = &eof;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(entry_nr >= 4U);

    err = test_fops_dispatch_unlink(&env, "dispatch_link");
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = test_fops_dispatch_unlink(&env, "renamed.txt");
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = test_fops_dispatch_unlink(&env, "base.txt");
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = test_fops_dispatch_unlink(&env, "dispatch_fifo");
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&args, 0, sizeof(args));
    args.op = FS_OP_RMDIR;
    args.parent_fuid = &env.root_fuid;
    args.name = "dispatch_dir";
    args.flags = FS_FLAG_DIRECTORY;
    err = fops_dispatch(&args);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    test_fops_env_teardown(&env);
    return 0;
}

const test_case_t FOPS_DISPATCH_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x3),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x3,
                             0x001),
                  test_fops_dispatch_rejects_bad_request,
                  "FOPS dispatch 入参防御",
                  "向统一入口注入 NULL、非法 op 和缺失必填字段",
                  "dispatch 在分发前返回 FOPS 参数错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x4),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x4,
                             0x001),
                  test_fops_dispatch_file_attr_xattr_round_trip,
                  "FOPS dispatch 文件、属性和 xattr 回环",
                  "通过统一入口执行 "
                  "create/lookup/open/read/write/attr/handle/xattr/fs",
                  "正式 dispatch 路径能驱动真实后端并保持对象状态一致"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x4),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_DISPATCH, 0x4,
                             0x002),
                  test_fops_dispatch_namespace_round_trip,
                  "FOPS dispatch 命名空间回环",
                  "通过统一入口执行 "
                  "mkdir/mknod/link/rename/symlink/readdir/rmdir",
                  "命名空间变更成功，目录项可枚举，退出时临时 root 可清理"),
};

const size_t FOPS_DISPATCH_CASE_COUNT =
        sizeof(FOPS_DISPATCH_CASES) / sizeof(FOPS_DISPATCH_CASES[0]);
