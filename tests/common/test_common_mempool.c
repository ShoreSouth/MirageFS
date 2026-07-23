#include "common_test_common.h"

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

    /* 先覆盖配置和 API 参数防御，避免非法池进入后续分配路径。 */
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

    /* 对齐分配和 calloc 分别验证地址约束与清零语义。 */
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

    /* 全局 wrapper 依赖全局池，单独覆盖 init/fini 前后的可见状态。 */
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


const test_case_t COMMON_MEMPOOL_CASES[] = {
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
};

const size_t COMMON_MEMPOOL_CASE_COUNT = sizeof(COMMON_MEMPOOL_CASES) / sizeof(COMMON_MEMPOOL_CASES[0]);
