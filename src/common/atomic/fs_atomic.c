#include "common/atomic/fs_atomic.h"

#include <stdio.h>
#include <assert.h>

/*
 * ============================================================
 * init
 * ============================================================
 */

void fs_atomic32_init(fs_atomic32_t *atom, int32_t value)
{
    assert(atom != NULL);

    atomic_init(atom, value);
}

void fs_atomic64_init(fs_atomic64_t *atom, int64_t value)
{
    assert(atom != NULL);

    atomic_init(atom, value);
}

/*
 * ============================================================
 * load/store
 * ============================================================
 */

int32_t fs_atomic32_load(const fs_atomic32_t *atom)
{
    assert(atom != NULL);

    return atomic_load(atom);
}

int64_t fs_atomic64_load(const fs_atomic64_t *atom)
{
    assert(atom != NULL);

    return atomic_load(atom);
}

void fs_atomic32_store(fs_atomic32_t *atom, int32_t value)
{
    assert(atom != NULL);

    atomic_store(atom, value);
}

void fs_atomic64_store(fs_atomic64_t *atom, int64_t value)
{
    assert(atom != NULL);

    atomic_store(atom, value);
}

/*
 * ============================================================
 * increment/decrement
 *
 * 返回修改后的值
 * ============================================================
 */

int32_t fs_atomic32_inc(fs_atomic32_t *atom)
{
    assert(atom != NULL);

    return atomic_fetch_add(atom, 1) + 1;
}

int64_t fs_atomic64_inc(fs_atomic64_t *atom)
{
    assert(atom != NULL);

    return atomic_fetch_add(atom, 1) + 1;
}

int32_t fs_atomic32_dec(fs_atomic32_t *atom)
{
    assert(atom != NULL);

    return atomic_fetch_sub(atom, 1) - 1;
}

int64_t fs_atomic64_dec(fs_atomic64_t *atom)
{
    assert(atom != NULL);

    return atomic_fetch_sub(atom, 1) - 1;
}

/*
 * ============================================================
 * add/sub
 *
 * 返回修改后的值
 * ============================================================
 */

int32_t fs_atomic32_add(fs_atomic32_t *atom, int32_t value)
{
    assert(atom != NULL);

    return atomic_fetch_add(atom, value) + value;
}

int64_t fs_atomic64_add(fs_atomic64_t *atom, int64_t value)
{
    assert(atom != NULL);

    return atomic_fetch_add(atom, value) + value;
}

int32_t fs_atomic32_sub(fs_atomic32_t *atom, int32_t value)
{
    assert(atom != NULL);

    return atomic_fetch_sub(atom, value) - value;
}

int64_t fs_atomic64_sub(fs_atomic64_t *atom, int64_t value)
{
    assert(atom != NULL);

    return atomic_fetch_sub(atom, value) - value;
}

/*
 * ============================================================
 * compare and swap
 *
 * 成功:
 *      返回 true
 *
 * 失败:
 *      返回 false
 *      expected 被更新为当前值
 * ============================================================
 */

bool fs_atomic32_cas(fs_atomic32_t *atom, int32_t *expected, int32_t desired)
{
    assert(atom != NULL);
    assert(expected != NULL);

    return atomic_compare_exchange_strong(atom, expected, desired);
}

bool fs_atomic64_cas(fs_atomic64_t *atom, int64_t *expected, int64_t desired)
{
    assert(atom != NULL);
    assert(expected != NULL);

    return atomic_compare_exchange_strong(atom, expected, desired);
}
