# Object Module

## 1. 模块概述

Object 模块负责管理 MirageFS 全局对象信息。

当前版本主要提供：

```text
(objectid, gen) -> ObjMeta
```

映射关系维护。

Object 模块是 MirageFS 元数据体系中的核心组件之一。

其职责是：

```text
Object Identity (objectid, gen)
        ↓
     Object Table
        ↓
      ObjMeta
        ↓
 Linux Backend Object
```

建立 MirageFS 对象与 Linux 后端对象之间的关联。

---

## 2. 设计目标

Object 模块负责解决以下问题：

### 对象定位

已知：

```text
objkey_t (objectid, gen)
```

快速获取：

```text
ObjMeta
```

从而定位对应 Linux 对象。

---

### 对象注册

当文件或目录创建时：

```text
create
    ↓
生成 (objectid, gen)
    ↓
生成 ObjMeta
    ↓
注册到 ObjTable
```

建立全局对象映射关系。

---

### 对象生命周期管理

负责：

```text
insert
lookup
remove
```

等对象管理操作。

---

## 3. 当前架构

```text
              VFS
               │
               ▼

           ObjTable

               │

    (objectid, gen) -> ObjMeta

               │

               ▼

            ObjMeta

               │

               ▼

    mount_id + file_handle
```

---

## 4. ObjMeta 与 ObjTable

### ObjMeta

ObjMeta 描述单个对象。

保存：

```text
objectid
gen
mount_id
file_handle
```

等后端定位信息。

其本质属于：

```text
Value
```

对象。

---

### ObjTable

ObjTable 维护：

```text
(objectid, gen) -> ObjMeta
```

映射关系。

其本质属于：

```text
Key-Value Container
```

对象。

---

二者关系：

```text
ObjTable
    ↓
ObjMeta
```

即：

```text
一个ObjTable
管理多个ObjMeta
```

---

## 5. 数据结构

### objtable_entry_t

对象表节点。

```c
typedef struct objtable_entry {

    ObjMeta_t meta;

    fs_list_head_t node;

} objtable_entry_t;
```

说明：

* meta 保存对象元数据
* node 用于挂接 Hash Bucket

---

### objtable_t

对象表主体。

```c
typedef struct objtable {

    fs_hash_t table;

} objtable_t;
```

内部使用：

```text
fs_hash
    +
fs_list
```

实现高效查找。

---

## 6. Hash 架构

ObjTable 不直接维护链表和 Bucket。

而是复用：

```text
common/hash
```

模块。

整体结构：

```text
ObjTable

    │

    ▼

 fs_hash

    │

    ▼

 Bucket

    │

    ▼

 ObjTableEntry
```

---

## 7. 生命周期

### 初始化

```c
objtable_init()
```

创建：

```text
Hash Bucket
Callback
统计信息
```

---

### 销毁

```c
objtable_destroy()
```

释放：

```text
ObjTableEntry
Hash Bucket
```

资源。

---

## 8. 基础操作

### 插入

```c
objtable_insert()
```

建立：

```text
(objectid, gen) -> ObjMeta
```

映射。

---

### 查找

```c
objtable_lookup()
```

根据：

```text
objkey_t (objectid, gen)
```

获取：

```text
ObjMeta
```

---

### 删除

```c
objtable_remove()
```

删除对象映射。

---

### 判断是否存在

```c
objtable_exists()
```

用于快速存在性检查。

---

### 获取数量

```c
objtable_count()
```

返回当前对象数量。

---

## 9. 当前设计特点

### 基于 Hash

查找复杂度：

```text
Average O(1)
```

适合频繁 Lookup 场景。

---

### 单进程

当前版本：

```text
Single Process
```

无需跨进程同步。

---

### 单线程

当前版本：

```text
Single Thread
```

不涉及锁管理。

---

### 内存驻留

所有对象信息保存在内存中。

当前不支持：

```text
持久化
回刷
恢复
```

功能。

---

## 10. 与其他模块关系

### FUID

提供对象身份标识。

```text
FUID
    ↓
Object
```

---

### ObjMeta

提供对象定位信息。

```text
ObjTable
    ↓
ObjMeta
```

---

### Hash

提供底层 KV 容器能力。

```text
ObjTable
    ↓
fs_hash
```

---

### VFS

通过 ObjTable 完成对象查找。

```text
VFS
    ↓
ObjTable
    ↓
ObjMeta
```

---

## 11. 未来规划

未来版本可能增加：

### ObjMgr

对象管理服务层。

```text
VFS
    ↓
ObjMgr
    ↓
ObjTable
```

负责：

* Cache
* Persistence
* Recovery

等高级功能。

---

### 持久化

支持：

```text
(objectid, gen) -> ObjMeta
```

落盘保存。

系统重启后自动恢复。

---

### Cache

支持：

```text
LRU
ARC
```

等缓存策略。

---

### Snapshot

支持对象历史版本管理。

---

## 12. 当前范围

MirageFS V1 中：

Object 模块仅负责：

```text
(objectid, gen) -> ObjMeta
```

映射管理。

不负责：

```text
目录结构
路径解析
权限管理
Snapshot
Dedup
Compression
Encryption
```

等高级功能。
