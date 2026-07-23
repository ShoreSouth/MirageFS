#include "fops_test_common.h"

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
    TEST_ASSERT_EQ_INT(fops_fuid_type_from_fs_type(FS_TYPE_LNK),
                       FUID_TYPE_SYMLINK);
    TEST_ASSERT_EQ_INT(fops_fuid_type_from_fs_type(FS_TYPE_UNKNOWN),
                       FUID_TYPE_INVALID);

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

const test_case_t FOPS_ATTR_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_ATTR, 0x1),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_ATTR, 0x1, 0x001),
                  test_fops_type_and_attr_helpers_convert_linux_stat,
                  "FOPS stat 属性转换", "构造 Linux struct stat",
                  "type/mode/uid/gid/size/time 字段正确映射")};

const size_t FOPS_ATTR_CASE_COUNT =
        sizeof(FOPS_ATTR_CASES) / sizeof(FOPS_ATTR_CASES[0]);
