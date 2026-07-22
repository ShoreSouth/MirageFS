#include "object_test_common.h"

static int test_objmgr_key_allocator_round_trip(void)
{
    fs_error_t err;
    obj_key_t key;
    obj_key_t stale;
    obj_key_t reused;

    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objmgr_alloc_key(NULL);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);

    err = objmgr_alloc_key(&key);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(objkey_is_valid(&key));
    stale = key;
    stale.objectid = 0;
    err = objmgr_free_key(&stale);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    stale = key;

    err = objmgr_free_key(&key);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objmgr_free_key(&stale);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);

    err = objmgr_alloc_key(&reused);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(key.objectid, reused.objectid);
    TEST_ASSERT_EQ_INT((int)key.gen + 1, reused.gen);
    err = objmgr_free_key(&reused);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    object_deinit();
    return 0;
}

static int test_objmgr_create_lookup_delete_with_refs(void)
{
    fs_error_t err;
    fuid_t fuid = test_object_make_fuid(401);
    obj_handle_t handle = test_object_make_handle_with_seed(3);
    obj_meta_t *meta;
    obj_meta_t *held;
    obj_meta_t *by_handle;

    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    meta = objmgr_create(&fuid, &handle);
    TEST_ASSERT_TRUE(meta != NULL);
    TEST_ASSERT_EQ_INT(1, objmgr_count());
    TEST_ASSERT_TRUE(objmgr_exists(&fuid));
    TEST_ASSERT_EQ_INT(OBJ_STATE_ACTIVE, objmgr_state(&fuid));
    TEST_ASSERT_TRUE(objmgr_lookup(&fuid) == meta);

    held = objmgr_acquire(&fuid);
    TEST_ASSERT_TRUE(held == meta);
    TEST_ASSERT_EQ_INT(1, objmgr_refcnt(&fuid));
    by_handle = objmgr_acquire_by_handle(&handle);
    TEST_ASSERT_TRUE(by_handle == meta);
    TEST_ASSERT_EQ_INT(2, objmgr_refcnt(&fuid));

    /* 这里保留一个引用后执行 delete，用来覆盖延迟回收路径。 */
    objmgr_release(by_handle);
    TEST_ASSERT_EQ_INT(1, objmgr_refcnt(&fuid));
    err = objmgr_delete(&fuid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(OBJ_STATE_DELETING, objmgr_state(&fuid));
    err = objmgr_delete(&fuid);
    TEST_ASSERT_OBJECT_ERRNO(err, EBUSY);
    TEST_ASSERT_FALSE(objmgr_exists(&fuid));
    TEST_ASSERT_TRUE(objmgr_acquire(&fuid) == NULL);
    TEST_ASSERT_TRUE(objmgr_acquire_by_handle(&handle) == NULL);

    /* 最后一个引用释放时，DELETING 对象应真正从管理器中移除。 */
    objmgr_release(held);
    TEST_ASSERT_EQ_INT(0, objmgr_count());
    TEST_ASSERT_EQ_INT(OBJ_STATE_INVALID, objmgr_state(&fuid));
    TEST_ASSERT_TRUE(objmgr_lookup(&fuid) == NULL);
    objmgr_release(NULL);
    object_deinit();
    return 0;
}

static int test_objmgr_rejects_duplicate_key_and_handle(void)
{
    fs_error_t err;
    fuid_t fuid1 = test_object_make_fuid(411);
    fuid_t fuid2 = test_object_make_fuid(412);
    obj_handle_t handle1 = test_object_make_handle_with_seed(4);
    obj_handle_t handle2 = test_object_make_handle_with_seed(5);
    obj_meta_t *meta;

    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);
    meta = objmgr_create(&fuid1, &handle1);
    TEST_ASSERT_TRUE(meta != NULL);
    TEST_ASSERT_TRUE(objmgr_create(&fuid1, &handle2) == NULL);
    TEST_ASSERT_TRUE(objmgr_create(&fuid2, &handle1) == NULL);
    TEST_ASSERT_EQ_INT(1, objmgr_count());
    err = objmgr_delete(&fuid1);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(0, objmgr_count());
    object_deinit();
    return 0;
}

static int test_objmgr_public_error_paths(void)
{
    fs_error_t err;
    fuid_t missing = test_object_make_fuid(421);
    fuid_t invalid = fuid_make(0, 0, 0, FUID_TYPE_FILE);
    fuid_t active = test_object_make_fuid(422);
    obj_handle_t handle = test_object_make_handle_with_seed(6);
    obj_handle_t bad_handle = test_object_make_handle_with_seed(7);

    err = object_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = objmgr_delete(&missing);
    TEST_ASSERT_OBJECT_ERRNO(err, ENOENT);
    err = objmgr_get(&missing);
    TEST_ASSERT_OBJECT_ERRNO(err, ENOENT);
    err = objmgr_put(&missing);
    TEST_ASSERT_OBJECT_ERRNO(err, ENOENT);
    TEST_ASSERT_EQ_INT(OBJ_STATE_INVALID, objmgr_state(&missing));
    TEST_ASSERT_EQ_INT(0, objmgr_refcnt(&missing));
    TEST_ASSERT_TRUE(objmgr_create(&invalid, &handle) == NULL);

    bad_handle.len = 0;
    TEST_ASSERT_TRUE(objmgr_create(&active, &bad_handle) == NULL);
    TEST_ASSERT_TRUE(objmgr_create(&active, &handle) != NULL);
    err = objmgr_put(&active);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmgr_get(&active);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1, objmgr_refcnt(&active));
    err = objmgr_put(&active);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objmgr_delete(&active);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    object_deinit();
    return 0;
}

static int test_objmgr_internal_state_ref_and_handle_edges(void)
{
    fs_error_t err;
    obj_runtime_t rt;
    obj_runtime_t rt_dup;
    fs_hash_t handle_table;
    fuid_t fuid = test_object_make_fuid(431);
    fuid_t fuid_dup = test_object_make_fuid(432);
    obj_handle_t handle = test_object_make_handle_with_seed(8);

    memset(&rt, 0, sizeof(rt));
    memset(&rt_dup, 0, sizeof(rt_dup));
    /* 先直接验证状态机允许/拒绝矩阵，再验证改变状态的错误码。 */
    TEST_ASSERT_TRUE(objmgr_state_can_transit(OBJ_STATE_INIT,
                                              OBJ_STATE_ACTIVE));
    TEST_ASSERT_TRUE(objmgr_state_can_transit(OBJ_STATE_ACTIVE,
                                              OBJ_STATE_DELETING));
    TEST_ASSERT_FALSE(objmgr_state_can_transit(OBJ_STATE_INVALID,
                                               OBJ_STATE_ACTIVE));
    TEST_ASSERT_FALSE(objmgr_state_can_transit(OBJ_STATE_DELETING,
                                               OBJ_STATE_ACTIVE));
    err = objmgr_change_state(NULL, OBJ_STATE_ACTIVE);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmgr_change_state(&rt, OBJ_STATE_DELETING);
    TEST_ASSERT_OBJECT_ERRNO(err, EPERM);
    rt.state = OBJ_STATE_INIT;
    err = objmgr_change_state(&rt, OBJ_STATE_ACTIVE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objmgr_change_state(&rt, OBJ_STATE_INIT);
    TEST_ASSERT_OBJECT_ERRNO(err, EPERM);

    err = objmgr_ref_get_locked(NULL);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmgr_ref_get_locked(&rt);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1, rt.refcnt);
    err = objmgr_ref_put_locked(&rt);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objmgr_ref_put_locked(&rt);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    rt.state = OBJ_STATE_DELETING;
    err = objmgr_ref_get_locked(&rt);
    TEST_ASSERT_OBJECT_ERRNO(err, EBUSY);

    err = objmgr_handle_index_init(&handle_table, 4);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    g_objmgr.handle_table = handle_table;
    /* 单独初始化 handle 索引，避免依赖完整 object_init 的其他状态。 */
    err = objmgr_insert_handle_locked(NULL);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmeta_init(&rt.meta, &fuid, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objmeta_init(&rt_dup.meta, &fuid_dup, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objmgr_insert_handle_locked(&rt);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(objmgr_lookup_handle_locked(&handle) == &rt);
    err = objmgr_insert_handle_locked(&rt_dup);
    TEST_ASSERT_OBJECT_ERRNO(err, EEXIST);
    objmgr_remove_handle_locked(NULL);
    objmgr_remove_handle_locked(&rt);
    TEST_ASSERT_TRUE(objmgr_lookup_handle_locked(&handle) == NULL);
    objmgr_handle_index_deinit(&g_objmgr.handle_table);
    memset(&g_objmgr.handle_table, 0, sizeof(g_objmgr.handle_table));
    return 0;
}

const test_case_t OBJECT_OBJMGR_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x1,
                         0x001),
              test_objmgr_key_allocator_round_trip,
              "ObjMgr key 分配回收",
              "申请 key、释放、重复释放 stale key、再次申请",
              "重复释放被拒绝，再次申请复用槽位并提升 generation"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x1,
                         0x002),
              test_objmgr_create_lookup_delete_with_refs,
              "ObjMgr 引用中的删除流程",
              "创建对象后按 FUID/handle acquire，再带引用 delete",
              "DELETING 阶段禁止新引用，最后 release 后完成回收"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x1,
                         0x003),
              test_objmgr_rejects_duplicate_key_and_handle,
              "ObjMgr 重复 key/handle",
              "分别注入重复 FUID 和重复 backend handle",
              "重复创建失败且对象计数不被污染"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x1,
                         0x004),
              test_objmgr_public_error_paths,
              "ObjMgr 公开错误路径",
              "对缺失对象 get/put/delete/state/refcnt，并注入非法 create 输入",
              "缺失对象返回 ENOENT/INVALID，非法输入被拒绝"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x2),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMGR,
                         0x2,
                         0x001),
              test_objmgr_internal_state_ref_and_handle_edges,
              "ObjMgr 内部状态和 handle 边界",
              "直接覆盖状态迁移、引用计数和 handle 索引重复插入",
              "非法迁移/重复 handle 返回指定错误，索引删除后不可查"),
};

const size_t OBJECT_OBJMGR_CASE_COUNT = sizeof(OBJECT_OBJMGR_CASES) / sizeof(OBJECT_OBJMGR_CASES[0]);
