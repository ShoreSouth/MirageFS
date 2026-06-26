

#include <string.h>
#include <errno.h>

#include "common//fs_common.h"
#include "object/obj_error.h"
#include "object/objpool/objpool.h"

/*
 * ============================================================
 * 默认配置
 * ============================================================
 */

/*
 * 默认对象池容量。
 *
 * 当前支持约 4096 个 ObjMeta。
 * 后续可改为配置文件读取。
 */
#define OBJPOOL_DEFAULT_OBJECTS    4096U

/*
 * ============================================================
 * 全局对象池
 * ============================================================
 */

typedef struct obj_pool
{
    
    fs_mempool_t *meta_pool; /* ObjMeta 专用内存池 */

} obj_pool_t;

static obj_pool_t g_objpool;

/*
 * ============================================================
 * 生命周期
 * ============================================================
 */

int32_t objpool_init(void)
{
    fs_error_t err;

    fs_mp_config_t cfg;

    FS_LOG_DUMP_INFO("enter");

    memset(&cfg, 0, sizeof(cfg));

    cfg.total_size =
            sizeof(obj_meta_t) *
            OBJPOOL_DEFAULT_OBJECTS;

    cfg.max_order = FS_MP_MAX_ORDER;

    cfg.flags = FS_MP_F_THREAD_SAFE;

    g_objpool.meta_pool =
            fs_mp_create(&cfg);

    if (g_objpool.meta_pool == NULL) {

        err = obj_error(
                    OBJ_SUB_INIT,
                    ENOMEM);

        FS_LOG_DUMP_ERROR(
                "create obj meta pool failed, err=%s (0x%x)",
                fs_error_str(err),
                err);

        return err;
    }

    FS_LOG_DUMP_INFO(
            "exit: ok");

    return FS_OK;
}

void objpool_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    if (g_objpool.meta_pool != NULL) {

        fs_mp_destroy(
                g_objpool.meta_pool);

        g_objpool.meta_pool = NULL;
    }

    FS_LOG_DUMP_INFO("exit");
}

/*
 * ============================================================
 * 对象申请 / 释放
 * ============================================================
 */

obj_meta_t *objpool_alloc(void)
{
    fs_error_t err;

    obj_meta_t *meta;

    FS_LOG_DUMP_INFO("enter");

    if (g_objpool.meta_pool == NULL) {

        err = obj_error(
                    OBJ_SUB_CREATE,
                    EINVAL);

        FS_LOG_DUMP_ERROR(
                "obj pool not initialized, err=%s (0x%x)",
                fs_error_str(err),
                err);

        return NULL;
    }

    meta = fs_mp_calloc(
                g_objpool.meta_pool,
                1,
                sizeof(*meta));

    if (meta == NULL) {

        err = obj_error(
                    OBJ_SUB_CREATE,
                    ENOMEM);

        FS_LOG_DUMP_ERROR(
                "allocate obj meta failed, err=%s (0x%x)",
                fs_error_str(err),
                err);

        return NULL;
    }

    FS_LOG_DUMP_INFO(
            "exit: meta=%p",
            (void *)meta);

    return meta;
}

void objpool_free(
                obj_meta_t *meta)
{
    FS_LOG_DUMP_INFO(
            "enter: meta=%p",
            (void *)meta);

    if (meta == NULL) {

        FS_LOG_DUMP_INFO(
                "exit: meta is NULL");

        return;
    }

    fs_mp_free(
            g_objpool.meta_pool,
            meta);

    FS_LOG_DUMP_INFO("exit");
}