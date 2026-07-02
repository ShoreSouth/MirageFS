# namespace.md

# Namespace 模块设计

## 1. 模块定位

`namespace/` 定义 FSC 的运行时对象：

```text
fsc_namespace_t
```

Namespace 表示 MirageFS 中一个 filesystem instance 的控制面入口。

它不是路径树，也不是目录项缓存，而是一个运行时管理对象，类似 Object Layer 中的 `obj_runtime_t`。

---

# 2. 模块职责

Namespace 模块负责：

* 定义 `fsc_namespace_t`
* 定义 namespace 生命周期状态
* 初始化 namespace 对象
* 清理 namespace 对象
* 校验 namespace 对象
* 校验 namespace name
* 提供 debug dump

Namespace 模块不负责：

* 内存分配
* hash 索引
* FSID 分配
* manager 编排
* mount / policy 语义

这些分别属于 `nspool`、`fstable`、`fsid`、`fsmgr`、`policy`。

---

# 3. 数据结构

核心结构如下：

```c
typedef struct fsc_namespace {

    fsc_fsid_t      fsid;
    char            name[FSC_NAMESPACE_NAME_MAX];

    obj_handle_t    root;

    fs_atomic32_t   refcnt;
    uint32_t        state;

    uint8_t         reserved[24];

} fsc_namespace_t;
```

字段说明：

| 字段 | 含义 |
| --- | --- |
| `fsid` | filesystem identity |
| `name` | namespace 名称 |
| `root` | root object 的 backend handle |
| `refcnt` | 引用计数，预留给后续 acquire/release |
| `state` | 生命周期状态 |
| `reserved` | 预留字段，保持结构大小稳定 |

当前结构大小固定为：

```text
FSC_NAMESPACE_SIZE == 128
```

并通过 `_Static_assert` 校验。

---

# 4. 生命周期状态

Namespace 当前状态如下：

```text
FSC_NAMESPACE_STATE_INVALID
FSC_NAMESPACE_STATE_INIT
FSC_NAMESPACE_STATE_ACTIVE
FSC_NAMESPACE_STATE_DELETING
```

生命周期流转：

```text
nspool_alloc()
    │
    ▼
fsc_namespace_init()
    │
    ▼
INIT
    │
    ▼
ACTIVE
    │
    ▼
DELETING
    │
    ▼
nspool_free()
```

说明：

* `INVALID` 只表示无效或不存在
* `INIT` 表示对象已初始化但尚未对外可见
* `ACTIVE` 表示已注册到 `fstable`
* `DELETING` 表示正在销毁

---

# 5. 初始化语义

`fsc_namespace_init()` 只负责填充字段。

它不负责：

* 分配内存
* 插入 fstable
* 设置对外可见性
* 增加引用计数

初始化完成后状态为：

```text
FSC_NAMESPACE_STATE_INIT
```

真正进入服务状态由 `fsmgr` 完成：

```text
fsc_namespace_change_state(ns, FSC_NAMESPACE_STATE_ACTIVE)
```

---

# 6. 所有权

Namespace 对象必须来自：

```text
nspool_alloc()
```

释放必须通过：

```text
nspool_free()
```

`fstable` 只保存借用指针。

`fsmgr` 是当前唯一负责 namespace 生命周期编排的模块。

---

# 7. API

```text
fsc_namespace_init()

fsc_namespace_deinit()

fsc_namespace_is_valid()

fsc_namespace_name_is_valid()

fsc_namespace_state()

fsc_namespace_state_can_transit()

fsc_namespace_change_state()

fsc_namespace_dump()
```

状态修改必须通过 `fsc_namespace_change_state()` 完成。
当前允许的状态迁移只有：

```text
INIT -> ACTIVE -> DELETING
```

`fsc_namespace_dump()` 只输出一行 namespace 快照，避免 dump 路径造成日志刷屏。

---

# 8. 设计总结

Namespace 是 FSC 的运行时对象。

它承载一个 filesystem instance 的基本控制面状态，但不直接管理内存、索引或策略。

这种拆分让 FSC 内部职责清晰：

```text
Namespace  定义对象
NSPool     分配对象
FSTable    索引对象
FSMgr      管理对象生命周期
```
