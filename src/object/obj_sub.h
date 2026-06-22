#pragma once

#include <stdint.h>

/*
 * ============================================================
 * Object Layer 子模块操作
 *
 * 用于 FS_ERR 的 sub 字段，
 * 标识 Object Layer 内部具体执行的操作。
 * ============================================================
 */

typedef enum obj_sub
{
    OBJ_SUB_NONE = 0,

    /*
     * 初始化。
     */
    OBJ_SUB_INIT,

    /*
     * 对象生命周期。
     */
    OBJ_SUB_CREATE,
    OBJ_SUB_DELETE,

    /*
     * 对象查找。
     */
    OBJ_SUB_LOOKUP,

    /*
     * 对象表管理。
     */
    OBJ_SUB_INSERT,
    OBJ_SUB_REMOVE,

    /*
     * 引用计数。
     */
    OBJ_SUB_GET,
    OBJ_SUB_PUT,
    OBJ_SUB_ACQUIRE,
    OBJ_SUB_RELEASE,

    /*
     * 生命周期状态。
     */
    OBJ_SUB_STATE,

} obj_sub_t;