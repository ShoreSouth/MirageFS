#include "framework/test_framework.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "common/fs_common.h"
#include "lsa/include/lsa_api.h"
#include "object/fuid/fuid.h"
#include "object/objmeta/objmeta.h"

static int test_fuid_make_sets_identity_and_type(void)
{
    fuid_t fuid = fuid_make(7, 42, 3, FUID_TYPE_FILE);

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

static int test_objkey_from_fuid_round_trip(void)
{
    fuid_t fuid = fuid_make(9, 100, 5, FUID_TYPE_DIR);
    obj_key_t key;
    obj_key_t expected = objkey_make(100, 5);

    objkey_from_fuid(&key, &fuid);

    TEST_ASSERT_TRUE(objkey_is_valid(&key));
    TEST_ASSERT_TRUE(objkey_equal(&key, &expected));
    TEST_ASSERT_FALSE(objkey_equal(&key, NULL));
    TEST_ASSERT_TRUE(objkey_hash(&key) != 0);
    return 0;
}

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

static const test_case_t TEST_CASES[] = {
    TEST_CASE(test_fuid_make_sets_identity_and_type,
              "FUID 构造",
              "构造合法文件 FUID",
              "身份字段、版本和类型判断正确"),
    TEST_CASE(test_fuid_invalid_and_flags,
              "FUID 边界和 flags",
              "设置/清除 flag，并传入 NULL/invalid",
              "flag 状态正确，非法 FUID 被拒绝"),
    TEST_CASE(test_objkey_from_fuid_round_trip,
              "ObjKey 转换",
              "从 FUID 提取 objectid/gen",
              "ObjKey 可比较、可 hash，NULL 比较安全失败"),
    TEST_CASE(test_objmeta_handle_lsa_round_trip,
              "ObjMeta handle 转换",
              "LSA handle 与 Object handle 往返转换",
              "mount/type/len/data 字段保持一致"),
    TEST_CASE(test_objmeta_rejects_invalid_inputs,
              "ObjMeta 参数校验",
              "注入 NULL 输出和超长 LSA handle",
              "返回 OBJECT 模块的 EINVAL/EOVERFLOW"),
};

int main(void)
{
    return test_run_suite("object", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
