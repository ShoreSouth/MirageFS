# objpool.md

# ObjPool 模块设计

## 1. 模块定位

ObjPool 是 Object Layer 的对象内存池。

它负责 **ObjMeta 对象的统一申请与释放**，为 ObjMgr 提供稳定的对象分配能力。

ObjPool 不关心：

* 对象生命周期
* 引用计数
* 对象状态
* 对象索引

仅负责对象内存管理。

因此它是 Object Layer 最底层的数据管理模块之一。

整体关系如下：

```
                ObjMgr
                   │
        create / delete
                   │
      ┌────────────┴────────────┐
      │                         │
   ObjTable                 ObjPool
      │                         │
   Object Index          Object Memory
      │                         │
      └────────────┬────────────┘
                   │
                ObjMeta
```

---

# 2. 模块职责

ObjPool 仅负责以下职责：

* 创建对象内存池
* 销毁对象内存池
* 分配 ObjMeta
* 释放 ObjMeta

除此之外，不参与任何业务逻辑。

例如：

* 不维护 Hash
* 不维护 RefCnt
* 不维护 Object State
* 不维护 FUID
* 不维护 Handle

这些均属于 ObjMgr 或 ObjMeta 的职责。

---

# 3. 为什么需要 ObjPool

MirageFS 的对象全部都是运行时对象。

如果 ObjMgr 直接调用：

```
malloc()
free()
```

未来会出现几个问题：

* 无法统计对象数量
* 无法做对象缓存
* 无法替换 allocator
* 无法做 NUMA 优化
* 无法做对象调试
* 无法扩展 Slab

因此增加一层 ObjPool。

ObjMgr 不关心对象如何分配，只负责：

```
objpool_alloc()

objpool_free()
```

未来更换实现时，ObjMgr 无需修改。

---

# 4. 为什么独立模块

ObjPool 与 ObjMgr 属于不同职责。

ObjMgr：

负责对象生命周期。

例如：

```
create

delete

lookup

acquire

release
```

ObjPool：

只负责：

```
allocate

free
```

符合单一职责原则（Single Responsibility Principle）。

---

# 5. 当前实现

当前版本使用 Common 模块提供的通用内存池。

系统初始化时：

```
objpool_init()
```

创建一个专属于 Object Layer 的内存池。

之后：

```
objpool_alloc()
```

调用：

```
fs_mp_alloc()
```

释放时：

```
objpool_free()
```

调用：

```
fs_mp_free()
```

因此当前实现实际上是：

```
ObjPool
        │
        ▼
Common Memory Pool
        │
        ▼
Buddy Allocator
```

ObjPool 自身不实现内存算法。

---

# 6. 为什么不是直接使用 fs_malloc()

Object Layer 不应依赖某一种内存实现。

如果 ObjMgr 到处都是：

```
fs_malloc()

fs_free()
```

未来：

* 换 Slab
* 换对象缓存
* 调试对象泄漏
* 做对象统计

都需要修改 ObjMgr。

增加 ObjPool 后：

```
ObjMgr

↓

ObjPool

↓

Memory Pool
```

内存策略完全封装。

---

# 7. 生命周期

```
Object Layer Init
        │
        ▼
objpool_init()
        │
        ▼
创建 ObjMeta Pool
        │
        ▼
──────────────────────────────
objpool_alloc()

↓

返回 obj_meta_t
──────────────────────────────
        │
        ▼
ObjMgr 管理生命周期
        │
        ▼
objpool_free()
        │
        ▼
Object Layer Deinit
        │
        ▼
objpool_deinit()
```

---

# 8. 接口说明

## objpool_init()

初始化对象内存池。

负责创建 Object Layer 专属内存池。

成功后才能进行对象分配。

---

## objpool_deinit()

销毁对象内存池。

释放整个对象池资源。

通常在 Object Layer 退出时调用。

---

## objpool_alloc()

申请一个 ObjMeta。

返回：

```
obj_meta_t *
```

返回的对象仅保证内存可用。

对象初始化由 ObjMeta 完成。

---

## objpool_free()

释放一个 ObjMeta。

ObjPool 不检查对象状态。

调用者必须保证：

* 生命周期结束
* 已从 ObjTable 删除
* 无任何引用

通常仅允许 ObjMgr 调用。

---

# 9. 与其它模块关系

```
                ObjMgr
                   │
      create/delete/reclaim
                   │
          ┌────────┴────────┐
          │                 │
       ObjTable         ObjPool
          │                 │
      Object Index     Object Memory
          │                 │
          └────────┬────────┘
                   │
                ObjMeta
```

各模块职责如下：

| 模块       | 职责                |
| -------- | ----------------- |
| ObjMgr   | 生命周期管理、引用计数、状态迁移  |
| ObjTable | FUID → ObjMeta 索引 |
| ObjMeta  | 对象元数据             |
| ObjPool  | 对象内存分配与释放         |

---

# 10. 未来扩展

目前 ObjPool 使用通用内存池实现。

未来可逐步演进为更高性能的对象分配器，而无需修改 ObjMgr。

例如：

### Phase 1（当前）

```
ObjPool
    ↓
Common Memory Pool
```

---

### Phase 2

增加对象统计：

```
Current Objects

Peak Objects

Alloc Count

Free Count
```

方便定位对象泄漏。

---

### Phase 3

增加对象缓存：

```
free()

↓

放入对象缓存

↓

alloc()

↓

优先复用缓存对象
```

减少频繁分配释放。

---

### Phase 4

切换为 Slab Cache：

```
ObjPool

↓

Slab Cache

↓

Buddy
```

固定大小对象可获得更好的：

* 分配速度
* Cache 命中率
* 内存碎片控制

ObjMgr 无需任何修改。

---

# 11. 设计总结

ObjPool 是 Object Layer 的对象内存管理模块。

它负责 **ObjMeta 对象的统一分配与释放**，为 ObjMgr 屏蔽底层内存实现细节。

通过将内存管理从生命周期管理中解耦，Object Layer 形成了清晰的职责划分：

```
ObjMgr
    │
    ├── 生命周期
    ├── 引用计数
    ├── 状态迁移
    └── 对象管理

ObjTable
    └── 对象索引

ObjMeta
    └── 对象元数据

ObjPool
    └── 对象内存管理
```

这种设计遵循高内聚、低耦合原则，使对象生命周期、索引和内存管理彼此独立，便于后续扩展对象缓存、Slab 分配器、统计分析及调试能力，而无需影响上层 ObjMgr 的实现。
