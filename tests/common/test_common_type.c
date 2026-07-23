#include "common_test_common.h"

static int test_type_helpers_convert_modes_and_unknowns(void)
{
    TEST_ASSERT_EQ_INT(FS_TYPE_REG, fs_type_from_mode(S_IFREG | 0644));
    TEST_ASSERT_EQ_INT(FS_TYPE_DIR, fs_type_from_mode(S_IFDIR | 0755));
    TEST_ASSERT_EQ_INT(FS_TYPE_LNK, fs_type_from_mode(S_IFLNK | 0777));
    TEST_ASSERT_EQ_INT(FS_TYPE_FIFO, fs_type_from_mode(S_IFIFO | 0644));
    TEST_ASSERT_EQ_INT(FS_TYPE_SOCK, fs_type_from_mode(S_IFSOCK | 0644));
    TEST_ASSERT_EQ_INT(FS_TYPE_BLK, fs_type_from_mode(S_IFBLK | 0600));
    TEST_ASSERT_EQ_INT(FS_TYPE_CHR, fs_type_from_mode(S_IFCHR | 0600));
    TEST_ASSERT_EQ_INT(FS_TYPE_UNKNOWN, fs_type_from_mode(0));

    TEST_ASSERT_STR_EQ("REG", fs_type_to_str(FS_TYPE_REG));
    TEST_ASSERT_STR_EQ("DIR", fs_type_to_str(FS_TYPE_DIR));
    TEST_ASSERT_STR_EQ("LNK", fs_type_to_str(FS_TYPE_LNK));
    TEST_ASSERT_STR_EQ("FIFO", fs_type_to_str(FS_TYPE_FIFO));
    TEST_ASSERT_STR_EQ("SOCK", fs_type_to_str(FS_TYPE_SOCK));
    TEST_ASSERT_STR_EQ("BLK", fs_type_to_str(FS_TYPE_BLK));
    TEST_ASSERT_STR_EQ("CHR", fs_type_to_str(FS_TYPE_CHR));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_type_to_str(FS_TYPE_UNKNOWN));
    return 0;
}


const test_case_t COMMON_TYPE_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                             TEST_COMMON_COMPONENT_TYPE,
                             0x1),
                  UT_CASE_NO(UT_MOD_COMMON,
                             TEST_COMMON_COMPONENT_TYPE,
                             0x1,
                             0x001),
                  test_type_helpers_convert_modes_and_unknowns,
                  "文件类型 helper",
                  "覆盖 mode 到 fs_type_t 转换和 fs_type_t 到字符串转换",
                  "所有已知类型返回预期值，未知类型返回 UNKNOWN"),
};

const size_t COMMON_TYPE_CASE_COUNT = sizeof(COMMON_TYPE_CASES) / sizeof(COMMON_TYPE_CASES[0]);
