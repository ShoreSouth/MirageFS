#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "common/error/fs_common_sub.h"

/* ============================================================
 *  数值工具
 * ============================================================ */

static inline uint64_t fs_min_u64(uint64_t a, uint64_t b)
{
    return (a < b) ? a : b;
}

static inline uint64_t fs_max_u64(uint64_t a, uint64_t b)
{
    return (a > b) ? a : b;
}

static inline uint32_t fs_min_u32(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

static inline uint32_t fs_max_u32(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}

/* ============================================================
 *  对齐相关（IO / SGL 核心）
 * ============================================================ */

static inline bool fs_is_aligned(uint64_t x, uint64_t align)
{
    return (x & (align - 1)) == 0;
}

static inline uint64_t fs_align_down(uint64_t x, uint64_t align)
{
    return x & ~(align - 1);
}

static inline uint64_t fs_align_up(uint64_t x, uint64_t align)
{
    return (x + align - 1) & ~(align - 1);
}

/* ============================================================
 *  区间处理
 * ============================================================ */

static inline bool fs_range_valid2(uint64_t offset,
    uint64_t len, uint64_t max_size)
{
    if (len == 0)
        return false;

    if (offset > UINT64_MAX - len)
        return false;

    if (offset + len > max_size)
        return false;

    return true;
}

static inline uint64_t fs_range_end(uint64_t offset, uint64_t len)
{
    return offset + len;
}

static inline bool fs_range_overlap(uint64_t o1, uint64_t l1,
                                   uint64_t o2, uint64_t l2)
{
    uint64_t e1 = o1 + l1;
    uint64_t e2 = o2 + l2;
    return !(e1 <= o2 || e2 <= o1);
}

/* ============================================================
 *  trim相关
 * ============================================================ */

typedef struct {
    uint64_t head_len;   // 被裁掉的头部长度
    uint64_t tail_len;   // 被裁掉的尾部长度
} fs_trim_info_t;

static inline fs_error_t fs_trim_to_aligned(uint64_t *offset,
    uint64_t *len, uint64_t align)
{
    uint64_t start = *offset;
    uint64_t length = *len;

    /* 无效输入 */
    if (length == 0)
        return FS_OK;

    /* 防止溢出 */
    if (start > UINT64_MAX - length)
        return fs_common_error(FS_COMMON_SUB_UTILS, FS_ERRNO_EOVERFLOW);

    uint64_t end = start + length;

    /* 对齐 */
    uint64_t aligned_start = fs_align_up(start, align);
    uint64_t aligned_end   = fs_align_down(end, align);

    /* 没有完整块 */
    if (aligned_start >= aligned_end) {
        *len = 0;
        return FS_OK;
    }

    *offset = aligned_start;
    *len    = aligned_end - aligned_start;

    return FS_OK;
}

static inline fs_error_t fs_trim_to_aligned_ex(uint64_t *offset,
                                        uint64_t *len,
                                        uint64_t align,
                                        fs_trim_info_t *info)
{
    uint64_t orig_start = *offset;
    uint64_t orig_end   = *offset + *len;

    if (*len == 0)
        return FS_OK;

    if (*offset > UINT64_MAX - *len)
        return fs_common_error(FS_COMMON_SUB_UTILS, FS_ERRNO_EOVERFLOW);

    uint64_t aligned_start = fs_align_up(orig_start, align);
    uint64_t aligned_end   = fs_align_down(orig_end, align);

    if (aligned_start >= aligned_end) {
        info->head_len = *len;
        info->tail_len = 0;
        *len = 0;
        return FS_OK;
    }

    info->head_len = aligned_start - orig_start;
    info->tail_len = orig_end - aligned_end;

    *offset = aligned_start;
    *len    = aligned_end - aligned_start;

    return FS_OK;
}

/* ============================================================
 *  字符串安全工具
 * ============================================================ */

static inline size_t fs_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);

    if (size > 0) {
        size_t copy = (len >= size) ? size - 1 : len;
        memcpy(dst, src, copy);
        dst[copy] = '\0';
    }

    return len;
}

/* ============================================================
 *  内存工具
 * ============================================================ */

static inline void fs_memzero(void *ptr, size_t size)
{
    memset(ptr, 0, size);
}

/* ============================================================
 *  Debug：buffer dump（非常好用）
 * ============================================================ */

static inline void fs_dump_hex(const void *buf, size_t len)
{
    const unsigned char *p = (const unsigned char *)buf;

    for (size_t i = 0; i < len; i++) {
        printf("%02X ", p[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    if (len % 16 != 0)
        printf("\n");
}
