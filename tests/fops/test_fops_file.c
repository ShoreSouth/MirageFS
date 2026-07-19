#include "fops_test_common.h"

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

const test_case_t FOPS_FILE_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_FILE,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_FILE,
                         0x1,
                         0x001),
              test_fops_linux_open_flags_maps_access_and_modifiers,
              "FOPS Linux open flag 转换",
              "构造读写方向和 append/truncate/sync/directory modifier",
              "转换为预期 Linux O_* flag"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_FILE,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_FILE,
                         0x1,
                         0x002),
              test_fops_file_check_rejects_bad_handles_and_access_modes,
              "FOPS file 校验",
              "NULL file、写只读、读写只写句柄",
              "非法句柄返回 EBADF，读写句柄通过")
};

const size_t FOPS_FILE_CASE_COUNT =
        sizeof(FOPS_FILE_CASES) / sizeof(FOPS_FILE_CASES[0]);
