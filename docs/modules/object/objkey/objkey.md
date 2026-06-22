# ObjKey 模块设计文档

## 1. 模块概述

ObjKey 是 Object Layer 内部的轻量级 value object，提供对象索引键的定义与操作。

ObjKey 从 FUID 中提取对象查找所需的最小信息：

```text
FUID (fsid, objectid, gen, type, flags, ...)
    │
    │ objkey_from_fuid()
    ▼
obj_key_t { objectid, gen }
```

与 FUID（64 字节完整身份标识）不同，ObjKey 仅保留 `(objectid, gen)` 两个字段，用于 ObjTable 哈希索引及对象比较。

模块定位：

```text
object/
├── fuid/          # 对象身份标识（完整 64 字节）
├── objkey/        # 对象索引键（本模块，16 字节 value object）
├── objmeta/       # 对象元数据
├── objtable/      # 对象表
└── objmgr/        # 对象生命周期管理器
```

ObjKey 的职责：

```text
obj_key_t (objectid, gen)
    ↓
ObjTable Hash Key
    ↓
ObjMeta Lookup
```

---

## 2. 设计目标

### 最小化

ObjKey 刻意精简，仅包含哈希索引所需的两个字段：

```text
objectid    — 对象唯一 ID
gen         — 对象版本号，避免对象重用冲突
```

不包含 fsid、type、flags 等 FUID 的其他字段。这些字段在对象查找阶段不需要参与键值比较。

### 值语义

`obj_key_t` 是 value object，具有以下特性：

- 栈上分配，无内部堆资源
- 通过 `objkey_make()` 按值构造，永不失败
- 提供 `is_valid`、`equal`、`hash` 等标准值对象操作
- 12 字节数据 + 4 字节尾部对齐 = 16 字节 `sizeof`

---

## 3. 数据结构

### obj_key_t

```c
typedef struct obj_key
{
    ObjectId_t objectid; /* 对象唯一 ID（uint64_t） */
    GenId_t gen;         /* 对象版本号（uint32_t）  */

} obj_key_t;             /* sizeof = 16（含尾部对齐） */
```

编译期检查：

```c
#define OBJKEY_SIZE 16

_Static_assert(sizeof(obj_key_t) == OBJKEY_SIZE,
               "obj_key_t size invalid");
```

---

## 4. API

### 构造

```c
obj_key_t objkey_make(ObjectId_t objectid, GenId_t gen);
```

按值构造 `obj_key_t`，永不失败。仅做字段打包，不校验有效性。

### 有效性判断

```c
bool objkey_is_valid(const obj_key_t *key);
```

返回 true 当且仅当 `key != NULL` 且 `objectid != 0` 且 `gen != 0`。

### 相等性比较

```c
bool objkey_equal(const obj_key_t *lhs, const obj_key_t *rhs);
```

返回 true 当且仅当两个 key 的 `objectid` 和 `gen` 均相等。任一参数为 NULL 返回 false。

### 哈希

```c
uint64_t objkey_hash(const obj_key_t *key);
```

返回 `objectid ^ gen`，供 ObjTable 等哈希容器使用。

### 从 FUID 转换

```c
void objkey_from_fuid(obj_key_t *key, const fuid_t *fuid);
```

从 FUID 中提取 `(objectid, gen)` 填入 `obj_key_t`。这是 objkey 模块唯一的非内联函数。

---

## 5. 与其他模块关系

### FUID

FUID 提供 `ObjectId_t`、`GenId_t` 等基础类型定义。`objkey_from_fuid()` 从 FUID 投影出索引键。

```text
FUID
    ↓
obj_key_t → ObjTable
```

### ObjTable

ObjTable 以 `obj_key_t` 作为哈希键，维护 `(objectid, gen) → ObjMeta` 映射。

```text
obj_key_t
    ↓
ObjTable (fs_hash)
    ↓
ObjMeta
```

ObjTable 的哈希回调（`objtable_key_hash`、`objtable_node_hash`、`objtable_match`）均委托给 `objkey_hash()` 和 `objkey_equal()`，不直接访问 `obj_key_t` 内部字段。

### ObjMeta

ObjMeta 内嵌 `obj_key_t key` 作为其身份标识字段。`objmeta_is_valid()` 和 `objmeta_equal()` 通过 `objkey_is_valid()` / `objkey_equal()` 完成 key 相关校验。

---

## 6. 当前范围

MirageFS V1 中，ObjKey 模块仅负责：

- `obj_key_t` 结构定义
- 内联 helper（make / is_valid / equal / hash）
- FUID → ObjKey 转换

不负责持久化、序列化、多版本管理等复杂逻辑。
