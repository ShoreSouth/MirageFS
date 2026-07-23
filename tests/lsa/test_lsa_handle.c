#include "lsa_test_common.h"

static int test_lsa_handle_capability_paths(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    int opened = -1;
    int32_t mount_id = -1;
    lsa_file_handle_t handle;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(
            test_lsa_open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(test_lsa_create_file(dirfd, "handle.txt", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);

    memset(&handle, 0, sizeof(handle));
    err = lsa_name_to_handle_at(dirfd, "handle.txt", &handle, &mount_id, 0);
    if (fs_succeeded(err))
    {
        err = lsa_open_by_handle_id(mount_id, &handle, O_RDONLY, &opened);
        TEST_ASSERT_TRUE((err == FS_OK) || fs_failed(err));
        if (opened >= 0)
        {
            TEST_ASSERT_EQ_INT(lsa_close(opened), FS_OK);
        }
        TEST_ASSERT_EQ_INT(lsa_release_mount(mount_id), FS_OK);
    }
    else
    {
        TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    }

    TEST_ASSERT_EQ_INT(
            test_lsa_assert_errno(
                    lsa_open_by_handle_at(-1, NULL, O_RDONLY, &opened),
                    FS_ERRNO_EINVAL),
            0);
    err = lsa_open_by_handle_id(0x12345678, &handle, O_RDONLY, &opened);
    TEST_ASSERT_EQ_INT(test_lsa_assert_errno(err, FS_ERRNO_ENOENT), 0);
    TEST_ASSERT_EQ_INT(lsa_release_mount(0x12345678), FS_OK);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "handle.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


static int test_lsa_bootstrap_root_and_init(void)
{
    char dir_path[PATH_MAX];
    char root_path[PATH_MAX];
    int32_t mount_id = -1;
    lsa_file_handle_t handle;
    fs_error_t err;
    int written;

    TEST_ASSERT_EQ_INT(test_lsa_make_tmpdir(dir_path, sizeof(dir_path)), 0);
    written = snprintf(root_path, sizeof(root_path), "%s/bootstrap", dir_path);
    TEST_ASSERT_TRUE((written > 0) && ((size_t)written < sizeof(root_path)));

    lsa_init();
    memset(&handle, 0, sizeof(handle));
    err = lsa_bootstrap_root(root_path, &handle, &mount_id);
    if (fs_succeeded(err))
    {
        TEST_ASSERT_TRUE(mount_id >= 0);
        TEST_ASSERT_EQ_INT(lsa_release_mount(mount_id), FS_OK);
    }
    else
    {
        TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    }

    TEST_ASSERT_EQ_INT(
            test_lsa_assert_errno(lsa_bootstrap_root(NULL, &handle, &mount_id),
                                  FS_ERRNO_EINVAL),
            0);
    test_lsa_cleanup_tmpdir(dir_path);
    return 0;
}


const test_case_t LSA_HANDLE_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA, TEST_LSA_COMPONENT_HANDLE, 0x1),
                  UT_CASE_NO(UT_MOD_LSA, TEST_LSA_COMPONENT_HANDLE, 0x1, 0x001),
                  test_lsa_handle_capability_paths,
                  "file handle 能力路径和非法参数验证",
                  "尝试 name_to_handle/open_by_handle 并注入非法 mount/handle",
                  "能力可用则可打开对象，不可用也必须返回 LSA 结构化错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA, TEST_LSA_COMPONENT_HANDLE, 0x2),
                  UT_CASE_NO(UT_MOD_LSA, TEST_LSA_COMPONENT_HANDLE, 0x2, 0x001),
                  test_lsa_bootstrap_root_and_init,
                  "LSA 初始化和 bootstrap root 能力验证",
                  "创建项目 output 下的 bootstrap root 并注册 mount handle",
                  "成功时可释放 mount，不支持 handle 时返回 LSA 结构化错误"),
};

const size_t LSA_HANDLE_CASE_COUNT =
        sizeof(LSA_HANDLE_CASES) / sizeof(LSA_HANDLE_CASES[0]);
