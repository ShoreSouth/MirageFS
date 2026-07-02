# fsmgr.md

# FSMgr 模块设计

## 1. 模块定位

`fsmgr/` 是 FSC 的门面模块。

它对外提供 filesystem instance 的创建、查找和销毁能力。

FSMgr 自己不实现底层数据结构，而是协调 FSC 内部组件：

```text
FSMgr
  ├── FSID
  ├── Namespace
  ├── NSPool
  └── FSTable
```

它在 FSC 中对应 Object Layer 的：

```text
objmgr
```

---

# 2. 模块职责

FSMgr 负责：

* 初始化内部 table 和 lock
* 创建 namespace
* 销毁 namespace
* 查找 namespace
* 查询 root handle
* 维护 namespace 数量
* 协调错误回滚

FSMgr 不负责：

* FSID 分配算法
* namespace 字段组织
* namespace 内存池实现
* hash table 实现
* mount / policy 规则

FSMgr 是协调者（Coordinator），不是所有逻辑的堆放处。

---

# 3. 内部结构

FSMgr 内部全局对象为：

```c
typedef struct fsc_manager {

    fsc_table_t   table;
    fs_mutex_t    lock;
    fs_atomic32_t namespace_count;

} fsc_manager_t;
```

字段说明：

| 字段 | 含义 |
| --- | --- |
| `table` | namespace 注册表 |
| `lock` | manager 级互斥锁 |
| `namespace_count` | 当前 namespace 数量 |

当前采用全局互斥锁保证一致性。

---

# 4. 创建流程

`fsmgr_create()` 是 namespace 的唯一创建入口。

流程如下：

```text
fsmgr_create(name, root)
    │
    ├── 校验 name
    ├── 加锁
    ├── 检查 name 是否已存在
    ├── fsid_alloc()
    ├── nspool_alloc()
    ├── fsc_namespace_init()
    ├── 设置 ACTIVE
    ├── fstable_insert()
    ├── namespace_count++
    ├── 解锁
    └── return namespace
```

创建成功后：

* namespace 已有 FSID
* namespace 已初始化
* namespace 已注册到 FSTable
* namespace 状态为 ACTIVE

返回值是借用指针。

---

# 5. 创建失败回滚

创建过程中任一步失败，都必须释放已经获得的资源。

例如：

```text
fsid_alloc() 成功
nspool_alloc() 失败
    │
    └── fsid_free()
```

或者：

```text
nspool_alloc() 成功
fsc_namespace_init() 失败
    │
    ├── nspool_free()
    └── fsid_free()
```

FSMgr 必须保证失败路径不泄漏 FSID 或 namespace 对象。

---

# 6. 销毁流程

`fsmgr_destroy()` 通过 FSID 销毁 namespace。

流程如下：

```text
fsmgr_destroy(fsid)
    │
    ├── 加锁
    ├── fstable_lookup_fsid()
    ├── 检查 refcnt == 0
    ├── 设置 DELETING
    ├── fstable_remove()
    ├── namespace_count--
    ├── 解锁
    ├── fsid_free()
    └── nspool_free()
```

当前版本 refcnt 还没有对外 acquire/release API，因此 destroy 只允许 refcnt 为 0。

---

# 7. 查找语义

FSMgr 提供两类 lookup：

```text
fsmgr_lookup(name)

fsmgr_lookup_fsid(fsid)
```

查找只返回 ACTIVE namespace。

如果 namespace 不存在，或状态不是 ACTIVE，返回：

```text
NULL
```

lookup 不增加引用计数。

调用者只能短时间借用返回指针，不能释放。

---

# 8. Root Handle

`fsmgr_get_root()` 根据 FSID 返回 namespace 中保存的 root object handle。

当前 root handle 只作为 backend locator 保存。

它不负责：

* 打开对象
* 校验对象类型
* 创建 root object

这些属于后续 VFS / Object / LSA 联动逻辑。

---

# 9. 并发模型

当前版本使用单把 manager 互斥锁：

```text
FSMgr Lock
    │
    ▼
FSTable
```

优点：

* 实现简单
* 生命周期一致性容易保证
* create/destroy/lookup 行为清晰

未来可以演进为：

```text
Global Lock
    │
    ▼
RWLock
    │
    ▼
Bucket Lock
```

对外 API 不需要变化。

---

# 10. API

```text
fsmgr_init()

fsmgr_deinit()

fsmgr_create()

fsmgr_destroy()

fsmgr_lookup()

fsmgr_lookup_fsid()

fsmgr_exists()

fsmgr_get_root()

fsmgr_count()
```

---

# 11. 未来扩展

FSMgr 后续可以增加：

* namespace acquire / release
* 默认 namespace
* root namespace
* mount namespace
* policy 检查入口
* namespace rename
* namespace readonly 标记

这些应保持协调逻辑，不应把底层索引和内存管理塞回 FSMgr。

---

# 12. 设计总结

FSMgr 是 FSC 的协调中心。

它通过组合 FSID、NSPool、Namespace 和 FSTable，提供 filesystem instance 的统一生命周期管理。

整体关系如下：

```text
              FSMgr
      ┌────────┼────────┐
      │        │        │
    FSID    NSPool   FSTable
              │        │
          Namespace  Index
```

这种设计保持了 FSC 内部职责清晰，也和 Object Layer 的 ObjMgr / ObjPool / ObjTable 结构保持一致。
