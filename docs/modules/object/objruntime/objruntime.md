# ObjRuntime 模块设计文档

## 1. 模块概述

ObjRuntime（Object Runtime）是 MirageFS 运行时对象的载体模块。

它负责将对象元数据（obj_meta_t）与运行时生命周期信息（refcnt、state）聚合为一个统一的运行时实例，作为系统中所有模块共同引用的锚点。

核心职责：

```text
obj_runtime_t
    │
    ├── obj_meta_t meta      → 对象固有属性（key + handle）
    ├── fs_atomic32_t refcnt → 原子引用计数
    └── uint32_t state       → 生命周期状态
```

与 obj_meta_t 的关系：

```text
obj_meta_t  — 描述"对象是什么"（What）
obj_runtime_t — 描述"对象实例"（Instance）

obj_meta_t 是 obj_runtime_t 的成员，
obj_runtime_t 是系统中所有模块共同识别的运行时句柄。
```

模块定位：

```text
object/
├── fuid/          # 对象身份标识
├── objkey/        # 对象索引键
├── objmeta/       # 对象元数据（key + handle）
├── objruntime/    # 运行时对象实例（本模块）
├── objtable/      # 对象表
├── objpool/       # 对象内存池
└── objmgr/        # 对象生命周期管理器
```

ObjRuntime 的职责：

```text
obj_runtime_t (meta + refcnt + state)
    ↓
ObjMgr 生命周期管理
    ↓
ObjTable 索引
    ↓
Cache / Storage / VFS 等模块统一引用
```

---

## 2. 设计目标

### 2.1 分离元数据与运行时状态

obj_meta_t 仅保存对象固有属性（key + handle），
refcnt、state 等生命周期信息由 obj_runtime_t 管理。

```text
之前（obj_meta_t 承载一切）：
    key + handle + refcnt + state  →  混合在一起

之后（职责分离）：
    obj_meta_t      →  key + handle      （元数据）
    obj_runtime_t   →  meta + refcnt + state （运行时实例）
```

### 2.2 统一运行时锚点

Cache、Storage、Journal 等一级模块通过 `obj_runtime_t *` 与对象建立关联，
而不是直接引用 `obj_meta_t *`。

```text
VFS
 ↓
obj_runtime_t *
 ↓
┌────────┼────────┐
ObjMgr  Cache  Storage
```

所有模块持有同一个 runtime 指针，避免多处持有 meta 指针导致的地址失效问题。

### 2.3 固定长度

```c
sizeof(obj_runtime_t) == 48
```

字段构成：

```text
obj_meta_t       : 40 bytes  (key 16 + handle 24)
fs_atomic32_t   :  4 bytes  (refcnt)
uint32_t        :  4 bytes  (state)
─────────────────────────
Total           : 48 bytes
```

优势：

- Cache Friendly
- MemPool 管理方便
- obj_meta_t 保持 40 字节轻量设计
- 与旧版 obj_meta_t（48 bytes）尺寸相同，pool 容量不变

---

## 3. 数据结构

### 3.1 obj_state_t — 生命周期状态

```c
typedef enum obj_state {
    OBJ_STATE_INVALID = 0,  /* 对象不存在，由 lookup 返回 */
    OBJ_STATE_INIT,         /* 刚创建，未激活              */
    OBJ_STATE_ACTIVE,       /* 已激活，可正常使用          */
    OBJ_STATE_DELETING      /* 删除中，阻止新引用          */
} obj_state_t;
```

状态迁移图：

```text
         INVALID
            │
       create()
            ▼
          INIT
            │
    初始化完成
            ▼
         ACTIVE
            │
        delete()
            ▼
        DELETING
            │
        ref==0
            ▼
      ObjPool Free
            │
            ▼
        INVALID (对象不存在)
```

- INVALID 不在 runtime 中存储，仅由 `objmgr_state()` 在 lookup 失败时返回
- 对象释放后即不存在，不再进入任何中间状态
- 状态的读写由 objmgr 负责，objruntime 层仅提供存储和访问器

### 3.2 obj_runtime_t 结构定义

```c
typedef struct obj_runtime {

    obj_meta_t       meta;   /* 对象元数据（key + handle） */
    fs_atomic32_t   refcnt; /* 引用计数，原子操作 */
    uint32_t         state; /* 生命周期状态，见 obj_state_t */

} obj_runtime_t;
```

逻辑结构：

```text
+-------------------+
| obj_meta_t meta   |
|  .key             |
|  .handle          |
+-------------------+
| refcnt            |
+-------------------+
| state             |
+-------------------+
```

---

### 3.3 内存布局

```text
Offset  Size    Field
------  ----    ----------------
0       8       meta.key.objectid
8       4       meta.key.gen
12      4       (key padding)
16      4       meta.handle.mount_id
20      2       meta.handle.type
22      2       meta.handle.len
24      16      meta.handle.data
40      4       refcnt
44      4       state

Total = 48 Bytes
```

---

### 3.4 编译期检查

保证结构尺寸固定：

```c
#define OBJRUNTIME_SIZE 48

_Static_assert(sizeof(obj_runtime_t) == OBJRUNTIME_SIZE,
               "obj_runtime_t size invalid");
```

---

## 4. API

### 状态访问器

```c
obj_state_t objruntime_state(const obj_runtime_t *rt);
```

封装 `rt->state` 的直接访问。后续引入 Atomic/Barrier/RCU 时仅需修改此 getter。

### Debug

```c
void objruntime_dump(const obj_runtime_t *rt);
```

输出 meta、refcnt、state 的单行快照，避免 dump 路径产生过多日志。

输出格式：

```text
objruntime: objectid=100, gen=1, refcnt=2, state=1, mount_id=23, handle_type=1, handle_bytes=8, file_handle=0a01bcff...
```

---

## 5. 模块职责

ObjRuntime 负责：

- 聚合 obj_meta_t + refcnt + state 为统一运行时结构
- 提供 obj_state_t 生命周期状态枚举
- 提供 `objruntime_state()` 状态访问器
- 提供 `objruntime_dump()` debug 输出

ObjRuntime 不负责：

- 状态迁移决策（由 objmgr 负责）
- 引用计数增减的并发控制（由 objmgr 负责）
- 内存分配与回收（由 objpool 负责）
- 对象索引与查找（由 objtable 负责）
- 元数据的校验与转换（由 objmeta 负责）

---

## 6. 与 obj_meta_t 的职责边界

| 结构 | 定位 | 包含字段 | sizeof |
|------|------|----------|--------|
| `obj_meta_t` | 对象固有属性 | key, handle | 40 |
| `obj_runtime_t` | 运行时对象实例 | meta, refcnt, state | 48 |

设计原则：

- obj_meta_t 回答"对象是谁，在哪"（What + Where）
- obj_runtime_t 回答"对象当前是什么状态，被多少人使用"（How + Count）
- obj_meta_t 是无状态的值对象（stateless value object）
- obj_runtime_t 是有状态的运行时载体（stateful runtime carrier）

---

## 7. 生命周期

### 7.1 创建

objmgr_create() 流程：

```text
objpool_alloc()           → 分配 obj_runtime_t

objmeta_init(&rt->meta)   → 初始化 meta（key + handle）

rt->state = INIT           → objmgr 设置初始状态
rt->refcnt = 0             → objmgr 初始化引用计数

objtable_insert(rt)        → 注册到 ObjTable（存指针）

objmgr_change_state(ACTIVE) → 激活
```

### 7.2 查找

```text
objmgr_lookup(fuid)
    ↓
objtable_lookup(key) → 返回 obj_runtime_t *
    ↓
objruntime_state(rt) → 检查状态
    ↓
return &rt->meta     → 公共 API 返回 obj_meta_t *
```

### 7.3 引用计数

```text
objmgr_acquire(fuid)
    ↓
objtable_lookup(key) → obj_runtime_t *
    ↓
objmgr_ref_get_locked(rt)
    ↓
fs_atomic32_inc(&rt->refcnt)
    ↓
return &rt->meta

objmgr_release(meta)
    ↓
FS_CONTAINER_OF(meta, obj_runtime_t, meta)
    ↓
objmgr_ref_put_locked(rt)
    ↓
fs_atomic32_dec(&rt->refcnt)
    ↓
if refcnt == 0 && state == DELETING → reclaim
```

### 7.4 回收

```text
refcnt == 0 && state == DELETING
    ↓
objmgr_reclaim_locked(rt)
    ↓
objtable_remove(key)     → 从 ObjTable 删除 entry（free entry wrapper）
objmeta_reset(&rt->meta) → 清空 meta
objpool_free(rt)         → 归还 runtime 到 pool
```

---

## 8. 与其他模块关系

### ObjMeta

obj_meta_t 是 obj_runtime_t 的成员。objruntime.h include objmeta.h。

```text
obj_runtime_t
    └── obj_meta_t meta
```

### ObjPool

ObjPool 管理 obj_runtime_t 的分配与释放。

```text
objpool_alloc()  → obj_runtime_t *
objpool_free(rt) → 归还到 pool
```

### ObjTable

ObjTable 以 `obj_runtime_t *` 作为索引值。

```text
objtable_entry_t
    └── obj_runtime_t *runtime   （指针，不持有副本）
```

### ObjMgr

ObjMgr 负责驱动 obj_runtime_t 的生命周期状态迁移和引用计数管理。

```text
objmgr_create()   → 分配 + 初始化 + 插入 + 激活
objmgr_delete()   → ACTIVE → DELETING → reclaim
objmgr_acquire()  → refcnt++
objmgr_release()  → refcnt--
```

### 未来模块（Cache / Storage / Journal）

通过 `obj_runtime_t *` 与对象建立关联：

```text
Cache
    └── cache_entry_t
            └── obj_runtime_t *runtime

Storage
    └── storage_entry_t
            └── obj_runtime_t *runtime

Journal
    └── txn_entry_t
            └── obj_runtime_t *runtime
```

所有模块共享同一个 runtime 指针，obj_runtime_t 作为系统中稳定的运行时锚点。

---

## 9. 当前范围

MirageFS V1 中，ObjRuntime 模块负责：

- `obj_runtime_t` 结构定义（meta + refcnt + state）
- `obj_state_t` 生命周期状态枚举
- `objruntime_state()` 状态访问器（static inline）
- `objruntime_dump()` debug 输出
- 编译期尺寸检查（OBJRUNTIME_SIZE == 48）

不负责持久化、缓存淘汰、分布式一致性等复杂逻辑。

---

## 10. 设计总结

ObjRuntime 是 MirageFS Object Layer 中承上启下的关键模块。

通过将 obj_meta_t（元数据）与 refcnt/state（运行时状态）聚合为 obj_runtime_t，
实现了元数据与运行时生命周期的职责分离。

obj_runtime_t 作为系统中所有模块共同引用的运行时锚点，
使 Cache、Storage、Journal 等未来一级模块能够通过统一的指针
与对象建立关联，而无需持有分散的 meta 引用，
为后续架构演进提供了稳定的基础。
