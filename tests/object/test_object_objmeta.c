#include "object_test_common.h"

static int test_objmeta_handle_lsa_round_trip(void)
{
    lsa_file_handle_t lsa_handle;
    lsa_file_handle_t out_lsa;
    obj_handle_t obj_handle;
    fs_error_t err;

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    lsa_handle.handle_type = 1;
    lsa_handle.handle_bytes = 4;
    lsa_handle.data[0] = 0x11;
    lsa_handle.data[1] = 0x22;
    lsa_handle.data[2] = 0x33;
    lsa_handle.data[3] = 0x44;

    err = objmeta_handle_from_lsa(&obj_handle, &lsa_handle, 88);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(88, obj_handle.mount_id);
    TEST_ASSERT_EQ_INT(1, obj_handle.type);
    TEST_ASSERT_EQ_INT(4, obj_handle.len);

    err = objmeta_handle_to_lsa(&out_lsa, &obj_handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(1, out_lsa.handle_type);
    TEST_ASSERT_EQ_INT(4, out_lsa.handle_bytes);
    TEST_ASSERT_TRUE(memcmp(out_lsa.data, lsa_handle.data, 4) == 0);
    return 0;
}

static int test_objmeta_init_equal_and_reset(void)
{
    obj_meta_t meta;
    obj_meta_t copy;
    fuid_t fuid = fuid_make(5, 77, 2, FUID_TYPE_FILE);
    obj_handle_t handle = test_object_make_handle();
    fs_error_t err;

    err = objmeta_init(&meta, &fuid, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(objmeta_is_valid(&meta));

    copy = meta;
    TEST_ASSERT_TRUE(objmeta_equal(&meta, &copy));
    copy.handle.data[0] ^= 0xffU;
    TEST_ASSERT_FALSE(objmeta_equal(&meta, &copy));

    objmeta_deinit(&meta);
    TEST_ASSERT_FALSE(objmeta_is_valid(&meta));
    return 0;
}

static int test_objmeta_rejects_invalid_inputs(void)
{
    fs_error_t err;
    lsa_file_handle_t lsa_handle;
    obj_handle_t obj_handle;

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    lsa_handle.handle_bytes = OBJMETA_MAX_HANDLE_SIZE + 1U;

    err = objmeta_handle_from_lsa(NULL, &lsa_handle, 1);
    TEST_ASSERT_EQ_INT(FS_MODULE_OBJECT, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    err = objmeta_handle_from_lsa(&obj_handle, &lsa_handle, 1);
    TEST_ASSERT_EQ_INT(FS_MODULE_OBJECT, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EOVERFLOW, fs_err_errno(err));
    return 0;
}

static int test_objmeta_rejects_handle_and_init_edges(void)
{
    fs_error_t err;
    lsa_file_handle_t lsa_handle;
    lsa_file_handle_t out_lsa;
    obj_handle_t obj_handle = test_object_make_handle();
    obj_handle_t bad_handle = test_object_make_handle();
    obj_meta_t meta;
    fuid_t fuid = test_object_make_fuid(201);
    fuid_t invalid_fuid = fuid_make(0, 0, 0, FUID_TYPE_FILE);

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    lsa_handle.handle_type = -1;
    lsa_handle.handle_bytes = 1;
    /* LSA handle 类型来自内核侧，转换时必须拒绝负数和 uint16 溢出。 */
    err = objmeta_handle_from_lsa(&obj_handle, &lsa_handle, 1);
    TEST_ASSERT_OBJECT_ERRNO(err, EOVERFLOW);

    lsa_handle.handle_type = (int)UINT16_MAX + 1;
    err = objmeta_handle_from_lsa(&obj_handle, &lsa_handle, 1);
    TEST_ASSERT_OBJECT_ERRNO(err, EOVERFLOW);

    err = objmeta_handle_to_lsa(NULL, &obj_handle);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmeta_handle_to_lsa(&out_lsa, NULL);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    TEST_ASSERT_FALSE(objmeta_is_valid(NULL));

    bad_handle.len = 0;
    /* ObjMeta 是 object 身份根，空 FUID/handle 都不能进入有效状态。 */
    err = objmeta_handle_to_lsa(&out_lsa, &bad_handle);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmeta_init(NULL, &fuid, &obj_handle);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmeta_init(&meta, NULL, &obj_handle);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmeta_init(&meta, &fuid, NULL);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmeta_init(&meta, &invalid_fuid, &obj_handle);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    err = objmeta_init(&meta, &fuid, &bad_handle);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);

    objmeta_reset(NULL);
    objmeta_deinit(NULL);
    objmeta_dump(NULL);
    return 0;
}

static int test_objmeta_equal_detects_all_identity_fields(void)
{
    fs_error_t err;
    obj_meta_t meta;
    obj_meta_t copy;
    fuid_t fuid = test_object_make_fuid(202);
    obj_handle_t handle = test_object_make_handle();

    err = objmeta_init(&meta, &fuid, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    copy = meta;
    TEST_ASSERT_TRUE(objmeta_equal(&meta, &copy));
    TEST_ASSERT_FALSE(objmeta_equal(NULL, &copy));
    TEST_ASSERT_FALSE(objmeta_equal(&meta, NULL));

    /* 相等性要覆盖 key 与 backend handle 的每个身份字段。 */
    copy = meta;
    copy.key.objectid++;
    TEST_ASSERT_FALSE(objmeta_equal(&meta, &copy));
    copy = meta;
    copy.handle.mount_id++;
    TEST_ASSERT_FALSE(objmeta_equal(&meta, &copy));
    copy = meta;
    copy.handle.type++;
    TEST_ASSERT_FALSE(objmeta_equal(&meta, &copy));
    copy = meta;
    copy.handle.len++;
    TEST_ASSERT_FALSE(objmeta_equal(&meta, &copy));
    copy = meta;
    copy.handle.data[1] ^= 0xffU;
    TEST_ASSERT_FALSE(objmeta_equal(&meta, &copy));

    objmeta_dump(&meta);
    objmeta_deinit(&meta);
    return 0;
}

const test_case_t OBJECT_OBJMETA_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x1,
                         0x001),
              test_objmeta_handle_lsa_round_trip,
              "ObjMeta handle 转换",
              "LSA handle 与 Object handle 往返转换",
              "mount/type/len/data 字段保持一致"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x1,
                         0x002),
              test_objmeta_init_equal_and_reset,
              "ObjMeta 初始化与比较",
              "初始化后复制、篡改 handle、再 deinit",
              "有效性、相等性和清空状态正确"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x1,
                         0x003),
              test_objmeta_rejects_invalid_inputs,
              "ObjMeta 参数校验",
              "注入 NULL 输出和超长 LSA handle",
              "返回 OBJECT 模块的 EINVAL/EOVERFLOW"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x2),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x2,
                         0x001),
              test_objmeta_rejects_handle_and_init_edges,
              "ObjMeta handle 边界拒绝",
              "注入非法 LSA 类型、NULL 参数、空 handle 和非法 FUID",
              "返回 OBJECT 模块 EINVAL/EOVERFLOW 且空操作安全"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x2),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJMETA,
                         0x2,
                         0x002),
              test_objmeta_equal_detects_all_identity_fields,
              "ObjMeta 相等性字段差异",
              "逐项篡改 key、mount、type、len、data 字段",
              "任一身份字段变化都会判定为不相等"),
};

const size_t OBJECT_OBJMETA_CASE_COUNT = sizeof(OBJECT_OBJMETA_CASES) / sizeof(OBJECT_OBJMETA_CASES[0]);
