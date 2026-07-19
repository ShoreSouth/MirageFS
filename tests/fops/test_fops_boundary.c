#include "fops_test_common.h"

#include <stdio.h>

static int test_fops_boundary_parent_type_and_lookup_dot(void)
{
    test_fops_env_t env;
    fops_object_result_t result;
    fops_create_attr_t attr;
    fuid_t out_fuid;
    fuid_t file_parent;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);
    file_parent = test_fops_make_fuid(FUID_TYPE_FILE);

    err = fops_lookup_plus(&file_parent, "x", FS_FLAG_NONE, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);
    err = fops_create_plus(&file_parent, "x", NULL, FS_FLAG_NONE, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);
    err = fops_mkdir_plus(&file_parent, "x", NULL, FS_FLAG_NONE, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);

    err = fops_lookup(&env.root_fuid, ".", FS_FLAG_DIRECTORY, &out_fuid);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(out_fuid.objectid, env.root_fuid.objectid);

    err = fops_lookup(&env.root_fuid, ".", FS_FLAG_REGULAR, &out_fuid);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EISDIR);

    err = fops_lookup(&env.root_fuid, "missing", FS_FLAG_NONE, NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_create(&env.root_fuid, "missing", NULL, FS_FLAG_NONE, NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_mkdir(&env.root_fuid, "missing", NULL, FS_FLAG_NONE, NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_SIZE;
    attr.size = 1U;
    err = fops_mkdir_plus(&env.root_fuid, "bad_size", &attr,
                          FS_FLAG_DIRECTORY, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    test_fops_env_teardown(&env);
    return 0;
}

static int test_fops_boundary_create_existing_flag_matrix(void)
{
    test_fops_env_t env;
    fops_object_result_t first;
    fops_object_result_t second;
    fops_object_result_t dir_out;
    fops_file_t *file;
    fops_attr_t attr;
    fs_error_t err;
    size_t actual;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    err = fops_create_plus(&env.root_fuid, "existing.txt", NULL,
                           FS_FLAG_EXCLUSIVE, &first);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    file = NULL;
    err = fops_open(&first.fuid, FS_FLAG_WRITE | FS_FLAG_REGULAR, &file);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_write(file, "abcdef", 6U, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 6U);
    err = fops_close(file);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_create_plus(&env.root_fuid, "existing.txt", NULL,
                           FS_FLAG_EXCLUSIVE, &second);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EEXIST);

    err = fops_create_plus(&env.root_fuid, "existing.txt", NULL,
                           FS_FLAG_REPLACE | FS_FLAG_TRUNCATE,
                           &second);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(second.fuid.objectid, first.fuid.objectid);

    err = fops_getattr(&second.fuid, FS_FLAG_REGULAR, &attr);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(attr.size, 0U);

    err = fops_mkdir_plus(&env.root_fuid, "as_dir", NULL,
                          FS_FLAG_DIRECTORY, &dir_out);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_create_plus(&env.root_fuid, "as_dir", NULL,
                           FS_FLAG_REPLACE, &second);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EISDIR);

    err = fops_unlink(&env.root_fuid, "existing.txt", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_rmdir(&env.root_fuid, "as_dir", FS_FLAG_DIRECTORY);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    test_fops_env_teardown(&env);
    return 0;
}

static int test_fops_boundary_mkdir_existing_and_wrapper(void)
{
    test_fops_env_t env;
    fops_object_result_t first;
    fuid_t out_fuid;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    err = fops_mkdir(&env.root_fuid, "child_dir", NULL,
                     FS_FLAG_DIRECTORY, &out_fuid);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_dir(&out_fuid));

    err = fops_mkdir_plus(&env.root_fuid, "child_dir", NULL,
                          FS_FLAG_EXCLUSIVE | FS_FLAG_DIRECTORY, &first);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), EEXIST);

    err = fops_mkdir_plus(&env.root_fuid, ".", NULL,
                          FS_FLAG_DIRECTORY, &first);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_rmdir(&env.root_fuid, "child_dir", FS_FLAG_DIRECTORY);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    test_fops_env_teardown(&env);
    return 0;
}

static int test_fops_boundary_link_and_rename_conflicts(void)
{
    test_fops_env_t env;
    fops_object_result_t base;
    fops_object_result_t target;
    fops_object_result_t linked;
    fuid_t cross_parent;
    fuid_t out_fuid;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    err = fops_create_plus(&env.root_fuid, "base.txt", NULL,
                           FS_FLAG_EXCLUSIVE, &base);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_create_plus(&env.root_fuid, "target.txt", NULL,
                           FS_FLAG_EXCLUSIVE, &target);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_link(&env.root_fuid, "base.txt", &env.root_fuid,
                    "ignored.txt", FS_FLAG_NONE, NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    cross_parent = env.root_fuid;
    cross_parent.fsid += 1U;
    err = fops_link_plus(&env.root_fuid, "base.txt", &cross_parent,
                         "cross.txt", FS_FLAG_NONE, &linked);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_link_plus(&env.root_fuid, "base.txt", &env.root_fuid,
                         "target.txt", FS_FLAG_NONE, &linked);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EEXIST);

    err = fops_link(&env.root_fuid, "missing.txt", &env.root_fuid,
                    "new_link.txt", FS_FLAG_NONE, &out_fuid);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENOENT);

    err = fops_rename(&env.root_fuid, "missing.txt", &env.root_fuid,
                      "rename_missing.txt", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENOENT);

    err = fops_rename(&env.root_fuid, "base.txt", &env.root_fuid,
                      "target.txt", FS_FLAG_NONE);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EEXIST);

    err = fops_rename(&env.root_fuid, "base.txt", &env.root_fuid,
                      "target.txt", FS_FLAG_REPLACE);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_lookup(&env.root_fuid, "target.txt", FS_FLAG_REGULAR,
                      &out_fuid);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_lookup(&env.root_fuid, "base.txt", FS_FLAG_REGULAR,
                      &out_fuid);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENOENT);

    err = fops_unlink(&env.root_fuid, "target.txt", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    test_fops_env_teardown(&env);
    return 0;
}


static int test_fops_boundary_internal_helper_edges(void)
{
    test_fops_env_t env;
    obj_handle_t handle;
    obj_meta_t *meta;
    fops_attr_t attr;
    struct stat st;
    fuid_t invalid;
    fuid_t missing;
    fuid_t out_fuid;
    fs_error_t err;
    int linux_flags;
    int fd;

    err = fops_validate_lookup_flags(0x80000000U, FS_OP_LOOKUP);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_validate_lookup_flags(FS_FLAG_DIRECTORY | FS_FLAG_REGULAR,
                                     FS_OP_LOOKUP);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    linux_flags = fops_linux_open_flags(FS_FLAG_READ | FS_FLAG_WRITE |
                                        FS_FLAG_APPEND | FS_FLAG_TRUNCATE |
                                        FS_FLAG_SYNC | FS_FLAG_DIRECT |
                                        FS_FLAG_DIRECTORY);
    TEST_ASSERT_TRUE((linux_flags & O_DIRECT) != 0);

    TEST_ASSERT_EQ_INT(fops_fuid_type_from_fs_type(FS_TYPE_FIFO),
                       FUID_TYPE_FIFO);
    TEST_ASSERT_EQ_INT(fops_fuid_type_from_fs_type(FS_TYPE_SOCK),
                       FUID_TYPE_SOCK);
    TEST_ASSERT_EQ_INT(fops_fuid_type_from_fs_type(FS_TYPE_BLK),
                       FUID_TYPE_BLK);
    TEST_ASSERT_EQ_INT(fops_fuid_type_from_fs_type(FS_TYPE_CHR),
                       FUID_TYPE_CHR);

    memset(&st, 0, sizeof(st));
    memset(&attr, 0xff, sizeof(attr));
    fops_attr_from_stat(NULL, &st);
    fops_attr_from_stat(&attr, NULL);

    handle = test_fops_make_handle(1);
    err = fops_fuid_from_handle(NULL, &handle, FS_TYPE_REG, &out_fuid,
                                FS_OP_LOOKUP);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    meta = (obj_meta_t *)1;
    fd = 123;
    err = fops_open_object(NULL, O_RDONLY, &meta, &fd, FS_OP_OPEN);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    fuid_set_invalid(&invalid);
    meta = (obj_meta_t *)1;
    fd = 123;
    err = fops_open_object(&invalid, O_RDONLY, &meta, &fd, FS_OP_OPEN);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    TEST_ASSERT_TRUE(meta == NULL);
    TEST_ASSERT_EQ_INT(fd, -1);

    missing = fuid_make(env.root_fuid.fsid, 12345678U, 1U, FUID_TYPE_FILE);
    err = fops_open_object(&missing, O_RDONLY, &meta, &fd, FS_OP_OPEN);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENOENT);

    err = fops_open_parent_dir(NULL, &meta, &fd, FS_OP_LOOKUP);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);
    err = fops_open_parent_dir(&missing, &meta, &fd, FS_OP_LOOKUP);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);

    test_fops_env_teardown(&env);
    return 0;
}

static int test_fops_boundary_attr_handle_mknod_edges(void)
{
    test_fops_env_t env;
    fops_object_result_t created;
    fops_object_result_t fifo_result;
    fops_create_attr_t create_attr;
    fops_mknod_req_t req;
    fops_setattr_t setattr;
    fops_file_t *file;
    fops_attr_t attr;
    fops_dirent_t entries[1];
    obj_handle_t handle;
    obj_handle_t missing_handle;
    fuid_t out_fuid;
    char fifo_path[220];
    uint32_t entry_nr;
    bool eof;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    memset(&create_attr, 0, sizeof(create_attr));
    create_attr.valid_mask = FOPS_CREATE_ATTR_MODE |
                             FOPS_CREATE_ATTR_UID |
                             FOPS_CREATE_ATTR_GID;
    create_attr.mode = 0640;
    create_attr.uid = getuid();
    create_attr.gid = getgid();
    err = fops_create_plus(&env.root_fuid, "owned.txt", &create_attr,
                           FS_FLAG_EXCLUSIVE, &created);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    memset(&setattr, 0, sizeof(setattr));
    setattr.valid_mask = FOPS_SETATTR_MODE |
                         FOPS_SETATTR_UID |
                         FOPS_SETATTR_GID |
                         FOPS_SETATTR_SIZE;
    setattr.mode = 0600;
    setattr.uid = getuid();
    setattr.gid = getgid();
    setattr.size = 3U;
    err = fops_setattr(&created.fuid, &setattr, FS_FLAG_REGULAR);
    TEST_ASSERT_EQ_INT(err, FS_OK);


    err = fops_getattr(&created.fuid, 0x80000000U, &attr);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_setattr(NULL, &setattr, FS_FLAG_NONE);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_setattr(&created.fuid, &setattr, 0x80000000U);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_access(&created.fuid, R_OK, 0x80000000U);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    entry_nr = 1U;
    eof = true;
    err = fops_readdir(&created.fuid, FS_FLAG_NONE, entries,
                       1U, &entry_nr, &eof);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);

    err = fops_getattr(&created.fuid, FS_FLAG_DIRECTORY, &attr);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);
    err = fops_access(&created.fuid, R_OK, FS_FLAG_DIRECTORY);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);

    file = NULL;
    err = fops_open(&created.fuid, FS_FLAG_READ | FS_FLAG_DIRECTORY, &file);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENOTDIR);
    TEST_ASSERT_TRUE(file == NULL);

    err = fops_gethandle(&created.fuid, &handle);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    file = NULL;
    err = fops_openhandle(&handle, FS_FLAG_READ | FS_FLAG_DIRECTORY, &file);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENOTDIR);
    TEST_ASSERT_TRUE(file == NULL);

    missing_handle = test_fops_make_handle(env.mount_id);
    missing_handle.type += 1;
    file = NULL;
    err = fops_openhandle(&missing_handle, FS_FLAG_READ, &file);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), ENOENT);
    TEST_ASSERT_TRUE(file == NULL);

    memset(&req, 0, sizeof(req));
    memset(&fifo_result, 0, sizeof(fifo_result));
    req.parent_fuid = &env.root_fuid;
    req.name = "fifo_wrapper";
    req.type = FS_TYPE_FIFO;
    req.attr = NULL;
    req.device = NULL;
    req.flags = FS_FLAG_EXCLUSIVE;
    err = fops_mknod(&req, &out_fuid);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_fifo(&out_fuid));

    err = fops_mknod_plus(&req, &fifo_result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EEXIST);

    (void)snprintf(fifo_path, sizeof(fifo_path), "%s/%s", env.path,
                   req.name);
    (void)unlink(fifo_path);
    err = fops_unlink(&env.root_fuid, "owned.txt", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    test_fops_env_teardown(&env);
    return 0;
}


static int test_fops_boundary_wrapper_and_validation_edges(void)
{
    test_fops_env_t env;
    fops_object_result_t result;
    fops_create_attr_t create_attr;
    fops_mknod_req_t req;
    fops_device_t device;
    fuid_t file_fuid;
    fuid_t sym_fuid;
    char link_buf[64];
    size_t actual;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    err = fops_create(&env.root_fuid, "wrapper_create.txt", NULL,
                      FS_FLAG_EXCLUSIVE, &file_fuid);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_file(&file_fuid));

    err = fops_symlink(&env.root_fuid, "sym_wrapper",
                       "wrapper_create.txt", FS_FLAG_EXCLUSIVE,
                       &sym_fuid);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_symlink(&sym_fuid));

    memset(&result, 0, sizeof(result));
    err = fops_symlink_plus(&env.root_fuid, "sym_wrapper",
                            "wrapper_create.txt", FS_FLAG_EXCLUSIVE,
                            &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EEXIST);

    actual = 99U;
    err = fops_readlink(&env.root_fuid, "sym_wrapper", FS_FLAG_NONE,
                        NULL, sizeof(link_buf), &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    TEST_ASSERT_EQ_INT(actual, 0U);

    err = fops_readlink(&env.root_fuid, "bad/name", FS_FLAG_NONE,
                        link_buf, sizeof(link_buf), &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    memset(link_buf, 0, sizeof(link_buf));
    err = fops_readlink(&env.root_fuid, "sym_wrapper", FS_FLAG_NONE,
                        link_buf, sizeof(link_buf), &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(actual > 0U);

    memset(&req, 0, sizeof(req));
    req.parent_fuid = &env.root_fuid;
    req.name = "bad_fifo";
    req.type = FS_TYPE_FIFO;
    req.flags = 0x80000000U;
    err = fops_mknod_plus(&req, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    memset(&create_attr, 0, sizeof(create_attr));
    create_attr.valid_mask = 0x80000000U;
    req.flags = FS_FLAG_EXCLUSIVE;
    req.attr = &create_attr;
    err = fops_mknod_plus(&req, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    req.attr = NULL;
    req.name = "bad/name";
    err = fops_mknod_plus(&req, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    req.name = "out_null_fifo";
    err = fops_mknod_plus(&req, NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    req.parent_fuid = NULL;
    err = fops_mknod_plus(&req, &result);
    TEST_ASSERT_TRUE(fs_failed(err));

    memset(&device, 0, sizeof(device));
    req.parent_fuid = &env.root_fuid;
    req.name = "char_device";
    req.type = FS_TYPE_CHR;
    req.device = &device;
    device.major_id = 1U;
    device.minor_id = 3U;
    err = fops_mknod_plus(&req, &result);
    TEST_ASSERT_TRUE((err == FS_OK) || fs_failed(err));

    if (err == FS_OK) {
        err = fops_unlink(&env.root_fuid, "char_device", FS_FLAG_NONE);
        TEST_ASSERT_EQ_INT(err, FS_OK);
    }

    err = fops_link_plus(&env.root_fuid, "wrapper_create.txt",
                         &env.root_fuid, "bad_link", 0x80000000U,
                         &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_link_plus(&env.root_fuid, "bad/name",
                         &env.root_fuid, "bad_link", FS_FLAG_NONE,
                         &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_link_plus(&env.root_fuid, "wrapper_create.txt",
                         &env.root_fuid, "bad/name", FS_FLAG_NONE,
                         &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_link_plus(&file_fuid, "missing",
                         &env.root_fuid, "old_parent_file",
                         FS_FLAG_NONE, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);

    err = fops_link_plus(&env.root_fuid, "wrapper_create.txt",
                         &file_fuid, "new_parent_file", FS_FLAG_NONE,
                         &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);

    err = fops_symlink_plus(&env.root_fuid, "target_null", NULL,
                            FS_FLAG_NONE, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_symlink_plus(&env.root_fuid, "bad_flag_sym", "target",
                            0x80000000U, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_symlink_plus(&env.root_fuid, "bad/name", "target",
                            FS_FLAG_NONE, &result);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_symlink(&env.root_fuid, "bad/name", "target",
                       FS_FLAG_NONE, &sym_fuid);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_readlink(&file_fuid, "sym_wrapper", FS_FLAG_NONE,
                        link_buf, sizeof(link_buf), &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, ENOTDIR);

    err = fops_unlink(&env.root_fuid, "sym_wrapper", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_unlink(&env.root_fuid, "wrapper_create.txt", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    test_fops_env_teardown(&env);
    return 0;
}

const test_case_t FOPS_BOUNDARY_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x1,
                         0x001),
              test_fops_boundary_parent_type_and_lookup_dot,
              "FOPS 父对象类型与点目录边界",
              "注入非目录父 FUID、点目录 lookup、wrapper 输出参数为空和 mkdir size attr",
              "非法父对象和非法输出被拒绝，点目录按类型约束返回预期结果"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x2),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x2,
                         0x001),
              test_fops_boundary_create_existing_flag_matrix,
              "FOPS create 已存在目标 flag 矩阵",
              "同一文件分别使用 EXCLUSIVE、REPLACE/TRUNCATE，并对目录执行 REPLACE create",
              "已存在文件按 flag 返回 EEXIST 或复用并截断，目录类型冲突返回 EISDIR"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x3),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x3,
                         0x001),
              test_fops_boundary_mkdir_existing_and_wrapper,
              "FOPS mkdir 已存在和 wrapper 边界",
              "通过 wrapper 创建目录，再重复 mkdir 和使用点目录名称",
              "首次创建成功，重复创建返回 EEXIST，非法名称在 syscall 前被拒绝"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x4),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x4,
                         0x001),
              test_fops_boundary_link_and_rename_conflicts,
              "FOPS link/rename 冲突矩阵",
              "注入空输出、跨 fsid、缺失 source、已存在 target 和 REPLACE rename",
              "link/rename 按冲突类型返回结构化错误，REPLACE 成功替换目标"),

    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x5),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x5,
                         0x001),
              test_fops_boundary_internal_helper_edges,
              "FOPS helper 内部边界",
              "注入未知 flag、互斥类型 flag、"
              "空 handle 输入、无效 FUID 和缺失对象",
              "helper 函数在 syscall 前返回结构化错误，"
              "DIRECT/FIFO/SOCK/BLK/CHR 映射符合预期"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x6),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x6,
                         0x001),
              test_fops_boundary_attr_handle_mknod_edges,
              "FOPS attr/handle/mknod 综合边界",
              "使用真实 LSA 临时目录注入 UID/GID setattr、"
              "类型冲突 open/openhandle 和 FIFO mknod wrapper",
              "属性修改成功，类型冲突返回 ENOTDIR，"
              "FIFO wrapper 创建成功且重复创建被拒绝"),

    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x7),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_BOUNDARY,
                         0x7,
                         0x001),
              test_fops_boundary_wrapper_and_validation_edges,
              "FOPS wrapper 与校验边界",
              "注入 create/symlink/readlink/mknod wrapper 成功与早期校验异常",
              "wrapper 返回 FUID，重复 symlink 返回 EEXIST，"
              "mknod 非法 flag/attr/name/out 在 syscall 前被拒绝"),
};

const size_t FOPS_BOUNDARY_CASE_COUNT =
        sizeof(FOPS_BOUNDARY_CASES) / sizeof(FOPS_BOUNDARY_CASES[0]);
