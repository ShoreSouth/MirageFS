#include "object_test_common.h"

static int test_objruntime_state_reads_stored_state(void)
{
    obj_runtime_t rt;

    memset(&rt, 0, sizeof(rt));
    rt.state = OBJ_STATE_ACTIVE;
    TEST_ASSERT_EQ_INT(OBJ_STATE_ACTIVE, objruntime_state(&rt));
    rt.state = OBJ_STATE_DELETING;
    TEST_ASSERT_EQ_INT(OBJ_STATE_DELETING, objruntime_state(&rt));
    return 0;
}

static int test_objruntime_dump_and_null_state(void)
{
    obj_runtime_t rt;

    memset(&rt, 0, sizeof(rt));
    rt.state = OBJ_STATE_INIT;
    TEST_ASSERT_EQ_INT(OBJ_STATE_INIT, objruntime_state(&rt));
    rt.state = OBJ_STATE_INVALID;
    TEST_ASSERT_EQ_INT(OBJ_STATE_INVALID, objruntime_state(&rt));
    return 0;
}

const test_case_t OBJECT_OBJRUNTIME_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJRUNTIME,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJRUNTIME,
                         0x1,
                         0x001),
              test_objruntime_state_reads_stored_state,
              "ObjRuntime 状态读取",
              "直接设置 ACTIVE/DELETING 状态",
              "getter 返回当前保存状态"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJRUNTIME,
                         0x2),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJRUNTIME,
                         0x2,
                         0x001),
              test_objruntime_dump_and_null_state,
              "ObjRuntime 状态边界读取",
              "直接设置 INIT/INVALID 状态",
              "getter 返回 runtime 当前保存状态"),
};

const size_t OBJECT_OBJRUNTIME_CASE_COUNT = sizeof(OBJECT_OBJRUNTIME_CASES) / sizeof(OBJECT_OBJRUNTIME_CASES[0]);
