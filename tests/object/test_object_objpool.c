#include "object_test_common.h"

static int test_objpool_standalone_lifecycle_edges(void)
{
    fs_error_t err;
    obj_runtime_t *rt;

    objpool_deinit();
    TEST_ASSERT_TRUE(objpool_alloc() == NULL);
    objpool_free(NULL);

    err = objpool_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    rt = objpool_alloc();
    TEST_ASSERT_TRUE(rt != NULL);
    objpool_free(rt);
    objpool_deinit();
    objpool_deinit();
    return 0;
}

const test_case_t OBJECT_OBJPOOL_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJPOOL,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJPOOL,
                         0x1,
                         0x001),
              test_objpool_standalone_lifecycle_edges,
              "ObjPool 独立生命周期",
              "未初始化申请、NULL free、初始化后申请释放和重复 deinit",
              "未初始化申请失败，释放和重复销毁安全"),
};

const size_t OBJECT_OBJPOOL_CASE_COUNT = sizeof(OBJECT_OBJPOOL_CASES) / sizeof(OBJECT_OBJPOOL_CASES[0]);
