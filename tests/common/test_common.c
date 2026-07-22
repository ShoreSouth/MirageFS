#include "framework/test_framework.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "common/fs_common.h"


typedef enum test_common_component {
    TEST_COMMON_COMPONENT_ERROR = 0x01,
    TEST_COMMON_COMPONENT_MODULE = 0x02,
    TEST_COMMON_COMPONENT_FLAG = 0x03,
    TEST_COMMON_COMPONENT_PATH = 0x04,
    TEST_COMMON_COMPONENT_ATOMIC = 0x05,
    TEST_COMMON_COMPONENT_LOCK = 0x06,
    TEST_COMMON_COMPONENT_OS = 0x07,
    TEST_COMMON_COMPONENT_MEMPOOL = 0x08,
    TEST_COMMON_COMPONENT_HASH = 0x09,
    TEST_COMMON_COMPONENT_TYPE = 0x0a,
} test_common_component_t;

typedef struct test_common_hash_node {
    int key;
    fs_list_head_t node;
} test_common_hash_node_t;

static uint64_t test_common_hash_node_hash(const fs_list_head_t *node)
{
    const test_common_hash_node_t *entry;

    entry = FS_CONTAINER_OF(node, test_common_hash_node_t, node);
    return (uint64_t)entry->key;
}

static uint64_t test_common_hash_key_hash(const void *key)
{
    return (uint64_t)*(const int *)key;
}

static bool test_common_hash_match(const fs_list_head_t *node,
                                   const void *key)
{
    const test_common_hash_node_t *entry;

    entry = FS_CONTAINER_OF(node, test_common_hash_node_t, node);
    return entry->key == *(const int *)key;
}

static int test_error_layout_round_trip(void)
{
    fs_error_t err = FS_ERR(FS_SEV_ERROR,
                            FS_MODULE_COMMON,
                            FS_COMMON_SUB_PATH,
                            FS_ERRNO_EINVAL);

    TEST_ASSERT_EQ_INT(FS_SEV_ERROR, fs_err_severity(err));
    TEST_ASSERT_EQ_INT(FS_MODULE_COMMON, fs_err_module(err));
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_FALSE(fs_succeeded(err));
    return 0;
}

static int test_error_helpers_format_unknown_and_ok_values(void)
{
    fs_error_t err;
    const char *text1;
    const char *text2;

    TEST_ASSERT_STR_EQ("OK", fs_error_str(FS_OK));
    TEST_ASSERT_STR_EQ("INFO", fs_severity_name(FS_SEV_INFO));
    TEST_ASSERT_STR_EQ("WARN", fs_severity_name(FS_SEV_WARN));
    TEST_ASSERT_STR_EQ("ERROR", fs_severity_name(FS_SEV_ERROR));
    TEST_ASSERT_STR_EQ("FATAL", fs_severity_name(FS_SEV_FATAL));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_severity_name((fs_err_severity_t)99));

    TEST_ASSERT_TRUE(fs_errno_valid(FS_ERRNO_EINVAL));
    TEST_ASSERT_FALSE(fs_errno_valid(9999));
    TEST_ASSERT_STR_EQ("EINVAL", fs_errno_name(FS_ERRNO_EINVAL));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_errno_name((fs_errno_t)9999));
    TEST_ASSERT_STR_EQ("Unknown errno", fs_errno_desc((fs_errno_t)9999));

    err = FS_ERR(FS_SEV_WARN,
                 FS_MODULE_MAX,
                 0xffU,
                 (fs_errno_t)9999);
    text1 = fs_error_str(err);
    text2 = fs_error_str(err);
    TEST_ASSERT_TRUE(text1 != NULL);
    TEST_ASSERT_TRUE(text2 != NULL);
    TEST_ASSERT_TRUE(text1 != text2);
    return 0;
}

static int test_module_and_op_helpers_handle_valid_and_invalid_values(void)
{
    TEST_ASSERT_TRUE(fs_module_valid(FS_MODULE_COMMON));
    TEST_ASSERT_FALSE(fs_module_valid(FS_MODULE_NONE));
    TEST_ASSERT_FALSE(fs_module_valid(FS_MODULE_MAX));
    TEST_ASSERT_STR_EQ("COMMON", fs_module_name(FS_MODULE_COMMON));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_module_name(FS_MODULE_MAX));

    TEST_ASSERT_TRUE(fs_op_valid(FS_OP_LOOKUP));
    TEST_ASSERT_FALSE(fs_op_valid(FS_OP_MAX));
    TEST_ASSERT_STR_EQ("LOOKUP", fs_op_name(FS_OP_LOOKUP));
    TEST_ASSERT_STR_EQ("UNKNOWN", fs_op_name(FS_OP_MAX));
    return 0;
}

static int test_flag_helper_detects_set_and_missing_bits(void)
{
    fs_flags_t flags = FS_FLAG_READ | FS_FLAG_WRITE | FS_FLAG_SYNC;

    TEST_ASSERT_TRUE(fs_flag_test(flags, FS_FLAG_READ));
    TEST_ASSERT_TRUE(fs_flag_test(flags, FS_FLAG_WRITE));
    TEST_ASSERT_FALSE(fs_flag_test(flags, FS_FLAG_DIRECTORY));
    TEST_ASSERT_FALSE(fs_flag_test(FS_FLAG_NONE, FS_FLAG_READ));
    return 0;
}

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

static int test_path_join_safe_normalizes_slash(void)
{
    char path[64];
    fs_error_t err = fs_path_join_safe(path, sizeof(path), "/tmp/", "mirage");

    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ("/tmp/mirage", path);
    return 0;
}

static int test_path_join_rejects_too_small_buffer(void)
{
    char path[4];
    fs_error_t err = fs_path_join(path, sizeof(path), "/abc", "def");

    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(FS_MODULE_COMMON, fs_err_module(err));
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_ENOSPC, fs_err_errno(err));
    return 0;
}

static int test_path_normalize_removes_dotdot(void)
{
    char path[64];
    fs_error_t err = fs_path_normalize(path, sizeof(path), "/a//b/./c/../d");

    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ("/a/b/d", path);
    return 0;
}

static int test_path_normalize_collapses_leading_dotdot(void)
{
    char path[64];
    fs_error_t err = fs_path_normalize(path, sizeof(path), "../../escape");

    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ("escape", path);
    return 0;
}

static int test_path_helpers_cover_split_query_and_mkdir(void)
{
    char path[128];
    fs_error_t err;
    int rc;

    err = fs_path_dirname(path, sizeof(path), "/a/b/c");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ("/a/b", path);
    err = fs_path_dirname(path, sizeof(path), "leaf");
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_STR_EQ(".", path);
    err = fs_path_dirname(path, 2U, "/abc/def");
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_ENOSPC, fs_err_errno(err));

    TEST_ASSERT_STR_EQ("c", fs_path_basename("/a/b/c"));
    TEST_ASSERT_STR_EQ("leaf", fs_path_basename("leaf"));
    TEST_ASSERT_TRUE(fs_path_basename(NULL) == NULL);
    TEST_ASSERT_TRUE(fs_path_is_absolute("/a"));
    TEST_ASSERT_FALSE(fs_path_is_absolute("a"));
    TEST_ASSERT_TRUE(fs_path_is_empty(NULL));
    TEST_ASSERT_TRUE(fs_path_is_empty(""));
    TEST_ASSERT_FALSE(fs_path_is_empty("a"));

    rc = system("rm -rf -- '../output/tests/common/path-tree'");
    (void)rc;
    err = fs_path_mkdir_recursive("../output/tests/common/path-tree/a/b",
                                  0775);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fs_path_exists("../output/tests/common/path-tree/a/b"));
    TEST_ASSERT_FALSE(fs_path_exists("../output/tests/common/missing"));
    err = fs_path_mkdir_recursive("", 0775);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_PATH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    rc = system("rm -rf -- '../output/tests/common/path-tree'");
    (void)rc;
    return 0;
}

static int test_atomic_wrappers_update_and_compare_values(void)
{
    fs_atomic32_t atom32;
    fs_atomic64_t atom64;
    int32_t expected32;
    int64_t expected64;

    fs_atomic32_init(&atom32, 1);
    TEST_ASSERT_EQ_INT(1, fs_atomic32_load(&atom32));
    fs_atomic32_store(&atom32, 10);
    TEST_ASSERT_EQ_INT(11, fs_atomic32_inc(&atom32));
    TEST_ASSERT_EQ_INT(10, fs_atomic32_dec(&atom32));
    TEST_ASSERT_EQ_INT(15, fs_atomic32_add(&atom32, 5));
    TEST_ASSERT_EQ_INT(12, fs_atomic32_sub(&atom32, 3));
    expected32 = 12;
    TEST_ASSERT_TRUE(fs_atomic32_cas(&atom32, &expected32, 20));
    TEST_ASSERT_EQ_INT(20, fs_atomic32_load(&atom32));
    expected32 = 12;
    TEST_ASSERT_FALSE(fs_atomic32_cas(&atom32, &expected32, 30));
    TEST_ASSERT_EQ_INT(20, expected32);

    fs_atomic64_init(&atom64, 100);
    TEST_ASSERT_EQ_INT(100, fs_atomic64_load(&atom64));
    fs_atomic64_store(&atom64, 1000);
    TEST_ASSERT_EQ_INT(1001, fs_atomic64_inc(&atom64));
    TEST_ASSERT_EQ_INT(1000, fs_atomic64_dec(&atom64));
    TEST_ASSERT_EQ_INT(1025, fs_atomic64_add(&atom64, 25));
    TEST_ASSERT_EQ_INT(1000, fs_atomic64_sub(&atom64, 25));
    expected64 = 1000;
    TEST_ASSERT_TRUE(fs_atomic64_cas(&atom64, &expected64, 2000));
    expected64 = 1000;
    TEST_ASSERT_FALSE(fs_atomic64_cas(&atom64, &expected64, 3000));
    TEST_ASSERT_EQ_INT(2000, expected64);
    return 0;
}

static int test_lock_wrappers_cover_mutex_and_rwlock(void)
{
    fs_mutex_t mutex;
    fs_rwlock_t rwlock;
    fs_error_t err;

    err = fs_mutex_init(NULL, "bad", 0);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_LOCK, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));

    err = fs_mutex_init(&mutex, "common-test-mutex", 0);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_FALSE(fs_mutex_is_locked(NULL));
    TEST_ASSERT_FALSE(fs_mutex_is_locked(&mutex));
    fs_mutex_lock(&mutex);
    TEST_ASSERT_TRUE(fs_mutex_is_locked(&mutex));
    fs_mutex_unlock(&mutex);
    TEST_ASSERT_TRUE(fs_mutex_trylock(&mutex));
    fs_mutex_unlock(&mutex);
    fs_mutex_destroy(&mutex);
    fs_mutex_destroy(NULL);

    err = fs_mutex_init(&mutex, "common-test-recursive",
                        FS_LOCK_F_RECURSIVE);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    fs_mutex_lock(&mutex);
    TEST_ASSERT_TRUE(fs_mutex_trylock(&mutex));
    fs_mutex_unlock(&mutex);
    fs_mutex_unlock(&mutex);
    fs_mutex_destroy(&mutex);

    err = fs_rwlock_init(NULL, "bad", 0);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_LOCK, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_rwlock_init(&rwlock, "common-test-rwlock", FS_LOCK_F_DEBUG);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    fs_rwlock_rdlock(&rwlock);
    TEST_ASSERT_TRUE(fs_rwlock_tryrdlock(&rwlock));
    fs_rwlock_unlock(&rwlock);
    fs_rwlock_unlock(&rwlock);
    fs_rwlock_wrlock(&rwlock);
    TEST_ASSERT_FALSE(fs_rwlock_trywrlock(&rwlock));
    fs_rwlock_unlock(&rwlock);
    TEST_ASSERT_TRUE(fs_rwlock_trywrlock(&rwlock));
    fs_rwlock_unlock(&rwlock);
    fs_rwlock_destroy(&rwlock);
    return 0;
}

static int test_os_helpers_return_cached_names_and_times(void)
{
    const char *thread_name;
    const char *thread_name_cached;
    const char *process_name;
    const char *time_text;

    TEST_ASSERT_TRUE(fs_get_tid() > 0U);
    thread_name = fs_get_thread_name();
    thread_name_cached = fs_get_thread_name();
    process_name = fs_get_process_name();
    time_text = fs_time_str();

    TEST_ASSERT_TRUE(thread_name != NULL);
    TEST_ASSERT_TRUE(thread_name[0] != '\0');
    TEST_ASSERT_TRUE(thread_name == thread_name_cached);
    TEST_ASSERT_TRUE(process_name != NULL);
    TEST_ASSERT_TRUE(process_name[0] != '\0');
    TEST_ASSERT_TRUE(fs_get_time_s() > 0U);
    TEST_ASSERT_TRUE(fs_get_time_ms() > 0U);
    TEST_ASSERT_TRUE(fs_get_time_us() > 0U);
    TEST_ASSERT_TRUE(fs_get_time_ns() < 1000000000ULL);
    TEST_ASSERT_TRUE(fs_get_monotonic_s() > 0U);
    TEST_ASSERT_TRUE(fs_get_monotonic_ms() > 0U);
    TEST_ASSERT_TRUE(fs_get_monotonic_us() > 0U);
    TEST_ASSERT_TRUE(fs_get_monotonic_ns() < 1000000000ULL);
    TEST_ASSERT_TRUE(time_text != NULL);
    TEST_ASSERT_TRUE(time_text[0] != '\0');
    return 0;
}

static int test_mempool_create_alloc_realloc_and_global_wrappers(void)
{
    fs_mp_config_t cfg;
    fs_mp_stats_t stats;
    fs_mempool_t *mp;
    void *ptr;
    void *aligned_ptr;
    unsigned char *zero_ptr;
    char *text_ptr;
    char *grown_ptr;
    void *realloc_null_ptr;
    void *global_ptr;
    void *global_realloc_null_ptr;
    fs_error_t err;

    memset(&cfg, 0, sizeof(cfg));
    cfg.total_size = FS_MP_PAGE_SIZE * 16U;
    cfg.max_order = 4U;
    cfg.flags = FS_MP_F_POISON | FS_MP_F_STATS | FS_MP_F_THREAD_SAFE;

    TEST_ASSERT_TRUE(fs_mp_create(NULL) == NULL);
    cfg.max_order = FS_MP_MAX_ORDER + 1U;
    TEST_ASSERT_TRUE(fs_mp_create(&cfg) == NULL);
    cfg.max_order = 4U;

    mp = fs_mp_create(&cfg);
    TEST_ASSERT_TRUE(mp != NULL);
    TEST_ASSERT_TRUE(fs_mp_verify(mp));
    TEST_ASSERT_TRUE(fs_mp_alloc(NULL, 8U) == NULL);
    TEST_ASSERT_TRUE(fs_mp_alloc(mp, 0U) == NULL);
    TEST_ASSERT_TRUE(fs_mp_alloc_align(mp, 8U, 3U) == NULL);
    TEST_ASSERT_TRUE(fs_mp_contains(NULL, NULL) == false);
    TEST_ASSERT_EQ_INT(0U, fs_mp_usable_size(NULL));

    ptr = fs_mp_alloc(mp, 32U);
    TEST_ASSERT_TRUE(ptr != NULL);
    TEST_ASSERT_TRUE(fs_mp_contains(mp, ptr));
    TEST_ASSERT_TRUE(fs_mp_usable_size(ptr) >= 32U);

    aligned_ptr = fs_mp_alloc_align(mp, 48U, 64U);
    TEST_ASSERT_TRUE(aligned_ptr != NULL);
    TEST_ASSERT_EQ_INT(0U, ((uintptr_t)aligned_ptr) & 63U);

    zero_ptr = fs_mp_calloc(mp, 4U, 8U);
    TEST_ASSERT_TRUE(zero_ptr != NULL);
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQ_INT(0U, zero_ptr[i]);
    }
    TEST_ASSERT_TRUE(fs_mp_calloc(mp, 0U, 8U) == NULL);
    TEST_ASSERT_TRUE(fs_mp_calloc(mp, (size_t)-1, 2U) == NULL);

    text_ptr = fs_mp_alloc(mp, 8U);
    TEST_ASSERT_TRUE(text_ptr != NULL);
    memcpy(text_ptr, "abc", 4U);
    grown_ptr = fs_mp_realloc(mp, text_ptr, 32U);
    TEST_ASSERT_TRUE(grown_ptr != NULL);
    TEST_ASSERT_STR_EQ("abc", grown_ptr);
    realloc_null_ptr = fs_mp_realloc(mp, NULL, 16U);
    TEST_ASSERT_TRUE(realloc_null_ptr != NULL);
    fs_mp_free(mp, realloc_null_ptr);
    fs_mp_free(mp, grown_ptr);
    TEST_ASSERT_TRUE(fs_mp_realloc(mp, ptr, 0U) == NULL);

    TEST_ASSERT_TRUE(fs_mp_alloc(mp, FS_MP_PAGE_SIZE * 64U) == NULL);
    fs_mp_get_stats(mp, &stats);
    TEST_ASSERT_TRUE(stats.alloc_count >= 5U);
    TEST_ASSERT_TRUE(stats.free_count >= 2U);
    fs_mp_dump(mp);

    fs_mp_free(mp, aligned_ptr);
    fs_mp_free(mp, zero_ptr);
    fs_mp_free(mp, NULL);
    fs_mp_destroy(mp);
    fs_mp_destroy(NULL);

    TEST_ASSERT_TRUE(fs_mp_global() == NULL);
    err = fs_mp_global_init(&cfg);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fs_mp_global() != NULL);
    global_ptr = fs_zalloc(24U);
    TEST_ASSERT_TRUE(global_ptr != NULL);
    global_ptr = fs_realloc(global_ptr, 48U);
    TEST_ASSERT_TRUE(global_ptr != NULL);
    fs_free(global_ptr);
    TEST_ASSERT_TRUE(fs_malloc(0U) == NULL);
    global_realloc_null_ptr = fs_realloc(NULL, 16U);
    TEST_ASSERT_TRUE(global_realloc_null_ptr != NULL);
    fs_free(global_realloc_null_ptr);
    fs_mp_global_fini();
    TEST_ASSERT_TRUE(fs_mp_global() == NULL);
    err = fs_mp_global_init(NULL);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_MEMPOOL, fs_err_sub(err));
    fs_mp_global_fini();
    return 0;
}

static int test_hash_table_rejects_invalid_inputs_and_round_trips_nodes(void)
{
    fs_hash_t hash;
    test_common_hash_node_t a;
    test_common_hash_node_t b;
    fs_list_head_t *found;
    int key;
    fs_error_t err;

    memset(&hash, 0, sizeof(hash));
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.key = 1;
    b.key = 5;
    fs_list_init(&a.node);
    fs_list_init(&b.node);

    err = fs_hash_init(NULL,
                       4U,
                       test_common_hash_node_hash,
                       test_common_hash_key_hash,
                       test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_hash_init(&hash,
                       0U,
                       test_common_hash_node_hash,
                       test_common_hash_key_hash,
                       test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_hash_init(&hash,
                       4U,
                       NULL,
                       test_common_hash_key_hash,
                       test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_hash_init(&hash,
                       4U,
                       test_common_hash_node_hash,
                       NULL,
                       test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_hash_init(&hash,
                       4U,
                       test_common_hash_node_hash,
                       test_common_hash_key_hash,
                       NULL);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));

    err = fs_hash_init(&hash,
                       4U,
                       test_common_hash_node_hash,
                       test_common_hash_key_hash,
                       test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(0U, fs_hash_count(&hash));
    err = fs_hash_insert(NULL, &a.node);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    err = fs_hash_insert(&hash, NULL);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_TRUE(fs_hash_lookup(NULL, &key) == NULL);
    TEST_ASSERT_TRUE(fs_hash_lookup(&hash, NULL) == NULL);
    fs_hash_remove(NULL, &a.node);
    fs_hash_remove(&hash, NULL);

    err = fs_hash_insert(&hash, &a.node);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    err = fs_hash_insert(&hash, &b.node);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_EQ_INT(2U, fs_hash_count(&hash));
    key = 5;
    found = fs_hash_lookup(&hash, &key);
    TEST_ASSERT_TRUE(found == &b.node);
    key = 7;
    TEST_ASSERT_TRUE(fs_hash_lookup(&hash, &key) == NULL);
    fs_hash_remove(&hash, &a.node);
    TEST_ASSERT_EQ_INT(1U, fs_hash_count(&hash));
    fs_hash_remove(&hash, &b.node);
    TEST_ASSERT_EQ_INT(0U, fs_hash_count(&hash));
    fs_hash_destroy(&hash);
    fs_hash_destroy(NULL);
    return 0;
}

int main(void)
{
    const test_case_t cases[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_ERROR,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_ERROR,
                         0x1,
                         0x001),
              test_error_layout_round_trip,
              "错误码布局往返",
              "构造 fs_error_t 并逐字段解码",
              "severity/module/sub/errno 与构造值完全一致"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_ERROR,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_ERROR,
                         0x1,
                         0x002),
              test_error_helpers_format_unknown_and_ok_values,
              "错误 helper 格式化和未知值",
              "覆盖 OK、severity 名称、errno 名称/描述和未知模块格式化",
              "合法值返回名称，未知值返回 UNKNOWN，错误字符串使用轮转缓冲"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_MODULE,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_MODULE,
                         0x1,
                         0x001),
              test_module_and_op_helpers_handle_valid_and_invalid_values,
              "模块和操作名 helper",
              "传入合法枚举、NONE/MAX 边界值",
              "合法值返回名称，非法值返回 UNKNOWN 或 false"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_FLAG,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_FLAG,
                         0x1,
                         0x001),
              test_flag_helper_detects_set_and_missing_bits,
              "flag 位检测",
              "构造 READ/WRITE/SYNC 组合并检测缺失位",
              "已设置 flag 为 true，未设置 flag 为 false"),
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
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x001),
              test_path_join_safe_normalizes_slash,
              "路径拼接 slash 归一",
              "左侧路径已经以斜杠结尾",
              "结果只保留一个路径分隔符"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x002),
              test_path_join_rejects_too_small_buffer,
              "路径拼接小 buffer",
              "目标缓冲区空间不足",
              "返回 COMMON/PATH/ENOSPC"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x003),
              test_path_normalize_removes_dotdot,
              "路径 normalize 折叠 dotdot",
              "输入包含重复斜杠、点和点点片段",
              "返回折叠后的规范路径"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x004),
              test_path_normalize_collapses_leading_dotdot,
              "路径 normalize 折叠前导 dotdot",
              "输入以 ../.. 开头的相对路径",
              "当前实现折叠为剩余相对路径"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_PATH,
                         0x1,
                         0x005),
              test_path_helpers_cover_split_query_and_mkdir,
              "路径拆分、查询和递归建目录",
              "覆盖 dirname/basename/absolute/empty/exists/mkdir_recursive 边界",
              "路径 helper 返回预期结果，非法路径返回 COMMON/PATH 错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_ATOMIC,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_ATOMIC,
                         0x1,
                         0x001),
              test_atomic_wrappers_update_and_compare_values,
              "Atomic 包装器读写和 CAS",
              "对 32/64 位 atomic 执行 init/load/store/inc/dec/add/sub/cas",
              "返回修改后的值，CAS 成功替换、失败回写 expected"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_LOCK,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_LOCK,
                         0x1,
                         0x001),
              test_lock_wrappers_cover_mutex_and_rwlock,
              "锁包装器生命周期和 trylock",
              "覆盖 mutex/rwlock 初始化、加锁、trylock、解锁和销毁",
              "正常路径成功，NULL 初始化返回 COMMON/LOCK/EINVAL"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_OS,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_OS,
                         0x1,
                         0x001),
              test_os_helpers_return_cached_names_and_times,
              "OS helper 线程进程名和时间",
              "读取线程名、进程名、实时时间、单调时间和格式化时间",
              "返回非空名称和合理时间值，并命中线程名缓存"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_MEMPOOL,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_MEMPOOL,
                         0x1,
                         0x001),
              test_mempool_create_alloc_realloc_and_global_wrappers,
              "Mempool 分配器和全局包装器",
              "创建私有池后执行 alloc/align/calloc/realloc/free/stats/verify/dump，再覆盖全局池 wrapper",
              "正常分配可用，非法配置和超大分配按预期失败"),
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_HASH,
                         0x1),
              UT_CASE_NO(UT_MOD_COMMON,
                         TEST_COMMON_COMPONENT_HASH,
                         0x1,
                         0x001),
              test_hash_table_rejects_invalid_inputs_and_round_trips_nodes,
              "Hash 表参数校验和节点回环",
              "注入初始化/插入/查找/删除非法参数，再插入两个节点并查找删除",
              "非法输入返回 COMMON/HASH/EINVAL，节点可按 key 命中并维护 count"),
    };

    return test_run_suite("common", cases, sizeof(cases) / sizeof(cases[0]));
}
