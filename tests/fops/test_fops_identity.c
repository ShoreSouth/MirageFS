#include "fops_test_common.h"

static int test_fops_make_child_fuid_inherits_parent_view(void)
{
    fuid_t parent_fuid;
    fuid_t child_fuid;

    parent_fuid = test_fops_make_fuid(FUID_TYPE_DIR);
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

const test_case_t FOPS_IDENTITY_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_IDENTITY,
                         0x1),
              UT_CASE_NO(UT_MOD_FOPS,
                         TEST_FOPS_COMPONENT_IDENTITY,
                         0x1,
                         0x001),
              test_fops_make_child_fuid_inherits_parent_view,
              "FOPS 子 FUID 构造",
              "从父目录 FUID 派生普通文件 FUID",
              "继承 fsid/view 字段并设置 child identity")
};

const size_t FOPS_IDENTITY_CASE_COUNT =
        sizeof(FOPS_IDENTITY_CASES) / sizeof(FOPS_IDENTITY_CASES[0]);
