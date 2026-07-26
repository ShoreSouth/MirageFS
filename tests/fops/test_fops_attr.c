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
    st.st_blocks = 8;
    st.st_blksize = 4096;
    st.st_nlink = 2U;
    st.st_atim.tv_sec = 10;
    st.st_atim.tv_nsec = 11;
    st.st_mtim.tv_sec = 20;
    st.st_mtim.tv_nsec = 21;
    st.st_ctim.tv_sec = 30;
    st.st_ctim.tv_nsec = 31;

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
    TEST_ASSERT_EQ_INT(attr.blocks, 8U);
    TEST_ASSERT_EQ_INT(attr.block_size, 4096U);
    TEST_ASSERT_EQ_INT(attr.nlink, 2U);
    TEST_ASSERT_EQ_INT(attr.atime_sec, 10U);
    TEST_ASSERT_EQ_INT(attr.atime_nsec, 11U);
    TEST_ASSERT_EQ_INT(attr.mtime_sec, 20U);
    TEST_ASSERT_EQ_INT(attr.mtime_nsec, 21U);
    TEST_ASSERT_EQ_INT(attr.ctime_sec, 30U);
    TEST_ASSERT_EQ_INT(attr.ctime_nsec, 31U);
    TEST_ASSERT_FALSE(attr.btime_valid);

    fops_attr_from_stat(NULL, &st);
    fops_attr_from_stat(&attr, NULL);
    return 0;
}

static int test_fops_attr_helper_converts_linux_statx_birth_time(void)
{
    struct statx stx;
    fops_attr_t attr;

    memset(&stx, 0, sizeof(stx));
    stx.stx_mask = STATX_BASIC_STATS | STATX_BTIME;
    stx.stx_mode = S_IFREG | 0640;
    stx.stx_uid = 123U;
    stx.stx_gid = 456U;
    stx.stx_size = 789U;
    stx.stx_blocks = 8U;
    stx.stx_blksize = 4096U;
    stx.stx_nlink = 2U;
    stx.stx_atime.tv_sec = 10;
    stx.stx_atime.tv_nsec = 11U;
    stx.stx_mtime.tv_sec = 20;
    stx.stx_mtime.tv_nsec = 21U;
    stx.stx_ctime.tv_sec = 30;
    stx.stx_ctime.tv_nsec = 31U;
    stx.stx_btime.tv_sec = 40;
    stx.stx_btime.tv_nsec = 41U;

    fops_attr_from_statx(&attr, &stx);

    TEST_ASSERT_EQ_INT(attr.type, FS_TYPE_REG);
    TEST_ASSERT_EQ_INT(attr.mode, (S_IFREG | 0640));
    TEST_ASSERT_EQ_INT(attr.blocks, 8U);
    TEST_ASSERT_EQ_INT(attr.block_size, 4096U);
    TEST_ASSERT_EQ_INT(attr.btime_sec, 40U);
    TEST_ASSERT_EQ_INT(attr.btime_nsec, 41U);
    TEST_ASSERT_TRUE(attr.btime_valid);

    fops_attr_from_statx(NULL, &stx);
    fops_attr_from_statx(&attr, NULL);
    return 0;
}

const test_case_t FOPS_ATTR_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_ATTR, 0x1),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_ATTR, 0x1, 0x001),
                  test_fops_type_and_attr_helpers_convert_linux_stat,
                  "FOPS stat 属性转换", "构造 Linux struct stat",
                  "type/mode/uid/gid/size/time 字段正确映射"),
        TEST_CASE(UT_LIST_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_ATTR, 0x2),
                  UT_CASE_NO(UT_MOD_FOPS, TEST_FOPS_COMPONENT_ATTR, 0x2, 0x001),
                  test_fops_attr_helper_converts_linux_statx_birth_time,
                  "FOPS statx 扩展属性转换",
                  "构造包含 STATX_BTIME 和纳秒时间的 Linux statx",
                  "块信息、纳秒时间和 birth time 有效位正确映射")};

const size_t FOPS_ATTR_CASE_COUNT =
        sizeof(FOPS_ATTR_CASES) / sizeof(FOPS_ATTR_CASES[0]);
