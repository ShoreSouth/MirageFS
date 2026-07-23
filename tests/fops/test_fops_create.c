#include "fops_test_common.h"

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

const test_case_t FOPS_CREATE_CASES[] = {TEST_CASE(
        UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_CREATE, 0x1),
        UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_CREATE, 0x1, 0x001),
        test_fops_create_mode_masks_permissions, "FOPS create mode",
        "传入带特殊位的 mode/default_mode", "按 FS_PERM_MASK 保留权限相关位")};

const size_t FOPS_CREATE_CASE_COUNT =
        sizeof(FOPS_CREATE_CASES) / sizeof(FOPS_CREATE_CASES[0]);
