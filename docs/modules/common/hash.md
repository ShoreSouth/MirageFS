# Hash Module

## 1. 模块概述

Hash 模块为 MirageFS 提供通用 Hash Table 容器。

当前实现采用：

```text
Hash Table
    +
Separate Chaining
    +
fs_list
```

方式解决 Hash 冲突。

特点：

* 通用容器
* O(1) 平均查找复杂度
* 支持任意业务对象嵌入
* 与 fs_list 模块深度集成
* 无对象拷贝
* 无额外 KV 包装层

适用于：

* ObjTable
* Cache
* 索引结构
* 元数据管理

---

## 2. 设计目标

Hash 模块定位于：

```text
Common Infrastructure
```

不感知业务对象类型。

Hash 模块仅负责：

```text
Key
    ↓
Bucket
    ↓
List
```

映射管理。

业务对象通过回调函数提供：

* Key 获取
* Key 匹配

Hash 模块不解析具体对象内容。

---

## 3. 整体架构

```text
                Hash Table
                      │
      ┌───────────────┼───────────────┐
      │               │               │
      ▼               ▼               ▼

   Bucket0         Bucket1         Bucket2

      │               │               │

      ▼               ▼               ▼

   fs_list        fs_list         fs_list

      │
      ▼

 ObjectEntry
```

---

## 4. Hash 冲突处理

当前采用：

```text
Separate Chaining
```

即：

每个 Bucket 维护一个链表。

多个 Key 映射到同一个 Bucket 时：

```text
Bucket
   │
   ▼

EntryA
   │
   ▼

EntryB
   │
   ▼

EntryC
```

通过链表进行管理。

优点：

* 实现简单
* 删除效率高
* 适合当前项目规模

---

## 5. 数据结构

### fs_hash_t

Hash Table 主结构。

```c
typedef struct fs_hash {

    uint32_t bucket_nr;

    uint64_t entry_nr;

    fs_list_head_t *buckets;

    fs_hash_node_hash_fn node_hash_fn;

    fs_hash_key_hash_fn key_hash_fn;

    fs_hash_match_fn match_fn;

} fs_hash_t;
```

字段说明：

| 字段           | 说明          |
| ------------- | ------------ |
| bucket_nr     | bucket数量    |
| entry_nr      | 当前节点数量    |
| buckets       | bucket数组    |
| node_hash_fn  | 节点哈希函数    |
| key_hash_fn   | Key哈希函数    |
| match_fn      | 节点匹配函数    |

---

## 6. 回调机制

Hash 模块通过三组回调函数实现通用 Key 支持。

业务模块在 `fs_hash_init()` 时注册回调，
Hash 模块在不同操作阶段调用对应回调。

---

### 6.1 节点哈希 (node_hash_fn)

用于 Hash 插入阶段，
从链表节点计算哈希值。

```c
typedef uint64_t (*fs_hash_node_hash_fn)(
    const fs_list_head_t *node);
```

示例：

```c
static uint64_t objtable_node_hash(
    const fs_list_head_t *node)
{
    const objtable_entry_t *entry;

    entry = FS_CONTAINER_OF(
        node,
        objtable_entry_t,
        node);

    return objkey_hash(&entry->meta.key);
}
```

---

### 6.2 Key 哈希 (key_hash_fn)

用于 Hash 查找阶段，
从外部 Key 计算哈希值。

Key 类型为 `const void *`，
支持任意业务自定义 Key。

```c
typedef uint64_t (*fs_hash_key_hash_fn)(
    const void *key);
```

示例：

```c
static uint64_t objtable_key_hash(
    const void *key)
{
    return objkey_hash(key);
}
```

---

### 6.3 节点匹配 (match_fn)

用于 Hash 查找阶段，
判断链表节点是否匹配给定 Key。

```c
typedef bool (*fs_hash_match_fn)(
    const fs_list_head_t *node,
    const void *key);
```

示例：

```c
static bool objtable_match(
    const fs_list_head_t *node,
    const void *key)
{
    const objtable_entry_t *entry;

    entry = FS_CONTAINER_OF(
        node,
        objtable_entry_t,
        node);

    return objkey_equal(&entry->meta.key, key);
}
```

---

## 7. 生命周期

### 初始化

```c
int fs_hash_init(
    fs_hash_t *hash,
    uint32_t bucket_nr,
    fs_hash_node_hash_fn node_hash_fn,
    fs_hash_key_hash_fn key_hash_fn,
    fs_hash_match_fn match_fn);
```

功能：

* 创建 Bucket 数组
* 初始化 Bucket 链表头
* 注册回调函数

---

### 销毁

```c
void fs_hash_destroy(
    fs_hash_t *hash);
```

说明：

仅释放 Hash Table 内部资源。

业务对象生命周期由调用者管理。

---

## 8. 基础操作

### 插入

```c
int fs_hash_insert(
    fs_hash_t *hash,
    fs_list_head_t *node);
```

流程：

```text
Key
 ↓

Hash
 ↓

Bucket
 ↓

List Tail
```

---

### 删除

```c
void fs_hash_remove(
    fs_hash_t *hash,
    fs_list_head_t *node);
```

说明：

节点必须已存在于 Hash Table。

---

### 查找

```c
fs_list_head_t *fs_hash_lookup(
    fs_hash_t *hash,
    const void *key);
```

返回：

```text
NULL
    未找到

node
    找到
```

---

## 9. 时间复杂度

| 操作           | 平均复杂度 |
| ------------ | ----- |
| Insert       | O(1)  |
| Remove       | O(1)  |
| Lookup       | O(1)  |
| Worst Lookup | O(n)  |

其中：

```text
n = Bucket链表长度
```

---

## 10. 使用示例

业务对象：

```c
typedef struct objtable_entry {

    obj_meta_t meta;

    fs_list_head_t node;

} objtable_entry_t;
```

初始化：

```c
fs_hash_init(
    &table,
    1024,
    objtable_node_hash,
    objtable_key_hash,
    objtable_match);
```

插入：

```c
fs_hash_insert(
    &table,
    &entry->node);
```

查找：

```c
obj_key_t key = objkey_make(objectid, gen);

node = fs_hash_lookup(
    &table,
    &key);
```

---

## 11. 当前限制

当前版本：

* Bucket 数量固定
* 不支持动态扩容
* 不支持线程安全
* 不支持遍历接口

适用于：

```text
MirageFS V1
```

单进程、单线程场景。

后续可扩展：

* 动态扩容
* 自定义 Hash 算法
* 线程安全封装
* 泛型 Key 支持
* Iterator 支持
