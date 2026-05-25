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

    mp = calloc(1, sizeof(*mp));

    if (mp == NULL) {

        FS_LOG_DUMP_ERROR("mempool calloc failed");

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

    if (fs_mutex_init(&mp->lock,
                      "mempool",
                      FS_LOCK_F_DEBUG) != 0) {

        FS_LOG_DUMP_ERROR("mempool lock init failed");

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

    FS_LOG_DUMP_INFO("mempool create success "
                "size=%lu "
                "max_order=%u",
                cfg->total_size,
                cfg->max_order);

    return mp;
}

void fs_mp_destroy(fs_mempool_t *mp)
{
    if (mp == NULL) {
        return;
    }

    FS_LOG_DUMP_INFO("mempool destroy");

    fs_mutex_destroy(&mp->lock);

    free(mp->base);

    memset(mp, 0, sizeof(*mp));

    free(mp);
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

    if (mp == NULL || size == 0) {
        return NULL;
    }

    order = fs_mp_calc_order(size);

    if (order == UINT32_MAX) {

        FS_LOG_DUMP_WARN("alloc too large size=%zu",
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

        FS_LOG_DUMP_WARN("mempool alloc failed size=%zu",
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

    return block;
}

void fs_mp_free(fs_mempool_t *mp,
           void *ptr)
{
    fs_mp_hdr_t *hdr;

    uint32_t order;
    uint32_t origin_order;

    void *block;
    void *buddy;

    if (mp == NULL || ptr == NULL) {
        return;
    }

    hdr = fs_mp_ptr_to_hdr(ptr);

    FS_ASSERT_MSG(hdr->magic == FS_MP_MAGIC_ALLOC ||
                  hdr->magic == FS_MP_MAGIC_FREE,
                  "invalid mempool magic");

    if (hdr->magic == FS_MP_MAGIC_FREE) {

        FS_LOG_DUMP_WARN("double free detected ptr=%p",
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
}

/* ============================================================
 * Global Pool
 * ============================================================ */

int fs_mp_global_init(const fs_mp_config_t *cfg)
{
    g_mp = fs_mp_create(cfg);

    return (g_mp != NULL) ? 0 : -1;
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

    FS_LOG_DUMP_INFO("total_bytes=%lu",
                mp->stats.total_bytes);

    FS_LOG_DUMP_INFO("used_bytes=%lu",
                mp->stats.used_bytes);

    FS_LOG_DUMP_INFO("free_bytes=%lu",
                mp->stats.free_bytes);

    FS_LOG_DUMP_INFO("alloc_count=%lu",
                mp->stats.alloc_count);

    FS_LOG_DUMP_INFO("free_count=%lu",
                mp->stats.free_count);

    FS_LOG_DUMP_INFO("current_allocs=%lu",
                mp->stats.current_allocs);

    FS_LOG_DUMP_INFO("split_count=%lu",
                mp->stats.split_count);

    FS_LOG_DUMP_INFO("merge_count=%lu",
                mp->stats.merge_count);

    for (i = 0; i <= mp->max_order; i++) {

        count = 0;

        FS_LIST_FOR_EACH(pos, &mp->free_area[i]) {
            count++;
        }

        FS_LOG_DUMP_INFO("order=%u "
                    "block_size=%lu "
                    "free_count=%u",
                    i,
                    FS_MP_ORDER_SIZE(i),
                    count);
    }

    FS_MP_UNLOCK(mp);
}