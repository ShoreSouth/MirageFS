#include "common/lock/fs_lock.h"
#include "common/assert/fs_assert.h"

#include <string.h>
#include <assert.h>

/* ============================================================
 * mutex
 * ============================================================ */

fs_error_t fs_mutex_init(fs_mutex_t *lock,
              const char *name,
              uint32_t flags)
{
    int rc;
    pthread_mutexattr_t attr;

    if (lock == NULL) {
        return fs_common_error(FS_COMMON_SUB_LOCK, FS_ERRNO_EINVAL);
    }

    memset(lock, 0, sizeof(*lock));

    pthread_mutexattr_init(&attr);

    if (flags & FS_LOCK_F_RECURSIVE) {

        pthread_mutexattr_settype(&attr,
                                  PTHREAD_MUTEX_RECURSIVE);
    }

    rc = pthread_mutex_init(&lock->mutex, &attr);
    if (rc != 0) {

        pthread_mutexattr_destroy(&attr);

        return fs_common_error(FS_COMMON_SUB_LOCK, rc);
    }

    pthread_mutexattr_destroy(&attr);

    lock->magic = FS_MUTEX_MAGIC;
    lock->flags = flags;
    lock->name  = name;

    return FS_OK;
}

void fs_mutex_destroy(fs_mutex_t *lock)
{
    if (lock == NULL) {
        return;
    }

    FS_ASSERT(lock->magic == FS_MUTEX_MAGIC);

    pthread_mutex_destroy(&lock->mutex);

    memset(lock, 0, sizeof(*lock));
}

void fs_mutex_lock(fs_mutex_t *lock)
{
    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_MUTEX_MAGIC);

    pthread_mutex_lock(&lock->mutex);

    lock->owner = pthread_self();
}

bool fs_mutex_trylock(fs_mutex_t *lock)
{
    int ret;

    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_MUTEX_MAGIC);

    ret = pthread_mutex_trylock(&lock->mutex);

    if (ret == 0) {

        lock->owner = pthread_self();

        return true;
    }

    return false;
}

void fs_mutex_unlock(fs_mutex_t *lock)
{
    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_MUTEX_MAGIC);

    lock->owner = 0;

    pthread_mutex_unlock(&lock->mutex);
}

bool fs_mutex_is_locked(fs_mutex_t *lock)
{
    bool locked;

    if (lock == NULL) {
        return false;
    }

    locked = !fs_mutex_trylock(lock);
    if (!locked) {
        fs_mutex_unlock(lock);
    }

    return locked;
}

/* ============================================================
 * rwlock
 * ============================================================ */

fs_error_t fs_rwlock_init(fs_rwlock_t *lock,
               const char *name,
               uint32_t flags)
{
    int rc;

    if (lock == NULL) {
        return fs_common_error(FS_COMMON_SUB_LOCK, FS_ERRNO_EINVAL);
    }

    memset(lock, 0, sizeof(*lock));

    rc = pthread_rwlock_init(&lock->rwlock, NULL);
    if (rc != 0) {
        return fs_common_error(FS_COMMON_SUB_LOCK, rc);
    }

    lock->magic = FS_RWLOCK_MAGIC;
    lock->flags = flags;
    lock->name  = name;

    return FS_OK;
}

void fs_rwlock_destroy(fs_rwlock_t *lock)
{
    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_RWLOCK_MAGIC);

    pthread_rwlock_destroy(&lock->rwlock);

    memset(lock, 0, sizeof(*lock));
}

void fs_rwlock_rdlock(fs_rwlock_t *lock)
{
    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_RWLOCK_MAGIC);

    pthread_rwlock_rdlock(&lock->rwlock);
}

void fs_rwlock_wrlock(fs_rwlock_t *lock)
{
    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_RWLOCK_MAGIC);

    pthread_rwlock_wrlock(&lock->rwlock);
}

bool fs_rwlock_tryrdlock(fs_rwlock_t *lock)
{
    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_RWLOCK_MAGIC);

    return pthread_rwlock_tryrdlock(&lock->rwlock) == 0;
}

bool fs_rwlock_trywrlock(fs_rwlock_t *lock)
{
    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_RWLOCK_MAGIC);

    return pthread_rwlock_trywrlock(&lock->rwlock) == 0;
}

void fs_rwlock_unlock(fs_rwlock_t *lock)
{
    FS_ASSERT(lock);
    FS_ASSERT(lock->magic == FS_RWLOCK_MAGIC);

    pthread_rwlock_unlock(&lock->rwlock);
}