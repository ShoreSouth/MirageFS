# fsc.md

# FSC 模块设计

## 1. 模块定位

FSC（Filesystem Control）是 MirageFS 的文件系统控制平面。

它负责管理 MirageFS 中的 filesystem instance，包括：

* FSID 分配
* Namespace 运行时对象
* Namespace 注册表
* 文件系统生命周期
* 后续 Mount / Policy / Quota / Snapshot 等控制逻辑的承载位置

FSC 不是单纯的 FSMgr。

FSMgr 只是 FSC 内部的一个协调组件，类似 ObjMgr 只是 Object Layer 内部的一个组件。

FSC 和 Object Layer 的关系如下：

```text
                 VFS
                  │
        create / lookup / destroy
                  │
                 FSC
      ┌───────────┼───────────┐
      │           │           │
    FSID      Namespace     FSTable
                  │           │
               NSPool     Name / FSID Index
                  │
                FSMgr
```

因此，FSC 是一个一级模块，而不是一个普通 manager 目录。

---

# 2. 与 Object Layer 的镜像关系

FSC 的目录和职责刻意参照 Object Layer。

| Object Layer | FSC | 说明 |
| --- | --- | --- |
| `object_init` | `fsc_init` | 一级模块初始化 |
| `obj_error` | `fsc_error` | 模块错误码构造 |
| `obj_sub` | `fsc_sub` | 子错误分类 |
| `fuid` | `fsid` | 文件系统身份 |
| `objmeta` / `objruntime` | `namespace` | 运行时对象 |
| `objpool` | `nspool` | 运行时对象内存池 |
| `objtable` | `fstable` | 对象 / namespace 索引 |
| `objmgr` | `fsmgr` | 生命周期协调器 |

这样做的目的不是形式统一，而是让 MirageFS 的一级模块都具备相同的工程结构：

```text
xxx/
    xxx_init.*
    xxx_error.*
    xxx_sub.*
    子模块/
    文档/
```

后续 VFS、Cache、Server 等一级模块也可以沿用这套模式。

---

# 3. 目录结构

当前 FSC 目录如下：

```text
src/fsc/
├── fsc_init.c
├── fsc_init.h
├── fsc_error.c
├── fsc_error.h
├── fsc_sub.c
├── fsc_sub.h
├── fsc_sub_table.h
├── fsid/
├── namespace/
├── nspool/
├── fstable/
├── fsmgr/
├── mount/
└── policy/
```

其中：

* `fsid/` 负责分配 filesystem id
* `namespace/` 定义 `fsc_namespace_t`
* `nspool/` 负责 namespace 对象内存管理
* `fstable/` 负责 namespace 索引
* `fsmgr/` 负责生命周期编排
* `mount/` 当前预留
* `policy/` 当前预留

---

# 4. 系统不变量

FSC 引入 MirageFS 的第一个系统不变量：

```text
Invariant #1:

Every Object Belongs to Exactly One Filesystem.
```

含义：

* 每一个 Object 在创建时绑定唯一 FSID
* Object 生命周期内 FSID 不可修改
* 子对象自动继承父对象 FSID
* 跨 FSID 的对象移动必须失败
* 跨文件系统 rename 应返回 EXDEV 类错误

这条规则让系统分层更加清晰：

```text
FSC       管理 filesystem / namespace
Object    通过 FUID 保存对象归属
VFS       执行语义检查，但不修改归属
LSA       不理解 filesystem 概念
```

---

# 5. 命名规则

只有 `common/` 这类基础库使用宽泛的 `fs_*` 类型名前缀。

FSC 自己定义的公开类型必须使用 `fsc_` 前缀。

当前类型包括：

```text
fsc_fsid_t
fsc_namespace_t
fsc_namespace_state_t
fsc_table_t
fsc_table_entry_t
fsc_manager_t
```

函数前缀按子模块职责命名：

```text
fsid_*
fsc_namespace_*
nspool_*
fstable_*
fsmgr_*
```

这样既避免和 common 的 `fs_*` 混淆，也保持和 Object Layer 的 `objmgr_*`、`objtable_*`、`objpool_*` 风格一致。

---

# 6. 初始化顺序

FSC 初始化遵循依赖顺序：

```text
fsc_init()
    │
    ├── fs_sub_register(FS_MODULE_FSC, fsc_sub_name)
    │
    ├── fsid_init()
    │
    ├── nspool_init()
    │
    └── fsmgr_init()
```

销毁顺序相反：

```text
fsc_deinit()
    │
    ├── fsmgr_deinit()
    ├── nspool_deinit()
    └── fsid_deinit()
```

这样可以保证：

* manager 先释放所有 namespace
* namespace 释放完成后再销毁 nspool
* 最后销毁 FSID 分配器

---

# 7. 当前 API 范围

第一版 FSC 只实现最小闭环：

```text
fsmgr_create()

fsmgr_lookup()

fsmgr_lookup_fsid()

fsmgr_destroy()

fsmgr_get_root()
```

这条链路覆盖：

* FSID 分配
* Namespace 分配
* Namespace 初始化
* FSTable 注册
* lookup
* destroy
* 内存回收

Mount、Policy、rename 检查等属于后续增量功能。

---

# 8. 所有权模型

当前所有权规则如下：

```text
fsmgr_create()
    │
    ├── fsid_alloc()
    ├── nspool_alloc()
    ├── fsc_namespace_init()
    └── fstable_insert()
```

创建成功后：

* `fsmgr` 管理 namespace 生命周期
* `fstable` 只持有索引引用
* 调用者拿到的是借用指针
* 调用者不得释放 `fsc_namespace_t`

销毁时：

```text
fsmgr_destroy()
    │
    ├── fstable_remove()
    ├── fsid_free()
    └── nspool_free()
```

---

# 9. 文档同步规则

FSC 后续任何变更，只要涉及以下内容，都必须同步更新 `docs/modules/fsc/`：

* public API
* 结构体字段
* 生命周期状态
* 所有权规则
* 错误码语义
* 模块职责边界
* 初始化 / 销毁顺序
* 与 Object / VFS / LSA 的依赖关系

代码和文档必须保持一致。

---

# 10. 设计总结

FSC 是 MirageFS 文件系统控制平面的一级模块。

它通过 FSID、Namespace、NSPool、FSTable 和 FSMgr 形成第一版完整闭环：

```text
             FSC
      ┌───────┼───────┐
      │       │       │
    FSID  Namespace  FSTable
              │       │
            NSPool   Index
              │       │
              └──FSMgr┘
```

当前实现有意保持简单，只提供 create / lookup / destroy 的基础能力。

这样后续可以在稳定骨架上继续增加：

* Mount
* Policy
* rename 跨 FSID 检查
* readonly
* quota
* snapshot
* namespace acquire / release

FSC 的核心原则是：控制面统一管理 filesystem identity，Object Layer 只保存并使用该身份，二者边界清晰。
