#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

/*
 * ============================================================
 * atomic type
 * ============================================================
 */

typedef _Atomic int32_t fs_atomic32_t;
typedef _Atomic int64_t fs_atomic64_t;

/*
 * ============================================================
 * init
 * ============================================================
 */

void fs_atomic32_init(
                fs_atomic32_t *atom,
                int32_t value);

void fs_atomic64_init(
                fs_atomic64_t *atom,
                int64_t value);

/*
 * ============================================================
 * load/store
 * ============================================================
 */

int32_t fs_atomic32_load(
                const fs_atomic32_t *atom);

int64_t fs_atomic64_load(
                const fs_atomic64_t *atom);

void fs_atomic32_store(
                fs_atomic32_t *atom,
                int32_t value);

void fs_atomic64_store(
                fs_atomic64_t *atom,
                int64_t value);

/*
 * ============================================================
 * increment/decrement
 * ============================================================
 */

int32_t fs_atomic32_inc(
                fs_atomic32_t *atom);

int64_t fs_atomic64_inc(
                fs_atomic64_t *atom);

int32_t fs_atomic32_dec(
                fs_atomic32_t *atom);

int64_t fs_atomic64_dec(
                fs_atomic64_t *atom);

/*
 * ============================================================
 * add/sub
 * ============================================================
 */

int32_t fs_atomic32_add(
                fs_atomic32_t *atom,
                int32_t value);

int64_t fs_atomic64_add(
                fs_atomic64_t *atom,
                int64_t value);

int32_t fs_atomic32_sub(
                fs_atomic32_t *atom,
                int32_t value);

int64_t fs_atomic64_sub(
                fs_atomic64_t *atom,
                int64_t value);

/*
 * ============================================================
 * compare and swap
 * ============================================================
 */

bool fs_atomic32_cas(
                fs_atomic32_t *atom,
                int32_t *expected,
                int32_t desired);

bool fs_atomic64_cas(
                fs_atomic64_t *atom,
                int64_t *expected,
                int64_t desired);
