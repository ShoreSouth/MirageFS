#include "object_test_common.h"

static int test_objkey_from_fuid_round_trip(void)
{
    fuid_t fuid = fuid_make(9, 100, 5, FUID_TYPE_DIR);
    obj_key_t key;
    obj_key_t expected = objkey_make(100, 5);

    /* ObjKey 是对象表索引键，只从 FUID 的 objectid/gen 投影身份。 */
    objkey_from_fuid(&key, &fuid);

    TEST_ASSERT_TRUE(objkey_is_valid(&key));
    TEST_ASSERT_TRUE(objkey_equal(&key, &expected));
    TEST_ASSERT_FALSE(objkey_equal(&key, NULL));
    TEST_ASSERT_TRUE(objkey_hash(&key) != 0);
    return 0;
}

const test_case_t OBJECT_OBJKEY_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_OBJKEY, 0x1),
                  UT_CASE_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_OBJKEY, 0x1,
                             0x001),
                  test_objkey_from_fuid_round_trip, "ObjKey 转换",
                  "从目录 FUID 提取 objectid/gen，构造预期 ObjKey 对比",
                  "ObjKey 有效、可比较、hash 非零，NULL 比较安全失败"),
};

const size_t OBJECT_OBJKEY_CASE_COUNT =
        sizeof(OBJECT_OBJKEY_CASES) / sizeof(OBJECT_OBJKEY_CASES[0]);
