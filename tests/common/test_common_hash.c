#include "common_test_common.h"

typedef struct test_common_hash_node
{
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
    return (uint64_t) * (const int *)key;
}

static bool test_common_hash_match(const fs_list_head_t *node, const void *key)
{
    const test_common_hash_node_t *entry;

    entry = FS_CONTAINER_OF(node, test_common_hash_node_t, node);
    return entry->key == *(const int *)key;
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

    err = fs_hash_init(NULL, 4U, test_common_hash_node_hash,
                       test_common_hash_key_hash, test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_hash_init(&hash, 0U, test_common_hash_node_hash,
                       test_common_hash_key_hash, test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_hash_init(&hash, 4U, NULL, test_common_hash_key_hash,
                       test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_hash_init(&hash, 4U, test_common_hash_node_hash, NULL,
                       test_common_hash_match);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));
    err = fs_hash_init(&hash, 4U, test_common_hash_node_hash,
                       test_common_hash_key_hash, NULL);
    TEST_ASSERT_EQ_INT(FS_COMMON_SUB_HASH, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(FS_ERRNO_EINVAL, fs_err_errno(err));

    err = fs_hash_init(&hash, 4U, test_common_hash_node_hash,
                       test_common_hash_key_hash, test_common_hash_match);
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


const test_case_t COMMON_HASH_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_HASH, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_HASH, 0x1,
                             0x001),
                  test_hash_table_rejects_invalid_inputs_and_round_trips_nodes,
                  "Hash 表参数校验和节点回环",
                  "注入初始化/插入/查找/删除非法参数，再插入两个节点并查找删除",
                  "非法输入返回 COMMON/HASH/EINVAL，节点可按 key 命中并维护 "
                  "count"),
};

const size_t COMMON_HASH_CASE_COUNT =
        sizeof(COMMON_HASH_CASES) / sizeof(COMMON_HASH_CASES[0]);
