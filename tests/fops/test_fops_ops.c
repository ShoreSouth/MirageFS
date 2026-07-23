#include "fops_test_common.h"

#include <stdio.h>
#include <stdlib.h>

static int test_fops_ops_reject_invalid_public_arguments(void)
{
    fs_error_t err;
    fuid_t parent;
    fuid_t out_fuid;
    fops_object_result_t out;
    fops_attr_t attr;
    fops_setattr_t setattr;
    fops_mknod_req_t mknod_req;
    fops_dirent_t entries[1];
    fops_dirent_plus_t plus_entries[1];
    obj_handle_t handle;
    fops_file_t *file;
    uint32_t entry_nr;
    bool eof;
    char buf[8];
    size_t actual;
    fops_statfs_t statfs_buf;

    parent = test_fops_make_fuid(FUID_TYPE_DIR);
    memset(&setattr, 0, sizeof(setattr));
    memset(&mknod_req, 0, sizeof(mknod_req));
    memset(&handle, 0, sizeof(handle));

    err = fops_lookup(NULL, "x", FS_FLAG_NONE, &out_fuid);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_lookup_plus(&parent, "x", FS_FLAG_DIRECTORY | FS_FLAG_REGULAR,
                           &out);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_create(NULL, "x", NULL, FS_FLAG_NONE, &out_fuid);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_create_plus(&parent, "x", NULL,
                           FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE, &out);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_mkdir(NULL, "d", NULL, FS_FLAG_NONE, &out_fuid);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_mkdir_plus(&parent, "d", NULL, FS_FLAG_REGULAR, &out);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_mknod(NULL, &out_fuid);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    mknod_req.parent_fuid = &parent;
    mknod_req.name = "blk";
    mknod_req.type = FS_TYPE_BLK;
    err = fops_mknod_plus(&mknod_req, &out);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_unlink(&parent, ".", FS_FLAG_NONE);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_rmdir(&parent, "d", FS_FLAG_REGULAR);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_rename(NULL, "a", &parent, "b", FS_FLAG_NONE);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EXDEV);

    err = fops_link(&parent, "a", &parent, "b", FS_FLAG_NONE, NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_symlink(&parent, "s", "target", FS_FLAG_NONE, NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_readlink(&parent, "s", FS_FLAG_DIRECTORY, buf, sizeof(buf),
                        &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_getattr(NULL, FS_FLAG_NONE, &attr);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    setattr.valid_mask = FOPS_SETATTR_MODE << 4;
    err = fops_setattr(&parent, &setattr, FS_FLAG_NONE);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_access(NULL, F_OK, FS_FLAG_NONE);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_gethandle(NULL, &handle);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    file = (fops_file_t *)1;
    err = fops_open(NULL, FS_FLAG_READ, &file);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    TEST_ASSERT_TRUE(file == NULL);
    err = fops_openhandle(NULL, FS_FLAG_READ, &file);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    TEST_ASSERT_TRUE(file == NULL);
    err = fops_close(NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EBADF);

    err = fops_read(NULL, buf, sizeof(buf), &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EBADF);
    err = fops_write(NULL, buf, sizeof(buf), &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EBADF);
    err = fops_pread(NULL, buf, sizeof(buf), 0, &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EBADF);
    err = fops_pwrite(NULL, buf, sizeof(buf), 0, &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EBADF);
    err = fops_truncate(&parent, 0U, FS_FLAG_DIRECTORY | FS_FLAG_REGULAR);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_readdir(NULL, FS_FLAG_NONE, entries, 1U, &entry_nr, &eof);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_readdirplus(&parent, FS_FLAG_REGULAR, plus_entries, 1U,
                           &entry_nr, &eof);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    err = fops_getxattr(NULL, "user.k", buf, sizeof(buf), &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_setxattr(NULL, "user.k", "v", 1U, FS_FLAG_NONE);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_listxattr(NULL, buf, sizeof(buf), &actual);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_removexattr(NULL, "user.k");
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_statfs(NULL, &statfs_buf);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);
    err = fops_syncfs(NULL);
    TEST_FOPS_EXPECT_FOPS_ERRNO(err, EINVAL);

    return 0;
}

static int test_fops_ops_create_lookup_rw_readdir_round_trip(void)
{
    test_fops_env_t env;
    fops_create_attr_t create_attr;
    fops_object_result_t created;
    fops_object_result_t looked_up;
    fops_file_t *file;
    fops_attr_t attr;
    fops_dirent_t entries[4];
    uint32_t entry_nr;
    bool eof;
    char read_buf[8];
    size_t actual;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    memset(&create_attr, 0, sizeof(create_attr));
    create_attr.valid_mask = FOPS_CREATE_ATTR_MODE | FOPS_CREATE_ATTR_SIZE;
    create_attr.mode = 0600;
    create_attr.size = 0U;

    err = fops_create_plus(&env.root_fuid, "alpha.txt", &create_attr,
                           FS_FLAG_EXCLUSIVE | FS_FLAG_REGULAR, &created);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_file(&created.fuid));

    err = fops_lookup_plus(&env.root_fuid, "alpha.txt", FS_FLAG_REGULAR,
                           &looked_up);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(looked_up.fuid.objectid, created.fuid.objectid);

    file = NULL;
    err = fops_open(&created.fuid, FS_FLAG_READ | FS_FLAG_WRITE, &file);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(file != NULL);

    err = fops_write(file, "abc", 3U, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3U);
    err = fops_pread(file, read_buf, 3U, 0, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3U);
    read_buf[3] = '\0';
    TEST_ASSERT_STR_EQ(read_buf, "abc");

    err = fops_close(file);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_getattr(&created.fuid, FS_FLAG_REGULAR, &attr);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(attr.type, FS_TYPE_REG);
    TEST_ASSERT_EQ_INT(attr.size, 3U);

    err = fops_readdir(&env.root_fuid, FS_FLAG_DIRECTORY, entries, 4U,
                       &entry_nr, &eof);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(entry_nr >= 1U);

    err = fops_unlink(&env.root_fuid, "alpha.txt", FS_FLAG_REGULAR);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    test_fops_env_teardown(&env);
    return 0;
}

static int test_fops_ops_namespace_mutation_round_trip(void)
{
    test_fops_env_t env;
    fops_object_result_t file_out;
    fops_object_result_t dir_out;
    fuid_t out_fuid;
    char link_buf[64];
    size_t actual;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(test_fops_env_setup(&env), 0);

    err = fops_mkdir_plus(&env.root_fuid, "dir_a", NULL, FS_FLAG_DIRECTORY,
                          &dir_out);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_dir(&dir_out.fuid));

    err = fops_create_plus(&env.root_fuid, "base.txt", NULL, FS_FLAG_EXCLUSIVE,
                           &file_out);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_link(&env.root_fuid, "base.txt", &env.root_fuid, "hard.txt",
                    FS_FLAG_EXCLUSIVE, &out_fuid);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_file(&out_fuid));

    err = fops_rename(&env.root_fuid, "hard.txt", &env.root_fuid, "renamed.txt",
                      FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    err = fops_symlink(&env.root_fuid, "link.txt", "base.txt",
                       FS_FLAG_EXCLUSIVE, &out_fuid);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(fuid_is_symlink(&out_fuid));

    memset(link_buf, 0, sizeof(link_buf));
    err = fops_readlink(&env.root_fuid, "link.txt", FS_FLAG_NONE, link_buf,
                        sizeof(link_buf), &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_STR_EQ(link_buf, "base.txt");
    TEST_ASSERT_EQ_INT(actual, strlen("base.txt"));

    err = fops_unlink(&env.root_fuid, "link.txt", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_unlink(&env.root_fuid, "renamed.txt", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_unlink(&env.root_fuid, "base.txt", FS_FLAG_NONE);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = fops_rmdir(&env.root_fuid, "dir_a", FS_FLAG_DIRECTORY);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    test_fops_env_teardown(&env);
    return 0;
}

const test_case_t FOPS_OPS_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_OPS, 0x1),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_OPS, 0x1, 0x001),
                  test_fops_ops_reject_invalid_public_arguments,
                  "FOPS 操作层公共入参防御",
                  "直接调用各 public op，注入 NULL、非法 flag、缺失输出参数",
                  "所有非法输入在进入底层 syscall 前返回 FOPS 模块错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_OPS, 0x2),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_OPS, 0x2, 0x001),
                  test_fops_ops_create_lookup_rw_readdir_round_trip,
                  "FOPS 文件创建、查找、读写和目录读取回环",
                  "临时 root 后端中创建普通文件并经过 open/write/pread/readdir",
                  "对象 FUID、属性、读写内容和目录项均符合预期"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_OPS, 0x2),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_OPS, 0x2, 0x002),
                  test_fops_ops_namespace_mutation_round_trip,
                  "FOPS namespace 修改回环",
                  "创建目录、文件、硬链接、软链接并执行 rename/unlink/rmdir",
                  "命名空间变更成功且清理后临时 root 可删除"),
};

const size_t FOPS_OPS_CASE_COUNT =
        sizeof(FOPS_OPS_CASES) / sizeof(FOPS_OPS_CASES[0]);
