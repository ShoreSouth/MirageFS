# fsid.md

# FSID 模块设计

## 1. 模块定位

`fsid/` 是 FSC 中最底层、最独立的基础组件。

它负责为 filesystem instance 分配唯一身份，即：

```text
fsc_fsid_t
```

FSC 使用 `fsc_fsid_t` 而不是 `fsid_t`，原因是 Linux 系统头文件中已经存在 `fsid_t` 类型。

为了避免命名冲突，FSC 内部统一使用：

```text
fsc_fsid_t
```

---

# 2. 模块职责

FSID 模块负责：

* 初始化 FSID 分配器
* 销毁 FSID 分配器
* 分配新的 FSID
* 释放 FSID
* 判断 FSID 是否有效
* 为 hash index 提供 hash 值

FSID 模块不负责：

* Namespace 内存管理
* Namespace 生命周期
* Mount 关系
* Policy 检查
* Object 创建

因此它是一个纯身份分配模块。

---

# 3. 当前实现

当前实现采用简单的单调递增分配器：

```text
g_fsid_next = 1

fsid_alloc()
    │
    ├── atomic inc
    └── return old value
```

其中：

```text
FSID_INVALID == 0
```

所以有效 FSID 从 1 开始。

---

# 4. 为什么暂不复用 FSID

当前 `fsid_free()` 只做合法性检查，不会把 FSID 放回 free-list。

这样做是刻意的。

原因：

* 第一版目标是跑通 FSC 生命周期闭环
* 不复用 FSID 可以避免 stale namespace 问题
* 后续可以在不影响上层 API 的情况下替换实现

未来如果需要复用，可以在模块内部加入：

```text
bitmap

free-list

generation
```

外部调用者无需修改。

---

# 5. API

## fsid_init()

初始化 FSID 分配器。

当前会把下一个可分配 FSID 重置为 1。

---

## fsid_deinit()

销毁 FSID 分配器。

当前实现没有动态资源，仅重置内部计数。

---

## fsid_alloc()

分配一个新的 `fsc_fsid_t`。

参数：

```text
[OUT] fsid
```

返回：

```text
FS_OK
fs_error_t
```

---

## fsid_free()

释放一个 FSID。

当前版本不复用 FSID，仅保留生命周期语义。

---

## fsid_is_valid()

判断 FSID 是否有效。

当前规则：

```text
fsid != FSID_INVALID
```

---

## fsid_hash()

为 `fstable` 的 FSID 索引提供 hash 值。

---

# 6. 与其它模块关系

```text
FSMgr
  │
  ├── fsid_alloc()
  │
  └── fsid_free()

FSTable
  │
  └── fsid_hash()

Namespace
  │
  └── 保存 fsc_fsid_t
```

---

# 7. 设计总结

FSID 模块是 FSC 的身份基础。

它把 filesystem identity 的生成和校验集中封装起来，使 Namespace、FSTable、FSMgr 都不需要关心 FSID 的分配策略。

当前实现简单，但 API 足够稳定，后续可以逐步演进为可复用、带 generation、防 stale 的分配器。
