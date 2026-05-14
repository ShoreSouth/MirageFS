#pragma once

#define _GNU_SOURCE
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Magic
 * ============================================================ */

#define FS_MUTEX_MAGIC      0x4D54584DU
#define FS_RWLOCK_MAGIC     0x52574C4BU

/* ============================================================
 * Flags
 * ============================================================ */

#define FS_LOCK_F_DEBUG     (1U << 0)
#define FS_LOCK_F_RECURSIVE (1U << 1)

/* ============================================================
 * mutex
 * ============================================================ */

typedef struct fs_mutex {

    uint32_t magic;

    uint32_t flags;

    const char *name;

    pthread_t owner;

    pthread_mutex_t mutex;

} fs_mutex_t;

/* ============================================================
 * rwlock
 * ============================================================ */

typedef struct fs_rwlock {

    uint32_t magic;

    uint32_t flags;

    const char *name;

    pthread_rwlock_t rwlock;

} fs_rwlock_t;

/* ============================================================
 * mutex
 * ============================================================ */

int fs_mutex_init(fs_mutex_t *lock,
              const char *name,
              uint32_t flags);

void fs_mutex_destroy(fs_mutex_t *lock);

void fs_mutex_lock(fs_mutex_t *lock);

bool fs_mutex_trylock(fs_mutex_t *lock);

void fs_mutex_unlock(fs_mutex_t *lock);

bool fs_mutex_is_locked(fs_mutex_t *lock);

/* ============================================================
 * rwlock
 * ============================================================ */

int fs_rwlock_init(fs_rwlock_t *lock,
               const char *name,
               uint32_t flags);

void fs_rwlock_destroy(fs_rwlock_t *lock);

void fs_rwlock_rdlock(fs_rwlock_t *lock);

void fs_rwlock_wrlock(fs_rwlock_t *lock);

bool fs_rwlock_tryrdlock(fs_rwlock_t *lock);

bool fs_rwlock_trywrlock(fs_rwlock_t *lock);

void fs_rwlock_unlock(fs_rwlock_t *lock);

#ifdef __cplusplus
}
#endif