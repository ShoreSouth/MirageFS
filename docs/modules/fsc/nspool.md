# nspool.md

# NSPool 模块设计

## 1. 模块定位

`nspool/` 是 FSC 的 Namespace 对象内存池。

它负责：

```text
fsc_namespace_t
```

的统一申请与释放。

它在 FSC 中对应 Object Layer 的：

```text
objpool
```

---

# 2. 为什么需要 NSPool

Namespace 是 FSC 的运行时对象。

虽然 namespace 数量通常不会像 object 数量那么大，但它仍然具有：

* 生命周期
* 状态
* 引用计数
* 索引
* 未来扩展字段

如果直接使用：

```text
malloc()

free()
```

会导致 FSC 与 Object Layer 的风格不一致，也不利于后续扩展。

增加 NSPool 后：

```text
FSMgr
  │
  ▼
NSPool
  │
  ▼
Common Mempool
```

FSMgr 不需要关心底层内存实现。

---

# 3. 模块职责

NSPool 负责：

* 创建 namespace 专用内存池
* 销毁 namespace 专用内存池
* 分配 `fsc_namespace_t`
* 释放 `fsc_namespace_t`

NSPool 不负责：

* 初始化 namespace 字段
* 设置 namespace 状态
* 插入 fstable
* 维护引用计数

这些由 Namespace / FSMgr 负责。

---

# 4. 当前实现

当前版本基于 Common Mempool。

初始化：

```text
nspool_init()
    │
    ▼
fs_mp_create()
```

分配：

```text
nspool_alloc()
    │
    ▼
fs_mp_calloc()
```

释放：

```text
nspool_free()
    │
    ├── fsc_namespace_deinit()
    └── fs_mp_free()
```

默认容量：

```text
NSPOOL_DEFAULT_NAMESPACES = 64
```

默认容量刻意保持较小。正常运行中一个进程内不会同时创建大量 filesystem namespace；
后续如果需要压测多 namespace 场景，再把该值提升或改成配置项。

---

# 5. 生命周期

NSPool 的生命周期由 `fsc_init()` 和 `fsc_deinit()` 管理。

```text
fsc_init()
    │
    ▼
nspool_init()
    │
    ▼
fsmgr_create()
    │
    ▼
nspool_alloc()
    │
    ▼
fsmgr_destroy()
    │
    ▼
nspool_free()
    │
    ▼
fsc_deinit()
    │
    ▼
nspool_deinit()
```

---

# 6. API

## nspool_init()

创建 namespace 专用内存池。

---

## nspool_deinit()

销毁 namespace 专用内存池。

调用前应确保所有 namespace 已经由 `fsmgr` 回收。

---

## nspool_alloc()

申请一个清零后的 `fsc_namespace_t`。

返回：

```text
NULL
fsc_namespace_t *
```

---

## nspool_free()

释放一个 namespace 对象。

调用者必须保证：

* namespace 已从 `fstable` 移除
* namespace 不再被外部持有
* 生命周期已经结束

当前通常仅允许 `fsmgr` 调用。

---

# 7. 未来扩展

NSPool 后续可以演进为：

* namespace 对象缓存
* alloc/free 统计
* 泄漏检测
* slab cache
* debug magic

FSMgr API 不需要因此变化。

---

# 8. 设计总结

NSPool 将 namespace 内存管理从生命周期管理中剥离出来。

这样 FSC 的分层更加清晰：

```text
FSMgr      生命周期编排
FSTable    索引
Namespace  对象内容
NSPool     对象内存
```
