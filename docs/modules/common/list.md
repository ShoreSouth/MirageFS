# List Module

## Overview

List 模块提供通用双向循环链表（Doubly Circular Linked List）能力。

该实现参考 Linux Kernel List 设计，采用 **Intrusive List（侵入式链表）** 模式，链表节点直接嵌入业务对象内部，而非额外分配独立节点对象。

模块特点：

* 双向链表
* 循环链表
* O(1) 插入
* O(1) 删除
* O(1) 移动
* 不负责内存管理
* 无锁设计
* Header Only

List 模块仅负责维护节点链接关系，不负责对象生命周期管理。

---

## Design Goals

### Generic

链表不依赖任何业务结构。

可用于：

* Cache
* Hash Bucket
* LRU
* Inode Manager
* Dentry Manager
* Object Manager
* Task Queue

等所有需要链表组织的数据结构。

### Zero Allocation

List 模块自身不申请内存。

所有节点均由业务对象提供。

### O(1) Operations

常见操作均为常数时间复杂度：

| Operation | Complexity |
| --------- | ---------- |
| Add Head  | O(1)       |
| Add Tail  | O(1)       |
| Delete    | O(1)       |
| Move      | O(1)       |
| Replace   | O(1)       |
| Splice    | O(1)       |

### Cache Friendly

链表节点嵌入对象内部：

```c
typedef struct CacheEntry {
    uint64_t key;
    uint64_t value;

    fs_list_head_t node;
} CacheEntry_t;
```

避免额外节点分配和指针跳转。

---

## Data Structure

### fs_list_head_t

链表节点结构：

```c
typedef struct fs_list_head {
    struct fs_list_head *next;
    struct fs_list_head *prev;
} fs_list_head_t;
```

每个节点同时保存：

* next
* prev

指向相邻节点。

---

## Circular List Layout

### Empty List

空链表状态：

```text
head
 ┌─────┐
 │  ●──┼──┐
 └─────┘  │
    ▲     │
    └─────┘
```

即：

```c
head->next = head;
head->prev = head;
```

判断空链表：

```c
fs_list_empty(head);
```

---

### Non-empty List

```text
head
 ↓
A <-> B <-> C
↑           ↓
└───────────┘
```

head 本身也是一个链表节点。

因此：

* 不需要 NULL 判断
* 不存在特殊头尾节点处理逻辑

---

## Initialization

### Static Initialization

```c
FS_LIST_HEAD(g_list);
```

等价于：

```c
fs_list_head_t g_list = {
    &g_list,
    &g_list
};
```

---

### Dynamic Initialization

```c
fs_list_head_t list;

fs_list_init(&list);
```

---

## Intrusive List Model

List 模块采用侵入式设计。

业务对象内部嵌入链表节点：

```c
typedef struct CacheEntry {

    uint64_t key;

    fs_list_head_t node;

} CacheEntry_t;
```

插入链表：

```c
fs_list_add_tail(
    &entry->node,
    &cache->lru_list);
```

通过节点反向获取对象：

```c
CacheEntry_t *entry =
    FS_LIST_ENTRY(node,
                  CacheEntry,
                  node);
```

---

## Core APIs

### Add

头插：

```c
fs_list_add(node, head);
```

结果：

```text
head <-> node <-> old_first
```

尾插：

```c
fs_list_add_tail(node, head);
```

结果：

```text
old_last <-> node <-> head
```

---

### Delete

删除节点：

```c
fs_list_del(node);
```

仅解除链接关系。

不会释放对象。

---

### Delete And Reinit

```c
fs_list_del_init(node);
```

删除后重新初始化：

```c
node->next = node;
node->prev = node;
```

适合避免重复删除导致链表损坏。

---

### Move

移动到头部：

```c
fs_list_move(node, head);
```

常用于：

* LRU 热点提升
* Task 优先级调整

移动到尾部：

```c
fs_list_move_tail(node, head);
```

---

### Replace

使用新节点替换旧节点：

```c
fs_list_replace(old, new);
```

链表位置保持不变。

---

### Splice

链表拼接：

```c
fs_list_splice(src, dst);
```

将：

```text
src : A B C
dst : X Y
```

变为：

```text
dst : A B C X Y
```

时间复杂度：

```text
O(1)
```

适合：

* 批量迁移
* 队列合并
* Cache Flush

---

## Traversal

### Raw Traversal

```c
fs_list_head_t *pos;

FS_LIST_FOR_EACH(pos, &list) {

}
```

反向遍历：

```c
FS_LIST_FOR_EACH_PREV(pos, &list) {

}
```

---

### Safe Traversal

遍历过程中允许删除：

```c
fs_list_head_t *pos;
fs_list_head_t *n;

FS_LIST_FOR_EACH_SAFE(
    pos,
    n,
    &list) {

    fs_list_del(pos);
}
```

推荐在删除场景使用。

---

## Typed Traversal

定义对象：

```c
typedef struct CacheEntry {

    uint64_t key;

    fs_list_head_t node;

} CacheEntry;
```

遍历：

```c
CacheEntry *entry;

FS_LIST_FOR_EACH_ENTRY(
    entry,
    &cache->list,
    node) {

}
```

无需手动调用：

```c
FS_LIST_ENTRY(...)
```

代码更简洁。

---

## Memory Ownership

List 模块不管理对象生命周期。

允许：

```c
fs_list_add(...)
fs_list_del(...)
```

不允许：

```c
free(node);
```

对象释放由业务模块负责。

---

## Thread Safety

List 模块不提供任何同步机制。

调用方负责加锁。

典型组合：

```c
pthread_mutex_t
pthread_rwlock_t
fs_spinlock_t
```

---

## Typical Usage

### LRU Cache

```text
Head
 ↓
Hot
 ↓
Warm
 ↓
Cold
```

访问对象：

```c
fs_list_move(
    &entry->node,
    &cache->lru);
```

提升至头部。

---

### Hash Bucket

```text
bucket
 ↓
obj1
 ↓
obj2
 ↓
obj3
```

用于解决哈希冲突。

---

### Task Queue

```text
head
 ↓
task1
 ↓
task2
 ↓
task3
```

实现简单任务调度。

---

## Notes

### 删除后建议重新初始化

推荐：

```c
fs_list_del_init(node);
```

而不是：

```c
fs_list_del(node);
```

避免节点被误用。

---

### 遍历删除必须使用 SAFE 宏

正确：

```c
FS_LIST_FOR_EACH_SAFE(...)
```

错误：

```c
FS_LIST_FOR_EACH(...)
{
    fs_list_del(...)
}
```

否则可能导致遍历失效。

---

## Relationship With Other Modules

List 是 Mirage 基础容器模块之一。

主要被以下模块依赖：

* cache
* hash
* objmeta
* inode
* dentry
* scheduler
* task queue

属于 Common 层核心基础设施。
