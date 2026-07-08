#pragma once

#include "common/macros/fs_defs.h"
#include "common/error/fs_common_sub.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 基础配置
 * ============================================================ */

#define FS_MP_PAGE_SIZE          4096U

#define FS_MP_MIN_ORDER          0
#define FS_MP_MAX_ORDER          16

/*
 * 最大块:
 *
 * 4KB << 16 = 256MB
 */

/* ============================================================
 * 调试魔数
 * ============================================================ */

#define FS_MP_MAGIC_ALLOC        0x4D50414CUL  /* MPAL */
#define FS_MP_MAGIC_FREE         0x4D504652UL  /* MPFR */

/* ============================================================
 * Poison
 * ============================================================ */

#define FS_MP_POISON_ALLOC       0xAA
#define FS_MP_POISON_FREE        0xDD

/* ============================================================
 * Flags
 * ============================================================ */

#define FS_MP_F_POISON           (1U << 0)
#define FS_MP_F_VERIFY           (1U << 1)
#define FS_MP_F_STATS            (1U << 2)
#define FS_MP_F_THREAD_SAFE      (1U << 3)

/* ============================================================
 * 前向声明
 * ============================================================ */

typedef struct fs_mempool fs_mempool_t;
typedef struct fs_slab_cache fs_slab_cache_t;

/* ============================================================
 * Allocation Header
 * ============================================================ */

/*
 * 每个分配块前面的头部
 *
 * 用户不可见
 */
typedef struct fs_mp_hdr {

    uint32_t magic;

    uint16_t order; /* buddy order */

    uint16_t flags;

    uint64_t req_size; /* 用户申请大小 */

    /*
     * buddy block 起点到 header 的偏移。
     *
     * 普通分配时为 0；对齐分配时 header 可能位于 block 内部，
     * free 时依靠该字段找回原始 buddy block。
     */
    uint64_t block_offset;


} fs_mp_hdr_t;

/* ============================================================
 * 配置
 * ============================================================ */

typedef struct fs_mp_config {
    /*
     * 总池大小
     *
     * 必须:
     *      PAGE_SIZE * 2^N
     */
    uint64_t total_size;

    /*
     * 最大伙伴阶
     */
    uint32_t max_order;

    /*
     * FS_MP_F_XXX
     */
    uint32_t flags;

} fs_mp_config_t;

/* ============================================================
 * 统计信息
 * ============================================================ */

typedef struct fs_mp_stats {

    uint64_t total_bytes;

    uint64_t used_bytes;
    uint64_t free_bytes;

    uint64_t alloc_count;
    uint64_t free_count;

    uint64_t alloc_fail_count;

    uint64_t page_count;

    uint64_t split_count;
    uint64_t merge_count;

    uint64_t current_allocs;

    uint64_t peak_used_bytes;

} fs_mp_stats_t;

/* ============================================================
 * 生命周期
 * ============================================================ */

/*
 * 使用配置创建
 */
fs_mempool_t *fs_mp_create(const fs_mp_config_t *cfg);

/*
 * 销毁
 */
void fs_mp_destroy(fs_mempool_t *mp);

/*
 * 全局池初始化
 */
fs_error_t fs_mp_global_init(const fs_mp_config_t *cfg);

/*
 * 全局池销毁
 */
void fs_mp_global_fini(void);

/*
 * 获取全局池
 */
fs_mempool_t *fs_mp_global(void);

/* ============================================================
 * 分配接口
 * ============================================================ */

/*
 * 分配
 */
void *fs_mp_alloc(fs_mempool_t *mp,
            size_t size);

/*
 * 对齐分配
 *
 * align:
 *      必须为2幂
 */
void *fs_mp_alloc_align(fs_mempool_t *mp,
                  size_t size,
                  size_t align);

/*
 * calloc
 */
void *fs_mp_calloc(fs_mempool_t *mp,
             size_t n,
             size_t size);

/*
 * realloc
 */
void *fs_mp_realloc(fs_mempool_t *mp,
              void *ptr,
              size_t new_size);

/*
 * 释放
 */
void fs_mp_free(fs_mempool_t *mp,
           void *ptr);

/*
 * 获取真实分配大小
 */
size_t fs_mp_usable_size(const void *ptr);

/* ============================================================
 * 全局快捷接口
 * ============================================================ */

void *fs_malloc(size_t size);

void *fs_zalloc(size_t size);

void *fs_realloc(void *ptr,
           size_t new_size);

void fs_free(void *ptr);

/* ============================================================
 * 调试接口
 * ============================================================ */

/*
 * 获取统计
 */
void fs_mp_get_stats(fs_mempool_t *mp,
                fs_mp_stats_t *stats);

/*
 * 打印状态
 */
void fs_mp_dump(fs_mempool_t *mp);

/*
 * 完整一致性检查
 */
bool fs_mp_verify(fs_mempool_t *mp);

/*
 * 指针是否属于该池
 */
bool fs_mp_contains(fs_mempool_t *mp,
               const void *ptr);

/*
 * 检查是否已释放
 */
bool fs_mp_is_freed(const void *ptr);

/* ============================================================
 * Slab（未来扩展）
 * ============================================================ */

/*
 * 创建对象缓存
 */
fs_slab_cache_t *fs_slab_create(const char *name,
               size_t obj_size);

/*
 * 销毁对象缓存
 */
void fs_slab_destroy(fs_slab_cache_t *cache);

/*
 * 分配对象
 */
void *fs_slab_alloc(fs_slab_cache_t *cache);

/*
 * 释放对象
 */
void fs_slab_free(fs_slab_cache_t *cache,
             void *ptr);

#ifdef __cplusplus
}
#endif
