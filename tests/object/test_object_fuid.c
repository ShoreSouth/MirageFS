#include "object_test_common.h"

static int test_fuid_make_sets_identity_and_type(void)
{
    fuid_t fuid = fuid_make(7, 42, 3, FUID_TYPE_FILE);

    /* make 同时填充身份字段和类型字段，版本应固定为当前 FUID 版本。 */
    TEST_ASSERT_TRUE(fuid_is_valid(&fuid));
    TEST_ASSERT_TRUE(fuid_is_file(&fuid));
    TEST_ASSERT_EQ_INT(7, fuid.fsid);
    TEST_ASSERT_EQ_INT(42, fuid.objectid);
    TEST_ASSERT_EQ_INT(3, fuid.gen);
    TEST_ASSERT_EQ_INT(FUID_CURRENT_VERSION, fuid.version);
    TEST_ASSERT_STR_EQ("file", fuid_type_str(FUID_TYPE_FILE));
    return 0;
}

static int test_fuid_invalid_and_flags(void)
{
    fuid_t fuid = fuid_make(1, 2, 1, FUID_TYPE_DIR);

    /* flag helper 需要覆盖置位、清位和 NULL 输入的安全空操作。 */
    TEST_ASSERT_FALSE(fuid_flag_test(&fuid, FUID_FLAG_COMPRESSED));
    fuid_flag_set(&fuid, FUID_FLAG_COMPRESSED);
    TEST_ASSERT_TRUE(fuid_flag_test(&fuid, FUID_FLAG_COMPRESSED));
    fuid_flag_clear(&fuid, FUID_FLAG_COMPRESSED);
    TEST_ASSERT_FALSE(fuid_flag_test(&fuid, FUID_FLAG_COMPRESSED));

    fuid_set_invalid(&fuid);
    TEST_ASSERT_FALSE(fuid_is_valid(&fuid));
    TEST_ASSERT_FALSE(fuid_is_valid(NULL));
    TEST_ASSERT_EQ_INT(FUID_TYPE_INVALID, fuid_get_type(NULL));
    return 0;
}

static int test_fuid_types_hash_and_debug_helpers(void)
{
    fuid_t file = fuid_make(1, 10, 1, FUID_TYPE_FILE);
    fuid_t dir = fuid_make(1, 11, 1, FUID_TYPE_DIR);
    fuid_t symlink = fuid_make(1, 12, 1, FUID_TYPE_SYMLINK);
    fuid_t fifo = fuid_make(1, 13, 1, FUID_TYPE_FIFO);
    fuid_t sock = fuid_make(1, 14, 1, FUID_TYPE_SOCK);
    fuid_t blk = fuid_make(1, 15, 1, FUID_TYPE_BLK);
    fuid_t chr = fuid_make(1, 16, 1, FUID_TYPE_CHR);
    fuid_t same_identity = fuid_make(1, 10, 1, FUID_TYPE_DIR);
    fuid_t zeroed;

    /* FUID 的身份字段不包含类型；类型用于语义判断，不改变对象身份。 */
    fuid_init(&zeroed);
    TEST_ASSERT_FALSE(fuid_is_valid(&zeroed));
    TEST_ASSERT_TRUE(fuid_is_valid(&file));
    TEST_ASSERT_TRUE(fuid_is_dir(&dir));
    TEST_ASSERT_TRUE(fuid_is_symlink(&symlink));
    TEST_ASSERT_TRUE(fuid_is_fifo(&fifo));
    TEST_ASSERT_TRUE(fuid_is_sock(&sock));
    TEST_ASSERT_TRUE(fuid_is_blk(&blk));
    TEST_ASSERT_TRUE(fuid_is_chr(&chr));
    TEST_ASSERT_FALSE(fuid_type_valid(FUID_TYPE_INVALID));
    TEST_ASSERT_STR_EQ("invalid", fuid_type_str(FUID_TYPE_INVALID));
    TEST_ASSERT_TRUE(fuid_equal(&file, &same_identity));
    TEST_ASSERT_FALSE(fuid_equal(&file, &dir));
    TEST_ASSERT_EQ_INT(0, fuid_hash(NULL));
    TEST_ASSERT_STR_EQ("null", fuid_to_str(NULL));
    TEST_ASSERT_TRUE(strstr(fuid_to_str(&file), "type=file") != NULL);
    fuid_flag_set(NULL, FUID_FLAG_COMPRESSED);
    fuid_flag_clear(NULL, FUID_FLAG_COMPRESSED);
    TEST_ASSERT_FALSE(fuid_flag_test(NULL, FUID_FLAG_COMPRESSED));
    return 0;
}

const test_case_t OBJECT_FUID_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_FUID, 0x1),
                  UT_CASE_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_FUID, 0x1,
                             0x001),
                  test_fuid_make_sets_identity_and_type, "FUID 构造",
                  "构造合法文件 FUID，并读取 fsid/objectid/gen/version/type",
                  "身份字段原样保存，版本为当前版本，文件类型判断正确"),
        TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_FUID, 0x1),
                  UT_CASE_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_FUID, 0x1,
                             0x002),
                  test_fuid_invalid_and_flags, "FUID 边界和 flags",
                  "对目录 FUID 设置/清除 compressed flag，并传入 NULL/invalid",
                  "flag 状态按位变化，invalid FUID 被拒绝，NULL 输入安全返回"),
        TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_FUID, 0x2),
                  UT_CASE_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_FUID, 0x2,
                             0x001),
                  test_fuid_types_hash_and_debug_helpers,
                  "FUID 类型和调试 helper",
                  "构造全部合法类型、NULL 输入和同身份不同类型",
                  "类型 helper/hash/debug 字符串和安全空操作符合预期"),
};

const size_t OBJECT_FUID_CASE_COUNT =
        sizeof(OBJECT_FUID_CASES) / sizeof(OBJECT_FUID_CASES[0]);
