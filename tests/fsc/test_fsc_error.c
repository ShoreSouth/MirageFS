#include "fsc_test_common.h"

static int test_fsc_error_encodes_module_sub_errno(void)
{
    fs_error_t err = fsc_error(FSC_SUB_FSID, EINVAL);

    TEST_ASSERT_EQ_INT(FS_SEV_ERROR, fs_err_severity(err));
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(FSC_SUB_FSID, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    TEST_ASSERT_TRUE(fsc_sub_valid(FSC_SUB_FSID));
    TEST_ASSERT_STR_EQ("FSID", fsc_sub_name(FSC_SUB_FSID));
    TEST_ASSERT_FALSE(fsc_sub_valid(FSC_SUB_MAX));
    TEST_ASSERT_STR_EQ("UNKNOWN", fsc_sub_name(FSC_SUB_MAX));
    return 0;
}


const test_case_t FSC_ERROR_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_ERROR,
                             0x1),
                  UT_CASE_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_ERROR,
                             0x1,
                             0x001),
                  test_fsc_error_encodes_module_sub_errno,
                  "FSC 错误码布局",
                  "构造 FSID 子模块错误",
                  "severity/module/sub/errno 字段可正确解析"),
};

const size_t FSC_ERROR_CASE_COUNT = sizeof(FSC_ERROR_CASES) / sizeof(FSC_ERROR_CASES[0]);
