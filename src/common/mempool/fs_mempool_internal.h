#pragma once

/*
 * mempool 内部实现头文件
 *
 * 注意:
 *      本文件仅允许:
 *
 *          fs_mempool.c
 *
 *      等mempool内部实现使用。
 *
 * 禁止:
 *      外部业务模块直接include。
 *
 * 原因:
 *      - 包含buddy allocator实现细节
 *      - 包含内部结构定义
 *      - 后续可能频繁修改
 *
 * 对外请使用:
 *
 *      fs_mempool.h
 */

#include "common/mempool/fs_mempool.h"

#include "common/list/fs_list.h"
#include "common/lock/fs_lock.h"
#include "common/assert/fs_assert.h"
#include "common/log/fs_log.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================
 * Internal Helper Macros
 * ============================================================ */

/*
 * mempool加锁
 */
#define FS_MP_LOCK(mp) fs_mutex_lock(&(mp)->lock)

/*
 * mempool解锁
 */
#define FS_MP_UNLOCK(mp) fs_mutex_unlock(&(mp)->lock)

/*
 * order对应block大小
 *
 * 例如:
 *
 *      order=0
 *          -> 4KB
 *
 *      order=1
 *          -> 8KB
 *
 *      order=2
 *          -> 16KB
 */
#define FS_MP_ORDER_SIZE(order) ((uint64_t)FS_MP_PAGE_SIZE << (order))

    /* ============================================================
 * Buddy Free Block
 * ============================================================ */

    /*
 * buddy空闲块
 *
 * 仅在:
 *      block处于free状态时有效。
 *
 * free状态:
 *
 *      +----------------------------------+
 *      | fs_mp_block_t                    |
 *      |      node                        |
 *      +----------------------------------+
 *
 * alloc状态:
 *
 *      +----------------------------------+
 *      | fs_mp_hdr_t                      |
 *      +----------------------------------+
 *      | user data                        |
 *      +----------------------------------+
 *
 * 注意:
 *      free block 与 alloc header
 *      共用同一片内存区域。
 */
    typedef struct fs_mp_block
    {
        /*
     * buddy free list node
     */
        fs_list_head_t node;

    } fs_mp_block_t;

    /* ============================================================
 * Memory Pool Core Object
 * ============================================================ */

    /*
 * mempool核心对象
 *
 * buddy allocator全局控制中心。
 *
 * 当前实现:
 *      - 单连续大内存
 *      - 4KB page
 *      - buddy allocator
 *      - split/merge
 *      - 全局锁
 *
 * 后续扩展:
 *      - slab
 *      - thread cache
 *      - NUMA
 *      - huge page
 *      - lockless fast path
 */
    typedef struct fs_mempool
    {
        void *base; /* 内存池起始地址，由 aligned_alloc() 分配，要求: PAGE_SIZE对齐 */

        uint64_t total_size; /* 内存池总大小，要求:PAGE_SIZE * 2^N */

        uint32_t max_order; /* 最大伙伴阶，最大块大小: PAGE_SIZE << max_order */

        uint32_t flags; /* FS_MP_F_XXX */

        fs_list_head_t
                free_area[FS_MP_MAX_ORDER + 1]; /* 每个order一个空闲链表 */

        fs_mp_stats_t stats; /* 内存池统计 */

        fs_mutex_t lock; /* mempool全局锁，当前:整个pool共用一把锁 */

    } fs_mempool_t;

    /* ============================================================
 * Internal Helper APIs
 * ============================================================ */

    /*
 * size -> buddy order
 *
 * 输入:
 *      用户申请大小
 *
 * 输出:
 *      能容纳:
 *
 *          hdr + user_data
 *
 *      的最小order。
 */
    uint32_t fs_mp_calc_order(size_t size);

    /*
 * ptr -> hdr
 *
 * 用户布局:
 *
 *      +----------------+
 *      | hdr            |
 *      +----------------+
 *      | user_ptr       |
 *      +----------------+
 */
    fs_mp_hdr_t *fs_mp_ptr_to_hdr(void *ptr);

    /*
 * hdr -> user ptr
 */
    void *fs_mp_hdr_to_ptr(fs_mp_hdr_t *hdr);

    /*
 * 获取buddy block
 *
 * buddy算法核心:
 *
 *      buddy = offset ^ block_size
 */
    void *fs_mp_buddy_ptr(fs_mempool_t *mp, void *ptr, uint32_t order);

    /*
 * free list push
 *
 * 插入指定order空闲链表。
 */
    void fs_mp_push_block(fs_mempool_t *mp, uint32_t order, void *ptr);

    /*
 * free list pop
 *
 * 从指定order取出一个block。
 */
    void *fs_mp_pop_block(fs_mempool_t *mp, uint32_t order);

    /*
 * 从free list移除指定block
 *
 * 用于:
 *      buddy merge
 */
    bool fs_mp_remove_block(fs_mempool_t *mp, uint32_t order, void *ptr);

#ifdef __cplusplus
}
#endif
