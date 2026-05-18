#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Magic Number
 * ============================================================ */

/*
 * 用于校验锁对象合法性。
 *
 * 防止:
 *  - 野指针
 *  - 未初始化对象
 *  - 已销毁对象
 */
#define FS_MUTEX_MAGIC      0x4D54584DU  /* MTXM */
#define FS_RWLOCK_MAGIC     0x52574C4BU  /* RWLK */

/* ============================================================
 * Lock Flags
 * ============================================================ */

/*
 * 启用调试能力
 *
 * 当前:
 *  - owner记录
 *  - magic校验
 *
 * 后续可扩展:
 *  - contention统计
 *  - lock trace
 *  - 死锁检测
 */
#define FS_LOCK_F_DEBUG         (1U << 0)

/*
 * 递归锁
 *
 * 同线程允许重复加锁。
 *
 * 注意:
 *  - 普通业务不建议使用
 *  - 容易隐藏设计问题
 */
#define FS_LOCK_F_RECURSIVE     (1U << 1)

/* ============================================================
 * Mutex
 * ============================================================ */

/*
 * 互斥锁
 *
 * 特点:
 *  - 同时仅允许一个线程进入临界区
 *  - 最常用同步原语
 *
 * 适用场景:
 *  - mempool
 *  - stats
 *  - 链表
 *  - 配置管理
 *  - 小粒度共享资源
 */
typedef struct fs_mutex {

    uint32_t magic; // 对象合法性校验

    uint32_t flags; // 锁属性(FS_LOCK_F_XXX)

    const char *name; // 锁名字

    pthread_t owner; // 当前持锁线程ID(仅调试用途)

    pthread_mutex_t mutex; // 底层pthread mutex

} fs_mutex_t;

/* ============================================================
 * Read Write Lock
 * ============================================================ */

/*
 * 读写锁
 *
 * 特点:
 *  - 多读并发
 *  - 写独占
 *
 * 适合:
 *  - 读多写少场景
 *
 * 典型场景:
 *  - inode cache
 *  - dentry cache
 *  - 元数据缓存
 */
typedef struct fs_rwlock {

    uint32_t magic; // 对象合法性校验

    uint32_t flags; // 锁属性(FS_LOCK_F_XXX)

    /*
     * 锁名字
     */
    const char *name;

    /*
     * 底层pthread rwlock
     */
    pthread_rwlock_t rwlock;

} fs_rwlock_t;

/* ============================================================
 * Mutex APIs
 * ============================================================ */

/*
 * 初始化互斥锁
 *
 * 参数:
 *      lock:
 *          锁对象
 *
 *      name:
 *          锁名称
 *
 *      flags:
 *          FS_LOCK_F_XXX
 *
 * 返回:
 *      0      成功
 *      -1     失败
 *
 * 注意:
 *      lock必须在destroy前保持有效
 */
int fs_mutex_init(fs_mutex_t *lock,
              const char *name,
              uint32_t flags);

/*
 * 销毁互斥锁
 *
 * 注意:
 *      销毁前必须确保:
 *          - 无线程持锁
 *          - 无线程等待该锁
 */
void fs_mutex_destroy(fs_mutex_t *lock);

/*
 * 加锁（阻塞）
 *
 * 如果锁已被占用:
 *      当前线程进入睡眠等待。
 */
void fs_mutex_lock(fs_mutex_t *lock);

/*
 * 尝试加锁（非阻塞）
 *
 * 返回:
 *      true    获取成功
 *      false   获取失败
 */
bool fs_mutex_trylock(fs_mutex_t *lock);

/*
 * 解锁
 *
 * 注意:
 *      必须由持锁线程调用。
 */
void fs_mutex_unlock(fs_mutex_t *lock);

/*
 * 判断锁是否已被占用
 *
 * 返回:
 *      true    已加锁
 *      false   未加锁
 *
 * 注意:
 *      仅用于调试。
 *
 *      不保证严格并发一致性。
 */
bool fs_mutex_is_locked(fs_mutex_t *lock);

/* ============================================================
 * RWLock APIs
 * ============================================================ */

/*
 * 初始化读写锁
 */
int fs_rwlock_init(fs_rwlock_t *lock,
               const char *name,
               uint32_t flags);

/*
 * 销毁读写锁
 */
void fs_rwlock_destroy(fs_rwlock_t *lock);

/*
 * 获取读锁
 *
 * 特点:
 *      多线程可同时持有读锁
 *
 * 要求:
 *      无写锁持有
 */
void fs_rwlock_rdlock(fs_rwlock_t *lock);

/*
 * 获取写锁
 *
 * 特点:
 *      独占访问
 *
 * 要求:
 *      无读锁
 *      无写锁
 */
void fs_rwlock_wrlock(fs_rwlock_t *lock);

/*
 * 尝试获取读锁
 */
bool fs_rwlock_tryrdlock(fs_rwlock_t *lock);

/*
 * 尝试获取写锁
 */
bool fs_rwlock_trywrlock(fs_rwlock_t *lock);

/*
 * 释放读写锁
 */
void fs_rwlock_unlock(fs_rwlock_t *lock);

#ifdef __cplusplus
}
#endif