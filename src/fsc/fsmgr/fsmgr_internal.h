#pragma once

#include "common/fs_common.h"
#include "fsc/fstable/fstable.h"

/*
 * ============================================================
 * manager
 * ============================================================
 */

typedef struct fsc_manager
{
    fsc_table_t table;
    fs_mutex_t lock;
    fs_atomic32_t namespace_count;

} fsc_manager_t;

extern fsc_manager_t g_fsmgr;
