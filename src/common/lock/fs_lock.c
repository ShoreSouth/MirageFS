#include "common/lock/fs_lock.h"
#include "common/assert/fs_assert.h"

#include <string.h>
#include <assert.h>

/* ============================================================
 * mutex
 * ============================================================ */

int fs_mutex_init(fs_mutex_t *lock,
              const char *name,
              uint32_t flags)
{
    pthread_mutexattr_t attr;

    if (lock == NULL) {
        return -1;
    }

    memset(lock, 0, sizeof(*lock));

    pthread_mutexattr_init(&attr);

    if (flags & FS_LOCK_F_RECURSIVE) {

        pthread_mutexattr_settype(&attr,
                                  PTHREAD_MUTEX_RECURSIVE);
    }

    if (pthread_mutex_init(&lock->mutex,
                           &attr) != 0) {

        pthread_mutexattr_destroy(&attr);

        return -1;
    }

    pthread_mutexattr_destroy(&attr);

    lock->magic = FS_MUTEX_MAGIC;
    lock->flags = flags;
    lock->name  = name;

    return 0;
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
    if (lock == NULL) {
        return false;
    }

    return !fs_mutex_trylock(lock);
}

/* ============================================================
 * rwlock
 * ============================================================ */

int fs_rwlock_init(fs_rwlock_t *lock,
               const char *name,
               uint32_t flags)
{
    if (lock == NULL) {
        return -1;
    }

    memset(lock, 0, sizeof(*lock));

    if (pthread_rwlock_init(&lock->rwlock,
                            NULL) != 0) {

        return -1;
    }

    lock->magic = FS_RWLOCK_MAGIC;
    lock->flags = flags;
    lock->name  = name;

    return 0;
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