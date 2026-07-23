#include "object_test_common.h"

static int test_objtable_insert_lookup_remove_round_trip(void)
{
    fs_error_t err;
    obj_table_t table;
    obj_runtime_t rt;
    fuid_t fuid = fuid_make(3, 55, 1, FUID_TYPE_FILE);
    obj_handle_t handle = test_object_make_handle();
    obj_key_t key = objkey_make(55, 1);

    memset(&rt, 0, sizeof(rt));
    err = objmeta_init(&rt.meta, &fuid, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = objtable_init(&table, 8);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(0, objtable_count(&table));

    /* 表内只索引 runtime 指针；插入后 lookup 返回的应是调用方提供的对象。 */
    err = objtable_insert(&table, &rt);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1, objtable_count(&table));
    TEST_ASSERT_TRUE(objtable_exists(&table, &key));
    TEST_ASSERT_TRUE(objtable_lookup(&table, &key) == &rt);

    err = objtable_insert(&table, &rt);
    TEST_ASSERT_EQ_INT(FS_MODULE_OBJECT, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EEXIST, fs_err_errno(err));

    err = objtable_remove(&table, &key);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(0, objtable_count(&table));
    TEST_ASSERT_FALSE(objtable_exists(&table, &key));

    objtable_destroy(&table);
    return 0;
}

static int test_objtable_rejects_invalid_inputs(void)
{
    fs_error_t err;
    obj_table_t table;
    obj_key_t key = objkey_make(1, 1);

    err = objtable_init(NULL, 8);
    TEST_ASSERT_EQ_INT(FS_MODULE_OBJECT, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = objtable_init(&table, 8);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    /* 公共 API 对 NULL runtime 与缺失 key 分别返回 EINVAL/ENOENT。 */
    err = objtable_insert(&table, NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_OBJECT, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = objtable_remove(&table, &key);
    TEST_ASSERT_EQ_INT(FS_MODULE_OBJECT, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOENT, fs_err_errno(err));

    TEST_ASSERT_TRUE(objtable_lookup(NULL, &key) == NULL);
    TEST_ASSERT_EQ_INT(0, objtable_count(NULL));
    objtable_destroy(&table);
    return 0;
}

static int test_objtable_destroy_and_edge_paths(void)
{
    fs_error_t err;
    obj_table_t table;
    obj_runtime_t rt1;
    obj_runtime_t rt2;
    obj_runtime_t invalid_rt;
    fuid_t fuid1 = test_object_make_fuid(301);
    fuid_t fuid2 = test_object_make_fuid(302);
    obj_handle_t handle1 = test_object_make_handle_with_seed(1);
    obj_handle_t handle2 = test_object_make_handle_with_seed(2);
    obj_key_t missing = objkey_make(999, 1);

    memset(&rt1, 0, sizeof(rt1));
    memset(&rt2, 0, sizeof(rt2));
    memset(&invalid_rt, 0, sizeof(invalid_rt));
    /* 无效 key/runtime 不应进入表；NULL 查询保持防御性失败。 */
    TEST_ASSERT_FALSE(objkey_is_valid(NULL));
    TEST_ASSERT_FALSE(objkey_is_valid(&invalid_rt.meta.key));
    TEST_ASSERT_FALSE(objtable_exists(NULL, &missing));
    TEST_ASSERT_TRUE(objtable_lookup(NULL, NULL) == NULL);

    err = objtable_init(&table, 4);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objtable_insert(&table, &invalid_rt);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmeta_init(&rt1.meta, &fuid1, &handle1);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objmeta_init(&rt2.meta, &fuid2, &handle2);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objtable_insert(&table, &rt1);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = objtable_insert(&table, &rt2);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(2, objtable_count(&table));

    /* 带元素 destroy 要能清空表结构；runtime 本体仍由调用方管理。 */
    err = objtable_remove(&table, NULL);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    objtable_destroy(&table);
    objtable_destroy(NULL);
    return 0;
}

const test_case_t OBJECT_OBJTABLE_CASES[] = {
        TEST_CASE(
                UT_LIST_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_OBJTABLE, 0x1),
                UT_CASE_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_OBJTABLE, 0x1,
                           0x001),
                test_objtable_insert_lookup_remove_round_trip,
                "ObjTable 插入查找删除",
                "插入有效 runtime，按 ObjKey 查询，再重复插入并删除",
                "lookup 返回原 runtime 指针，重复插入 EEXIST，删除后计数归零且 "
                "key 不存在"),
        TEST_CASE(
                UT_LIST_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_OBJTABLE, 0x1),
                UT_CASE_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_OBJTABLE, 0x1,
                           0x002),
                test_objtable_rejects_invalid_inputs, "ObjTable 参数校验",
                "初始化传入 NULL table，插入 NULL runtime，并删除缺失 key",
                "返回 OBJECT 模块 EINVAL/ENOENT，NULL 查询和计数安全失败"),
        TEST_CASE(
                UT_LIST_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_OBJTABLE, 0x2),
                UT_CASE_NO(UT_MOD_OBJECT, TEST_OBJECT_COMPONENT_OBJTABLE, 0x2,
                           0x001),
                test_objtable_destroy_and_edge_paths, "ObjTable 销毁和边界路径",
                "插入多个 runtime 后直接 destroy，并注入无效 key/runtime",
                "计数正确，非法输入返回 OBJECT/EINVAL，销毁可释放表项"),
};

const size_t OBJECT_OBJTABLE_CASE_COUNT =
        sizeof(OBJECT_OBJTABLE_CASES) / sizeof(OBJECT_OBJTABLE_CASES[0]);
