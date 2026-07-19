#include "fops_test_common.h"

static int test_fops_handle_from_lsa_checked_validates_mount_boundary(void)
{
    fs_error_t err;
    obj_meta_t parent_meta;
    obj_handle_t out_handle;
    lsa_file_handle_t lsa_handle;

    memset(&parent_meta, 0, sizeof(parent_meta));
    memset(&lsa_handle, 0, sizeof(lsa_handle));
    parent_meta.handle = test_fops_make_handle(42);
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

const test_case_t FOPS_HANDLE_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_HANDLE,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_HANDLE,
                         0x1,
                         0x001),
              test_fops_handle_from_lsa_checked_validates_mount_boundary,
              "FOPS handle 转换边界",
              "NULL 输出、跨 mount、合法 LSA handle",
              "非法参数返回 EINVAL/EXDEV，合法时完成字段转换")
};

const size_t FOPS_HANDLE_CASE_COUNT =
        sizeof(FOPS_HANDLE_CASES) / sizeof(FOPS_HANDLE_CASES[0]);
