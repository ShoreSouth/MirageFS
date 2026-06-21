#pragma once

#include <stdbool.h>

#include "object/fuid/fuid.h"

/* ============================================================
 * 核心结构
 * ============================================================ */

/*
 * obj_key_t
 *
 * MirageFS 对象唯一标识。
 *
 * (objectid, gen) 共同构成对象 identity，
 * 用于 ObjTable 的 key 查找和对象生命周期追踪。
 */
typedef struct objkey {

    ObjectId_t objectid;

    GenId_t gen;

} obj_key_t;

/* ============================================================
 * helper
 * ============================================================ */

/*
 * 构造 obj_key_t。
 */
static inline obj_key_t objkey_make(
                        ObjectId_t objectid,
                        GenId_t gen)
{
    obj_key_t key;

    key.objectid = objectid;
    key.gen      = gen;

    return key;
}

/*
 * 判断 key 是否有效。
 *
 * objectid 和 gen 均不能为 0。
 */
static inline bool objkey_valid(
                        const obj_key_t *key)
{
    if (key == NULL) {
        return false;
    }

    if (key->objectid == 0) {
        return false;
    }

    if (key->gen == 0) {
        return false;
    }

    return true;
}

/*
 * 比较两个 key 是否相同。
 *
 * 仅比较 (objectid, gen)。
 */
static inline bool objkey_equal(
                        const obj_key_t *lhs,
                        const obj_key_t *rhs)
{
    if ((lhs == NULL) ||
        (rhs == NULL)) {

        return false;
    }

    return (lhs->objectid == rhs->objectid) &&
           (lhs->gen      == rhs->gen);
}
