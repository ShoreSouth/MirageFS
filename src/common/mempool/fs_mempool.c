#include "common/mempool/fs_mempool_internal.h"
#include "common/mempool/fs_mempool.h"
#include "common/list/fs_list.h"

#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Global Mempool
 * ============================================================ */

static fs_mempool_t *g_mp = NULL;

/* ============================================================
 * Internal Helpers
 * ============================================================ */

/*
 * size -> buddy order
 */
uint32_t fs_mp_calc_order(size_t size)
{
    uint64_t need_size;
    uint32_t order;

    need_size = size + sizeof(fs_mp_hdr_t);

    for (order = FS_MP_MIN_ORDER;
         order <= FS_MP_MAX_ORDER;
         order++) {

        if (FS_MP_ORDER_SIZE(order) >= need_size) {
            return order;
        }
    }

    return UINT32_MAX;
}

/*
 * user ptr -> hdr
 */
fs_mp_hdr_t *fs_mp_ptr_to_hdr(void *ptr)
{
    return ((fs_mp_hdr_t *)ptr - 1);
}

/*
 * hdr -> user ptr
 */
void *fs_mp_hdr_to_ptr(fs_mp_hdr_t *hdr)
{
    return (void *)(hdr + 1);
}

/*
 * ptr -> offset
 */
static inline uintptr_t fs_mp_offset(fs_mempool_t *mp,
             void *ptr)
{
    return ((uintptr_t)ptr - (uintptr_t)mp->base);
}

/*
 * 获取buddy block
 *
 * buddy = offset ^ block_size
 */
void *fs_mp_buddy_ptr(fs_mempool_t *mp,
                void *ptr,
                uint32_t order)
{
    uintptr_t offset;
    uintptr_t buddy_offset;

    offset = fs_mp_offset(mp, ptr);

    buddy_offset = offset ^ FS_MP_ORDER_SIZE(order);

    return ((uint8_t *)mp->base + buddy_offset);
}

/* ============================================================
 * Free List Helpers
 * ============================================================ */

/*
 * 插入free list
 */
void fs_mp_push_block(fs_mempool_t *mp,
                 uint32_t order,
                 void *ptr)
{
    fs_mp_block_t *block;

    block = (fs_mp_block_t *)ptr;

    fs_list_add(&block->node,
                &mp->free_area[order]);
}

/*
 * 弹出free block
 */
void *fs_mp_pop_block(fs_mempool_t *mp,
                uint32_t order)
{
    fs_list_head_t *head;
    fs_mp_block_t *block;

    head = &mp->free_area[order];

    if (fs_list_empty(head)) {
        return NULL;
    }

    block = FS_LIST_ENTRY(head->next,
                          fs_mp_block_t,
                          node);

    fs_list_del(&block->node);

    return (void *)block;
}

/*
 * 从free list移除指定block
 */
bool fs_mp_remove_block(fs_mempool_t *mp,
                   uint32_t order,
                   void *ptr)
{
    fs_list_head_t *pos, *next;
    fs_mp_block_t *block;

    FS_LIST_FOR_EACH_SAFE(pos, next, &mp->free_area[order]) {

        block = FS_LIST_ENTRY(pos,
                              fs_mp_block_t,
                              node);

        if ((void *)block == ptr) {

            fs_list_del(&block->node);

            return true;
        }
    }

    return false;
}

/* ============================================================
 * Lifecycle
 * ============================================================ */

fs_mempool_t *fs_mp_create(const fs_mp_config_t *cfg)
{
    fs_mempool_t *mp;
    uint32_t i;

    FS_ASSERT(cfg != NULL);

    FS_LOG_DUMP_INFO("enter: total_size=%llu max_order=%u",
                     (unsigned long long)cfg->total_size,
                     cfg->max_order);

    mp = calloc(1, sizeof(*mp));

    if (mp == NULL) {

        FS_LOG_DUMP_ERROR("calloc mp failed");

        return NULL;
    }

    mp->base = aligned_alloc(FS_MP_PAGE_SIZE,
                             cfg->total_size);

    if (mp->base == NULL) {

        FS_LOG_DUMP_ERROR("aligned_alloc failed");

        free(mp);

        return NULL;
    }

    mp->total_size = cfg->total_size;
    mp->max_order  = cfg->max_order;
    mp->flags      = cfg->flags;

    for (i = 0; i <= FS_MP_MAX_ORDER; i++) {
        fs_list_init(&mp->free_area[i]);
    }

    if (fs_failed(fs_mutex_init(&mp->lock,
                                "mempool",
                                FS_LOCK_F_DEBUG))) {

        FS_LOG_DUMP_ERROR("mutex init failed");

        free(mp->base);
        free(mp);

        return NULL;
    }

    /*
     * 初始整个pool作为一个最大块
     */
    fs_mp_push_block(mp,
                     mp->max_order,
                     mp->base);

    mp->stats.total_bytes = cfg->total_size;
    mp->stats.free_bytes  = cfg->total_size;

    FS_LOG_DUMP_INFO("exit: ok total_size=%llu max_order=%u",
                     (unsigned long long)cfg->total_size,
                     cfg->max_order);

    return mp;
}

void fs_mp_destroy(fs_mempool_t *mp)
{
    FS_LOG_DUMP_INFO("enter: mp=%p", (void *)mp);

    if (mp == NULL) {
        return;
    }

    fs_mutex_destroy(&mp->lock);

    free(mp->base);

    memset(mp, 0, sizeof(*mp));

    free(mp);

    FS_LOG_DUMP_INFO("exit: done");
}

/* ============================================================
 * Allocation
 * ============================================================ */

void *fs_mp_alloc(fs_mempool_t *mp,
            size_t size)
{
    uint32_t order;
    uint32_t cur_order;

    void *block;
    void *buddy;

    fs_mp_hdr_t *hdr;

    FS_LOG_DUMP_INFO("enter: mp=%p size=%zu",
                     (void *)mp, size);

    if (mp == NULL || size == 0) {
        return NULL;
    }

    order = fs_mp_calc_order(size);

    if (order == UINT32_MAX) {

        FS_LOG_DUMP_WARN("size too large size=%zu",
                    size);

        return NULL;
    }

    FS_MP_LOCK(mp);

    /*
     * 找可用块
     */
    for (cur_order = order;
         cur_order <= mp->max_order;
         cur_order++) {

        block = fs_mp_pop_block(mp, cur_order);

        if (block != NULL) {
            break;
        }
    }

    if (cur_order > mp->max_order) {

        mp->stats.alloc_fail_count++;

        FS_MP_UNLOCK(mp);

        FS_LOG_DUMP_WARN("alloc failed size=%zu",
                    size);

        return NULL;
    }

    /*
     * split
     */
    while (cur_order > order) {

        cur_order--;

        buddy = ((uint8_t *)block +
                 FS_MP_ORDER_SIZE(cur_order));

        fs_mp_push_block(mp,
                         cur_order,
                         buddy);

        mp->stats.split_count++;
    }

    hdr = (fs_mp_hdr_t *)block;

    hdr->magic    = FS_MP_MAGIC_ALLOC;
    hdr->order    = order;
    hdr->flags    = 0;
    hdr->req_size = size;

    block = fs_mp_hdr_to_ptr(hdr);

    if (mp->flags & FS_MP_F_POISON) {

        memset(block,
               FS_MP_POISON_ALLOC,
               size);
    }

    mp->stats.alloc_count++;
    mp->stats.used_bytes += FS_MP_ORDER_SIZE(order);
    mp->stats.free_bytes -= FS_MP_ORDER_SIZE(order);

    mp->stats.current_allocs++;

    if (mp->stats.used_bytes >
        mp->stats.peak_used_bytes) {

        mp->stats.peak_used_bytes =
            mp->stats.used_bytes;
    }

    FS_MP_UNLOCK(mp);

    FS_LOG_DUMP_INFO("exit: ptr=%p order=%u",
                     block, order);

    return block;
}

void *fs_mp_alloc_align(fs_mempool_t *mp,
                  size_t size,
                  size_t align)
{
    uint32_t order;
    uint32_t order_align;

    size_t real_size;

    uint64_t block_size;

    if (mp == NULL || size == 0) {

        FS_LOG_DUMP_WARN("alloc_align: invalid param "
                    "mp=%p size=%zu align=%zu",
                    (void *)mp, size, align);

        return NULL;
    }

    if ((align & (align - 1)) != 0) {

        FS_LOG_DUMP_WARN("alloc_align: align not power of 2 "
                    "align=%zu",
                    align);

        return NULL;
    }

    order = fs_mp_calc_order(size);

    if (order == UINT32_MAX) {

        FS_LOG_DUMP_WARN("alloc_align: size too large "
                    "size=%zu",
                    size);

        return NULL;
    }

    /*
     * Buddy block of order N is naturally aligned to
     * PAGE_SIZE << N. If align exceeds the block size
     * that order would provide, raise the order.
     */
    if (align > FS_MP_PAGE_SIZE) {

        block_size = FS_MP_ORDER_SIZE(order);

        if (align > block_size) {

            order_align = 0;

            block_size = FS_MP_PAGE_SIZE;

            while (block_size < align &&
                   order_align < FS_MP_MAX_ORDER) {

                order_align++;
                block_size <<= 1;
            }

            if (order_align > FS_MP_MAX_ORDER) {

                FS_LOG_DUMP_WARN("alloc_align: "
                            "align too large "
                            "align=%zu",
                            align);

                return NULL;
            }

            if (order_align > order) {
                order = order_align;
            }
        }

        /*
         * Enforce alignment via overallocation.
         *
         * Allocate enough extra that an
         * aligned address within the block
         * is guaranteed.
         */
        real_size = FS_MP_ORDER_SIZE(order) -
                    sizeof(fs_mp_hdr_t);

        if (align > real_size) {

            order++;

            if (order > FS_MP_MAX_ORDER) {

                FS_LOG_DUMP_WARN("alloc_align: "
                            "align too large "
                            "align=%zu",
                            align);

                return NULL;
            }
        }
    }

    return fs_mp_alloc(mp, size);
}

void *fs_mp_calloc(fs_mempool_t *mp,
             size_t n,
             size_t size)
{
    size_t total;

    void *ptr;

    FS_LOG_DUMP_INFO("enter: mp=%p n=%zu size=%zu",
                     (void *)mp, n, size);

    if (mp == NULL || n == 0 || size == 0) {

        FS_LOG_DUMP_WARN("calloc: invalid param");

        return NULL;
    }

    if (n > SIZE_MAX / size) {

        FS_LOG_DUMP_WARN("calloc: overflow "
                    "n=%zu size=%zu",
                    n, size);

        return NULL;
    }

    total = n * size;

    ptr = fs_mp_alloc(mp, total);

    if (ptr == NULL) {

        FS_LOG_DUMP_WARN("calloc: alloc failed");

        return NULL;
    }

    memset(ptr, 0, total);

    FS_LOG_DUMP_INFO("exit: ptr=%p", ptr);

    return ptr;
}

void *fs_mp_realloc(fs_mempool_t *mp,
              void *ptr,
              size_t new_size)
{
    void *new_ptr;

    fs_mp_hdr_t *hdr;

    size_t copy_size;

    FS_LOG_DUMP_INFO("enter: mp=%p ptr=%p new_size=%zu",
                     (void *)mp, ptr, new_size);

    if (ptr == NULL) {

        new_ptr = fs_mp_alloc(mp, new_size);

        FS_LOG_DUMP_INFO("exit: alloc new ptr=%p",
                         new_ptr);

        return new_ptr;
    }

    if (new_size == 0) {

        fs_mp_free(mp, ptr);

        FS_LOG_DUMP_INFO("exit: freed");

        return NULL;
    }

    hdr = fs_mp_ptr_to_hdr(ptr);

    copy_size = hdr->req_size;

    if (copy_size > new_size) {
        copy_size = new_size;
    }

    new_ptr = fs_mp_alloc(mp, new_size);

    if (new_ptr == NULL) {

        FS_LOG_DUMP_WARN("realloc: alloc failed");

        return NULL;
    }

    memcpy(new_ptr, ptr, copy_size);

    fs_mp_free(mp, ptr);

    FS_LOG_DUMP_INFO("exit: new_ptr=%p", new_ptr);

    return new_ptr;
}

void fs_mp_free(fs_mempool_t *mp,
           void *ptr)
{
    fs_mp_hdr_t *hdr;

    uint32_t order;
    uint32_t origin_order;

    void *block;
    void *buddy;

    FS_LOG_DUMP_INFO("enter: mp=%p ptr=%p",
                     (void *)mp, ptr);

    if (mp == NULL || ptr == NULL) {
        return;
    }

    hdr = fs_mp_ptr_to_hdr(ptr);

    FS_ASSERT_MSG(hdr->magic == FS_MP_MAGIC_ALLOC ||
                  hdr->magic == FS_MP_MAGIC_FREE,
                  "invalid mempool magic");

    if (hdr->magic == FS_MP_MAGIC_FREE) {

        FS_LOG_DUMP_WARN("double free ptr=%p",
                         ptr);

        FS_ASSERT(false);

        return;
    }

    order = hdr->order;
    origin_order = order;

    block = (void *)hdr;

    FS_MP_LOCK(mp);

    if (mp->flags & FS_MP_F_POISON) {

        memset(ptr,
               FS_MP_POISON_FREE,
               hdr->req_size);
    }

    hdr->magic = FS_MP_MAGIC_FREE;

    /*
     * buddy merge
     */
    while (order < mp->max_order) {

        buddy = fs_mp_buddy_ptr(mp,
                                block,
                                order);

        if (!fs_mp_remove_block(mp,
                                order,
                                buddy)) {
            break;
        }

        /*
         * 取较低地址作为merge后block
         */
        if (buddy < block) {
            block = buddy;
        }

        order++;

        mp->stats.merge_count++;
    }

    fs_mp_push_block(mp,
                     order,
                     block);

    mp->stats.free_count++;

    mp->stats.used_bytes -=
        FS_MP_ORDER_SIZE(origin_order);

    mp->stats.free_bytes +=
        FS_MP_ORDER_SIZE(origin_order);

    mp->stats.current_allocs--;

    FS_MP_UNLOCK(mp);

    FS_LOG_DUMP_INFO("exit: done");
}

size_t fs_mp_usable_size(const void *ptr)
{
    fs_mp_hdr_t *hdr;

    if (ptr == NULL) {
        return 0;
    }

    hdr = fs_mp_ptr_to_hdr((void *)ptr);

    return FS_MP_ORDER_SIZE(hdr->order) -
           sizeof(fs_mp_hdr_t);
}

/* ============================================================
 * Global Pool
 * ============================================================ */

fs_error_t fs_mp_global_init(const fs_mp_config_t *cfg)
{
    g_mp = fs_mp_create(cfg);

    return (g_mp != NULL) ? FS_OK :
           fs_common_error(FS_COMMON_SUB_MEMPOOL, FS_ERRNO_ENOMEM);
}

void fs_mp_global_fini(void)
{
    fs_mp_destroy(g_mp);

    g_mp = NULL;
}

fs_mempool_t *fs_mp_global(void)
{
    return g_mp;
}

/* ============================================================
 * Global Wrappers
 * ============================================================ */

void *fs_malloc(size_t size)
{
    return fs_mp_alloc(g_mp, size);
}

void *fs_zalloc(size_t size)
{
    void *ptr;

    ptr = fs_malloc(size);

    if (ptr != NULL) {
        memset(ptr, 0, size);
    }

    return ptr;
}

void *fs_realloc(void *ptr,
           size_t new_size)
{
    void *new_ptr;
    fs_mp_hdr_t *hdr;
    size_t copy_size;

    if (ptr == NULL) {
        return fs_malloc(new_size);
    }

    if (new_size == 0) {

        fs_free(ptr);

        return NULL;
    }

    hdr = fs_mp_ptr_to_hdr(ptr);

    copy_size = hdr->req_size;

    if (copy_size > new_size) {
        copy_size = new_size;
    }

    new_ptr = fs_malloc(new_size);

    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr,
           ptr,
           copy_size);

    fs_free(ptr);

    return new_ptr;
}

void fs_free(void *ptr)
{
    fs_mp_free(g_mp, ptr);
}

/* ============================================================
 * Debug APIs
 * ============================================================ */

void fs_mp_get_stats(fs_mempool_t *mp,
                fs_mp_stats_t *stats)
{
    if (mp == NULL || stats == NULL) {
        return;
    }

    FS_MP_LOCK(mp);

    *stats = mp->stats;

    FS_MP_UNLOCK(mp);
}

bool fs_mp_contains(fs_mempool_t *mp,
               const void *ptr)
{
    uintptr_t start;
    uintptr_t end;
    uintptr_t addr;

    if (mp == NULL || ptr == NULL) {
        return false;
    }

    start = (uintptr_t)mp->base;
    end   = start + mp->total_size;

    addr = (uintptr_t)ptr;

    return (addr >= start && addr < end);
}

bool fs_mp_is_freed(const void *ptr)
{
    fs_mp_hdr_t *hdr;

    if (ptr == NULL) {
        return false;
    }

    hdr = fs_mp_ptr_to_hdr((void *)ptr);

    return (hdr->magic == FS_MP_MAGIC_FREE);
}

void fs_mp_dump(fs_mempool_t *mp)
{
    uint32_t i;
    uint32_t count;

    fs_list_head_t *pos;

    if (mp == NULL) {
        return;
    }

    FS_MP_LOCK(mp);

    FS_LOG_DUMP_INFO("========== mempool dump ==========");

    FS_LOG_DUMP_INFO("total_bytes=%llu",
                (unsigned long long)mp->stats.total_bytes);

    FS_LOG_DUMP_INFO("used_bytes=%llu",
                (unsigned long long)mp->stats.used_bytes);

    FS_LOG_DUMP_INFO("free_bytes=%llu",
                (unsigned long long)mp->stats.free_bytes);

    FS_LOG_DUMP_INFO("alloc_count=%llu",
                (unsigned long long)mp->stats.alloc_count);

    FS_LOG_DUMP_INFO("free_count=%llu",
                (unsigned long long)mp->stats.free_count);

    FS_LOG_DUMP_INFO("current_allocs=%llu",
                (unsigned long long)mp->stats.current_allocs);

    FS_LOG_DUMP_INFO("split_count=%llu",
                (unsigned long long)mp->stats.split_count);

    FS_LOG_DUMP_INFO("merge_count=%llu",
                (unsigned long long)mp->stats.merge_count);

    for (i = 0; i <= mp->max_order; i++) {

        count = 0;

        FS_LIST_FOR_EACH(pos, &mp->free_area[i]) {
            count++;
        }

        FS_LOG_DUMP_INFO("order=%u "
                    "block_size=%llu "
                    "free_count=%u",
                    i,
                    (unsigned long long)FS_MP_ORDER_SIZE(i),
                    count);
    }

    FS_MP_UNLOCK(mp);
}

bool fs_mp_verify(fs_mempool_t *mp)
{
    uint32_t i;

    fs_list_head_t *pos;

    fs_mp_block_t *block;

    if (mp == NULL) {
        return false;
    }

    FS_MP_LOCK(mp);

    for (i = 0; i <= mp->max_order; i++) {

        FS_LIST_FOR_EACH(pos, &mp->free_area[i]) {

            block = FS_LIST_ENTRY(pos,
                                  fs_mp_block_t,
                                  node);

            if (!fs_mp_contains(mp, block)) {

                FS_MP_UNLOCK(mp);

                FS_LOG_DUMP_ERROR(
                        "verify: block %p "
                        "not in pool range",
                        (void *)block);

                return false;
            }
        }
    }

    FS_MP_UNLOCK(mp);

    return true;
}