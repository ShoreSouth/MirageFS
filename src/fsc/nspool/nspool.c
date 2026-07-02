#include "fsc/nspool/nspool.h"

#include <string.h>

#include "fsc/fsc_error.h"

/*
 * ============================================================
 * default config
 * ============================================================
 */

#define NSPOOL_DEFAULT_NAMESPACES 64U

/*
 * ============================================================
 * global pool
 * ============================================================
 */

typedef struct ns_pool {

    fs_mempool_t *namespace_pool;

} ns_pool_t;

static ns_pool_t g_nspool;

/*
 * ============================================================
 * lifecycle
 * ============================================================
 */

fs_error_t nspool_init(void)
{
    fs_error_t err;
    fs_mp_config_t cfg;

    FS_LOG_DUMP_INFO("enter");

    memset(&cfg, 0, sizeof(cfg));

    cfg.total_size = sizeof(fsc_namespace_t) * NSPOOL_DEFAULT_NAMESPACES;
    cfg.max_order = FS_MP_MAX_ORDER;
    cfg.flags = FS_MP_F_THREAD_SAFE;

    g_nspool.namespace_pool = fs_mp_create(&cfg);
    if (g_nspool.namespace_pool == NULL) {
        err = fsc_error(FSC_SUB_NSPOOL, FS_ERRNO_ENOMEM);
        FS_LOG_DUMP_ERROR("create namespace pool failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void nspool_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    if (g_nspool.namespace_pool != NULL) {
        fs_mp_destroy(g_nspool.namespace_pool);
        g_nspool.namespace_pool = NULL;
    }

    FS_LOG_DUMP_INFO("exit: done");
}

/*
 * ============================================================
 * allocation
 * ============================================================
 */

fsc_namespace_t *nspool_alloc(void)
{
    fs_error_t err;
    fsc_namespace_t *ns;

    FS_LOG_DUMP_INFO("enter");

    if (g_nspool.namespace_pool == NULL) {
        err = fsc_error(FSC_SUB_ALLOC, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("namespace pool not initialized, err=%s (0x%x)",
                          fs_error_str(err), err);
        return NULL;
    }

    ns = fs_mp_calloc(g_nspool.namespace_pool, 1, sizeof(*ns));
    if (ns == NULL) {
        err = fsc_error(FSC_SUB_ALLOC, FS_ERRNO_ENOMEM);
        FS_LOG_DUMP_ERROR("allocate namespace failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return NULL;
    }

    FS_LOG_DUMP_INFO("exit: ns=%p", (void *)ns);
    return ns;
}

void nspool_free(
                fsc_namespace_t *ns)
{
    FS_LOG_DUMP_INFO("enter: ns=%p", (void *)ns);

    if (ns == NULL) {
        FS_LOG_DUMP_INFO("exit: ns is NULL");
        return;
    }

    fsc_namespace_deinit(ns);
    fs_mp_free(g_nspool.namespace_pool, ns);

    FS_LOG_DUMP_INFO("exit: done");
}
