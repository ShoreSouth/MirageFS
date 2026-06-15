#pragma once

#include <stdbool.h>

#include "fuid/fuid.h"

/* ============================================================
 * 核心结构
 * ============================================================ */

typedef struct objkey {

    ObjectId_t objectid;

    GenId_t gen;

} objkey_t;

/* ============================================================
 * helper
 * ============================================================ */

static inline objkey_t objkey_make(
                        ObjectId_t objectid,
                        GenId_t gen)
{
    objkey_t key;

    key.objectid = objectid;
    key.gen      = gen;

    return key;
}

static inline bool objkey_valid(
                        const objkey_t *key)
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

static inline bool objkey_equal(
                        const objkey_t *lhs,
                        const objkey_t *rhs)
{
    if ((lhs == NULL) ||
        (rhs == NULL)) {

        return false;
    }

    return (lhs->objectid == rhs->objectid) &&
           (lhs->gen      == rhs->gen);
}