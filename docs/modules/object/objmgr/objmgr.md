# objmgr.md

# ObjMgr 模块设计

## 1. 模块定位

ObjMgr（Object Manager）是 Object Layer 的核心管理模块。

它负责 MirageFS 运行时对象（Runtime Object）的统一管理，对外提供对象生命周期、引用计数以及状态管理接口。

ObjMgr 是 Object Layer 唯一对外暴露对象管理能力的模块。

Object Layer 的整体结构如下：

```text
                 VFS / FSMGR
                      │
                create / lookup
                acquire / release
                delete / exists
                      │
                  ObjMgr
        ┌────────┼────────┐
        │        │        │
    ObjPool   ObjTable  ObjMeta
        │        │
    Memory    Object Index
```

其中：

* ObjMgr 负责对象生命周期管理
* ObjPool 负责运行时对象内存管理（obj_runtime_t）
* ObjTable 提供对象索引
* ObjMeta 保存对象固有元数据（key + handle）
* ObjRuntime 聚合 meta + refcnt + state

---

# 2. 模块职责

ObjMgr 负责：

* 对象创建
* 对象删除
* 对象查找
* 对象引用计数
* 生命周期状态迁移
* 全局对象管理
* 并发同步

ObjMgr 不负责：

* 内存分配（ObjPool，管理 obj_runtime_t）
* Hash 索引实现（ObjTable）
* 元数据组织（ObjMeta + ObjRuntime）

因此 ObjMgr 更像 Object Layer 的调度中心（Coordinator）。

---

# 3. 生命周期管理

ObjMgr 管理对象完整生命周期。

当前状态如下：

```text
        create()

            │
            ▼

        OBJ_STATE_INIT

            │

            ▼

      OBJ_STATE_ACTIVE

            │

      delete()

            ▼

    OBJ_STATE_DELETING

            │

     refcnt == 0

            ▼

      reclaim object

            │

            ▼

      Object Destroyed
```

说明：

* INIT 为初始化阶段，仅 ObjMgr 内部可见
* ACTIVE 为正常服务状态
* DELETING 表示对象正在删除，不再接受新的引用
* 对象最终释放后直接消失，不存在稳定的 DELETED 状态

因此：

```text
DELETED
```

并不是生命周期状态，而是对象已经不存在。

---

# 4. 对象创建流程

对象创建流程如下：

```text
objmgr_create()

        │

        ▼

objpool_alloc()           → obj_runtime_t *

        │

        ▼

objmeta_init(&rt->meta)   → 初始化 meta

        │

        ▼

rt->state = INIT          → 设置初始状态

        │

        ▼

objtable_insert(rt)       → 注册到 ObjTable

        │

        ▼

INIT → ACTIVE             → 激活

        │

        ▼

return &rt->meta           → 返回 obj_meta_t *
```

说明：

ObjMgr 是对象唯一创建入口。

对象创建成功以后：

* 已进入 ObjTable
* 已完成初始化
* 已进入 ACTIVE 状态

调用者无需再进行任何初始化。

---

# 5. 对象删除流程

删除流程如下：

```text
objmgr_delete()

        │

lookup()

        │

        ▼

ACTIVE ?

        │

        ▼

ACTIVE → DELETING

        │

        ▼

refcnt == 0 ?

      │        │

     NO       YES

      │        │

等待最后引用     reclaim

               │

               ▼

objtable_remove()

objmeta_reset()

objpool_free(runtime)
```

说明：

删除采用延迟回收（Deferred Reclaim）机制。

对象一旦进入 DELETING：

* 不允许新的 acquire()
* 已有引用仍然有效
* 最后一个引用释放后自动回收对象

因此不存在悬空指针问题。

---

# 6. 引用计数模型

ObjMgr 使用引用计数保证对象生命周期。

```
lookup()

    不增加引用

acquire()

    ref++

release()

    ref--

get()

    ref++

put()

    ref--
```

推荐业务层统一使用：

```text
acquire()

release()
```

底层模块可直接使用：

```text
get()

put()
```

---

# 7. 为什么 create() 不增加引用

ObjMgr 的设计遵循统一规则：

只有：

```text
acquire()

get()
```

会增加引用计数。

因此：

```text
create()

lookup()
```

返回的对象仅表示：

> 当前可以访问该对象。

如果调用者需要长期持有对象，应主动调用：

```text
objmgr_acquire()
```

结束后调用：

```text
objmgr_release()
```

这样整个 Object Layer 的引用模型保持一致。

---

# 8. Lookup 与 Acquire

ObjMgr 提供两种获取对象方式。

## lookup()

```text
obj_meta_t *
```

特点：

* 不增加引用
* 仅适合短时间访问
* 不保证对象不会被删除

主要用于：

* 查询状态
* 调试
* 临时访问

---

## acquire()

```text
obj_meta_t *
```

特点：

* 自动增加引用
* 生命周期受到保护
* 推荐业务层使用

使用方式：

```text
meta = objmgr_acquire(...);

/* 使用对象 */

objmgr_release(meta);
```

---

# 9. 状态可见性

ObjMgr 对不同状态采用不同访问策略。

| 状态           | lookup | acquire | delete |
| ------------ | ------ | ------- | ------ |
| INIT         | ×      | ×       | ×      |
| ACTIVE       | √      | √       | √      |
| DELETING     | ×      | ×       | ×      |
| INVALID（不存在） | ×      | ×       | ×      |

因此：

Object Layer 对外只暴露：

```text
ACTIVE
```

其它状态均属于内部生命周期状态。

---

# 10. 内部模块关系

ObjMgr 内部依赖三个子模块。

## ObjPool

负责：

```text
allocate

free
```

ObjMgr 不直接管理内存。

---

## ObjTable

负责：

```text
lookup

insert

remove
```

ObjMgr 不关心 Hash 实现。

---

## ObjMeta

负责：

```text
Object Metadata
```

包括：

* Key
* RefCnt
* State
* Handle

ObjMgr 驱动状态变化，但不直接组织元数据。

---

# 11. API 分类

## 生命周期

```text
objmgr_init()

objmgr_deinit()
```

---

## 对象管理

```text
objmgr_create()

objmgr_delete()

objmgr_lookup()

objmgr_exists()
```

---

## 引用管理

```text
objmgr_acquire()

objmgr_release()

objmgr_get()

objmgr_put()
```

---

## 查询接口

```text
objmgr_state()

objmgr_refcnt()

objmgr_count()
```

---

# 12. 并发模型

当前版本采用一级全局互斥锁：

```text
ObjMgr Lock

↓

ObjTable

↓

ObjMeta
```

所有对象操作均受同一把锁保护。

优点：

* 实现简单
* 状态一致性容易保证
* 生命周期管理清晰

未来可逐步演进为：

```text
Global Lock

        ↓

RW Lock

        ↓

Bucket Lock

        ↓

Per Object Lock
```

整个 API 保持不变。

---

# 13. 模块划分

ObjMgr 当前拆分为三个实现文件。

## objmgr.c

负责流程编排。

包括：

* create
* delete
* lookup
* exists
* init
* deinit

---

## objmgr_ref.c

负责引用计数。

包括：

```text
get

put

acquire

release
```

以及引用计数查询。

---

## objmgr_internal.c

负责内部辅助逻辑。

包括：

* 状态迁移
* 回收对象
* ObjTable Helper
* 生命周期 Helper

供 ObjMgr 内部共享，不对外暴露。

---

# 14. 设计总结

ObjMgr 是 MirageFS Object Layer 的核心协调模块。

它不负责对象内存分配、对象索引或元数据组织，而是通过组合 ObjPool、ObjTable 和 ObjMeta，为上层模块提供统一、安全且线程安全的运行时对象管理能力。

通过生命周期状态机与引用计数机制，ObjMgr 保证对象在整个运行期间始终保持一致性，并采用延迟回收（Deferred Reclaim）策略避免悬空引用。

整体职责划分如下：

```text
                ObjMgr
      ┌──────────┼──────────┐
      │          │          │
 生命周期      引用计数     调度编排
      │          │
      ├──────────┼──────────┐
      │          │          │
  ObjPool    ObjTable    ObjMeta
      │          │          │
 内存管理    对象索引    对象元数据
```

这种设计遵循高内聚、低耦合原则，使 Object Layer 能够在保持稳定 API 的前提下，持续演进对象缓存、细粒度锁、Slab 分配器及其它高性能特性，而无需影响上层 VFS/FSMGR 的使用方式。
