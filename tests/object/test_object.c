#include "framework/test_framework.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "common/fs_common.h"
#include "lsa/include/lsa_api.h"
#include "object/fuid/fuid.h"
#include "object/object_init.h"
#include "object/obj_error.h"
#include "object/objmeta/objmeta.h"
#include "object/objruntime/objruntime.h"
#include "object/objtable/objtable.h"
#include "object/objmgr/objmgr.h"
#include "object/objmgr/objmgr_internal.h"
#include "object/objpool/objpool.h"


typedef enum test_object_component {
    TEST_OBJECT_COMPONENT_FUID = 0x01,
    TEST_OBJECT_COMPONENT_OBJKEY = 0x02,
    TEST_OBJECT_COMPONENT_OBJMETA = 0x03,
    TEST_OBJECT_COMPONENT_OBJRUNTIME = 0x04,
    TEST_OBJECT_COMPONENT_OBJTABLE = 0x05,
    TEST_OBJECT_COMPONENT_OBJPOOL = 0x06,
    TEST_OBJECT_COMPONENT_OBJMGR = 0x07,
} test_object_component_t;

static obj_handle_t make_handle_with_seed(uint8_t seed)
{
    obj_handle_t handle;

    memset(&handle, 0, sizeof(handle));
    handle.mount_id = (uint64_t)(88U + seed);
    handle.type = (uint16_t)(1U + seed);
    handle.len = 4;
    handle.data[0] = (uint8_t)(0x11U + seed);
    handle.data[1] = (uint8_t)(0x22U + seed);
    handle.data[2] = (uint8_t)(0x33U + seed);
    handle.data[3] = (uint8_t)(0x44U + seed);
    return handle;
}

static obj_handle_t make_handle(void)
{
    return make_handle_with_seed(0);
}

static fuid_t make_object_fuid(ObjectId_t objectid)
{
    return fuid_make(17, objectid, 1, FUID_TYPE_FILE);
}

#define TEST_ASSERT_OBJECT_ERRNO(err, posix_errno) \
    do { \
        fs_error_t test_err__ = (err); \
        TEST_ASSERT_EQ_INT(FS_MODULE_OBJECT, fs_err_module(test_err__)); \
        TEST_ASSERT_EQ_INT((posix_errno), fs_err_errno(test_err__)); \
    } while (0)

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

static int test_objmeta_init_equal_and_reset(void)
{
    obj_meta_t meta;
    obj_meta_t copy;
    fuid_t fuid = fuid_make(5, 77, 2, FUID_TYPE_FILE);
    obj_handle_t handle = make_handle();
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

static int test_objtable_insert_lookup_remove_round_trip(void)
{
    fs_error_t err;
    obj_table_t table;
    obj_runtime_t rt;
    fuid_t fuid = fuid_make(3, 55, 1, FUID_TYPE_FILE);
    obj_handle_t handle = make_handle();
    obj_key_t key = objkey_make(55, 1);

    memset(&rt, 0, sizeof(rt));
    err = objmeta_init(&rt.meta, &fuid, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = objtable_init(&table, 8);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(0, objtable_count(&table));

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

static int test_objmeta_rejects_handle_and_init_edges(void)
{
    fs_error_t err;
    lsa_file_handle_t lsa_handle;
    lsa_file_handle_t out_lsa;
    obj_handle_t obj_handle = make_handle();
    obj_handle_t bad_handle = make_handle();
    obj_meta_t meta;
    fuid_t fuid = make_object_fuid(201);
    fuid_t invalid_fuid = fuid_make(0, 0, 0, FUID_TYPE_FILE);

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    lsa_handle.handle_type = -1;
    lsa_handle.handle_bytes = 1;
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
    fuid_t fuid = make_object_fuid(202);
    obj_handle_t handle = make_handle();

    err = objmeta_init(&meta, &fuid, &handle);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    copy = meta;
    TEST_ASSERT_TRUE(objmeta_equal(&meta, &copy));
    TEST_ASSERT_FALSE(objmeta_equal(NULL, &copy));
    TEST_ASSERT_FALSE(objmeta_equal(&meta, NULL));

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

static int test_objtable_destroy_and_edge_paths(void)
{
    fs_error_t err;
    obj_table_t table;
    obj_runtime_t rt1;
    obj_runtime_t rt2;
    obj_runtime_t invalid_rt;
    fuid_t fuid1 = make_object_fuid(301);
    fuid_t fuid2 = make_object_fuid(302);
    obj_handle_t handle1 = make_handle_with_seed(1);
    obj_handle_t handle2 = make_handle_with_seed(2);
    obj_key_t missing = objkey_make(999, 1);

    memset(&rt1, 0, sizeof(rt1));
    memset(&rt2, 0, sizeof(rt2));
    memset(&invalid_rt, 0, sizeof(invalid_rt));
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

    err = objtable_remove(&table, NULL);
    TEST_ASSERT_OBJECT_ERRNO(err, EINVAL);
    objtable_destroy(&table);
    objtable_destroy(NULL);
    return 0;
}

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
    fuid_t fuid = make_object_fuid(401);
    obj_handle_t handle = make_handle_with_seed(3);
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
    fuid_t fuid1 = make_object_fuid(411);
    fuid_t fuid2 = make_object_fuid(412);
    obj_handle_t handle1 = make_handle_with_seed(4);
    obj_handle_t handle2 = make_handle_with_seed(5);
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
    fuid_t missing = make_object_fuid(421);
    fuid_t invalid = fuid_make(0, 0, 0, FUID_TYPE_FILE);
    fuid_t active = make_object_fuid(422);
    obj_handle_t handle = make_handle_with_seed(6);
    obj_handle_t bad_handle = make_handle_with_seed(7);

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
    fuid_t fuid = make_object_fuid(431);
    fuid_t fuid_dup = make_object_fuid(432);
    obj_handle_t handle = make_handle_with_seed(8);

    memset(&rt, 0, sizeof(rt));
    memset(&rt_dup, 0, sizeof(rt_dup));
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

static const test_case_t TEST_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_FUID,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_FUID,
                         0x1,
                         0x001),
              test_fuid_make_sets_identity_and_type,
              "FUID 构造",
              "构造合法文件 FUID",
              "身份字段、版本和类型判断正确"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_FUID,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_FUID,
                         0x1,
                         0x002),
              test_fuid_invalid_and_flags,
              "FUID 边界和 flags",
              "设置/清除 flag，并传入 NULL/invalid",
              "flag 状态正确，非法 FUID 被拒绝"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJKEY,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJKEY,
                         0x1,
                         0x001),
              test_objkey_from_fuid_round_trip,
              "ObjKey 转换",
              "从 FUID 提取 objectid/gen",
              "ObjKey 可比较、可 hash，NULL 比较安全失败"),
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
                         TEST_OBJECT_COMPONENT_OBJTABLE,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJTABLE,
                         0x1,
                         0x001),
              test_objtable_insert_lookup_remove_round_trip,
              "ObjTable 插入查找删除",
              "插入有效 runtime 并重复插入",
              "查找命中、重复插入 EEXIST、删除后不存在"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJTABLE,
                         0x1),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJTABLE,
                         0x1,
                         0x002),
              test_objtable_rejects_invalid_inputs,
              "ObjTable 参数校验",
              "NULL table/runtime 和删除缺失 key",
              "返回 OBJECT 模块 EINVAL/ENOENT"),
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_FUID,
                         0x2),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_FUID,
                         0x2,
                         0x001),
              test_fuid_types_hash_and_debug_helpers,
              "FUID 类型和调试 helper",
              "构造全部合法类型、NULL 输入和同身份不同类型",
              "类型 helper/hash/debug 字符串和安全空操作符合预期"),
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
    TEST_CASE(UT_LIST_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJTABLE,
                         0x2),
              UT_CASE_NO(UT_MOD_OBJECT,
                         TEST_OBJECT_COMPONENT_OBJTABLE,
                         0x2,
                         0x001),
              test_objtable_destroy_and_edge_paths,
              "ObjTable 销毁和边界路径",
              "插入多个 runtime 后直接 destroy，并注入无效 key/runtime",
              "计数正确，非法输入返回 OBJECT/EINVAL，销毁可释放表项"),
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

int main(void)
{
    return test_run_suite("object", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
