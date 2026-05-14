#pragma once

#include "common/mempool/fs_mempool_api.h"

#include <pthread.h>

typedef struct fs_mp_block {
    struct fs_mp_block *next;
} fs_mp_block_t;

typedef struct fs_mempool {

    void *base;

    uint64_t total_size;

    uint32_t max_order;

    uint32_t flags;

    fs_mp_block_t *free_area[FS_MP_MAX_ORDER + 1];

    fs_mp_stats_t stats;

    pthread_mutex_t lock;
} fs_mempool_t;