#include "common_test_common.h"

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

    err = fs_mutex_init(&mutex, "common-test-recursive", FS_LOCK_F_RECURSIVE);
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


const test_case_t COMMON_LOCK_CASES[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_LOCK, 0x1),
                  UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_LOCK, 0x1,
                             0x001),
                  test_lock_wrappers_cover_mutex_and_rwlock,
                  "锁包装器生命周期和 trylock",
                  "覆盖 mutex/rwlock 初始化、加锁、trylock、解锁和销毁",
                  "正常路径成功，NULL 初始化返回 COMMON/LOCK/EINVAL"),
};

const size_t COMMON_LOCK_CASE_COUNT =
        sizeof(COMMON_LOCK_CASES) / sizeof(COMMON_LOCK_CASES[0]);
