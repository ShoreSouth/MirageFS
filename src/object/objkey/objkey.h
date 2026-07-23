#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "object/fuid/fuid.h"

/* ============================================================
 * 常量定义
 * ============================================================ */

#define OBJKEY_SIZE 16

/* ============================================================
 * 核心结构
 * ============================================================
 */

/*
 * 对象索引 Key。
 *
 * ObjKey 是 Object Layer 内部使用的对象索引键，
 * 由 (objectid, gen) 组成。
 *
 * 与 FUID 不同，ObjKey 仅保留对象查找所需的最小信息，
 * 用于 ObjTable 哈希索引及对象比较。
 */
typedef struct obj_key
{
    ObjectId_t objectid; /* 对象唯一 ID */
    GenId_t gen;         /* 对象版本号，避免对象重用冲突 */

} obj_key_t;

/* ============================================================
 * 编译期检查
 * ============================================================ */

_Static_assert(sizeof(obj_key_t) == OBJKEY_SIZE, "obj_key_t size invalid");

/* ============================================================
 * helper（内联）
 * ============================================================
 */

/*
 * 构造对象 Key。
 */
static inline obj_key_t objkey_make(ObjectId_t objectid, GenId_t gen)
{
    obj_key_t key;

    key.objectid = objectid;
    key.gen = gen;

    return key;
}

/*
 * 判断对象 Key 是否有效。
 *
 * objectid 和 gen 均不能为 0。
 */
static inline bool objkey_is_valid(const obj_key_t *key)
{
    if (key == NULL)
    {
        return false;
    }

    return (key->objectid != 0) && (key->gen != 0);
}

/*
 * 判断两个对象 Key 是否相等。
 */
static inline bool objkey_equal(const obj_key_t *lhs, const obj_key_t *rhs)
{
    if ((lhs == NULL) || (rhs == NULL))
    {
        return false;
    }

    return (lhs->objectid == rhs->objectid) && (lhs->gen == rhs->gen);
}

/*
 * 计算对象 Key 的哈希值。
 */
static inline uint64_t objkey_hash(const obj_key_t *key)
{
    return ((uint64_t)key->objectid) ^ ((uint64_t)key->gen);
}

/* ============================================================
 * 转换接口
 * ============================================================
 */

/*
 * 根据 FUID 生成对象 Key。
 *
 * ObjKey 是 ObjTable 的唯一索引键，
 * 提取 FUID 中用于对象索引的字段：
 *
 *      (objectid, gen)
 *
 * 参数：
 *      [OUT] key   : 输出对象 Key
 *      [IN]  fuid  : 输入对象标识
 */
void objkey_from_fuid(obj_key_t *key, const fuid_t *fuid);
