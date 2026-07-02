# fstable.md

# FSTable 模块设计

## 1. 模块定位

`fstable/` 是 FSC 的 Namespace 注册表。

它负责把：

```text
FSID / name
```

映射到：

```text
fsc_namespace_t *
```

在 FSC 中，它对应 Object Layer 的：

```text
objtable
```

---

# 2. 模块职责

FSTable 负责：

* 初始化 namespace 注册表
* 销毁 namespace 注册表
* 插入 namespace
* 移除 namespace
* 按 FSID 查找 namespace
* 按 name 查找 namespace
* 统计 namespace 数量

FSTable 不负责：

* 分配 namespace 内存
* 释放 namespace 内存
* 分配 FSID
* 管理 namespace 状态
* 管理引用计数

这些属于 `nspool`、`fsid`、`namespace`、`fsmgr`。

---

# 3. 双索引设计

FSTable 当前维护两个索引：

```text
fsid_index:
    fsc_fsid_t -> fsc_namespace_t *

name_index:
    name -> fsc_namespace_t *
```

原因：

* FSID 是系统内部稳定身份
* name 是外部创建 / 查找入口
* 两者都必须唯一

因此插入时会同时检查：

```text
fstable_exists_fsid()

fstable_exists_name()
```

任意一个已存在，都会返回 EEXIST 类错误。

---

# 4. 数据结构

## fsc_table_t

```c
typedef struct fsc_table {

    fs_hash_t fsid_index;
    fs_hash_t name_index;

} fsc_table_t;
```

`fsc_table_t` 是索引容器。

它不直接保存 namespace 数组，而是通过 hash table 管理 entry。

---

## fsc_table_entry_t

```c
typedef struct fsc_table_entry {

    fsc_namespace_t *ns;

    fs_list_head_t fsid_node;
    fs_list_head_t name_node;

} fsc_table_entry_t;
```

一个 entry 同时挂入两个 hash index。

注意：

```text
entry 拥有 hash 节点
entry 不拥有 namespace
```

namespace 生命周期由 `fsmgr` 管理。

---

# 5. 插入流程

```text
fstable_insert(table, ns)
    │
    ├── 参数检查
    ├── namespace 校验
    ├── 检查 FSID 是否重复
    ├── 检查 name 是否重复
    ├── 分配 fsc_table_entry_t
    ├── 插入 fsid_index
    └── 插入 name_index
```

如果第二个索引插入失败，需要回滚第一个索引。

---

# 6. 移除流程

```text
fstable_remove(table, fsid, ns_out)
    │
    ├── 通过 fsid 查找 entry
    ├── 从 fsid_index 移除
    ├── 从 name_index 移除
    ├── 通过 ns_out 返回 namespace
    └── 释放 entry
```

移除只释放 entry，不释放 namespace。

---

# 7. 错误语义

| 操作 | 失败原因 | 错误类型 |
| --- | --- | --- |
| insert | 参数非法 | EINVAL |
| insert | namespace 无效 | EINVAL |
| insert | FSID 或 name 重复 | EEXIST |
| insert | entry 分配失败 | ENOMEM |
| remove | 参数非法 | EINVAL |
| remove | 未找到 | ENOENT |

---

# 8. API

```text
fstable_init()

fstable_deinit()

fstable_insert()

fstable_remove()

fstable_lookup_fsid()

fstable_lookup_name()

fstable_exists_fsid()

fstable_exists_name()

fstable_count()
```

---

# 9. 与其它模块关系

```text
FSMgr
  │
  ├── fstable_insert()
  ├── fstable_remove()
  ├── fstable_lookup_name()
  └── fstable_lookup_fsid()

FSTable
  │
  └── Common Hash
```

FSTable 是纯索引层。

---

# 10. 设计总结

FSTable 将 namespace 的查询能力从 FSMgr 中分离出来。

FSMgr 只负责生命周期编排，FSTable 只负责索引。

这种职责划分和 Object Layer 中的 ObjMgr / ObjTable 保持一致，便于后续增加 bucket lock、RCU、持久化索引等能力。
