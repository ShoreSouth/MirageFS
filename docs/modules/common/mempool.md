# Mempool Module

## Overview

Mempool 模块是 MirageFS 的统一内存管理子系统。

负责：

* 内存池管理
* 动态内存分配
* Buddy Allocator
* 内存统计
* 内存调试
* 后续 Slab 分配器扩展

MirageFS 中绝大部分动态内存最终都应来自 Mempool。

系统默认同时提供：

```text
Global Memory Pool
+
Custom Memory Pool
```

两种使用模式。

---

## Design Goals

### Eliminate Direct malloc/free

统一内存来源：

```c
fs_malloc()
fs_free()
```

而不是：

```c
malloc()
free()
```

这样可以：

* 统计内存使用
* 检测泄漏
* 统一调试
* 支持对象缓存

---

### High Performance

当前采用：

```text
Buddy Allocator
```

特点：

```text
O(logN) 分配
O(logN) 释放
快速合并
碎片可控
```

适合文件系统场景。

---

### Debug Friendly

支持：

```text
Magic Check
Poison Memory
Pool Verify
Statistics
Allocation Tracking
```

便于定位问题。

---

### Future Expandability

预留：

```text
Slab
Thread Cache
NUMA
HugePage
Lock-Free Fast Path
```

扩展能力。

---

## Architecture

```text
                    +------------------+
                    | fs_malloc()      |
                    | fs_free()        |
                    +---------+--------+
                              |
                              v
                    +------------------+
                    | Global Mempool   |
                    +---------+--------+
                              |
                              v
                    +------------------+
                    | Buddy Allocator  |
                    +---------+--------+
                              |
                              v
                    +------------------+
                    | Memory Region    |
                    +------------------+
```

未来：

```text
Buddy
   |
   +---- Slab Cache
   |
   +---- Thread Cache
```

---

## Memory Layout

每个分配块布局：

```text
+--------------------+
| fs_mp_hdr_t        |
+--------------------+
| User Data          |
+--------------------+
```

用户只能看到：

```text
User Data
```

Header 对用户不可见。

### Allocated Block

```text
+------------------------------------+
| fs_mp_hdr_t                        |
| magic                              |
| order                              |
| req_size                           |
+------------------------------------+
| user buffer                        |
+------------------------------------+
```

### Free Block

空闲状态：

```text
+------------------------------------+
| fs_mp_block_t                      |
| list node                          |
+------------------------------------+
```

注意：

```text
fs_mp_hdr_t
fs_mp_block_t
```

共用同一块内存。

---

## Buddy Allocator Design

### Page Size

固定：

```text
4KB
```

定义：

```c
FS_MP_PAGE_SIZE
```

---

### Order

块大小：

```text
order 0 -> 4KB
order 1 -> 8KB
order 2 -> 16KB
...
order N -> 4KB * 2^N
```

---

### Maximum Order

当前：

```text
FS_MP_MAX_ORDER = 16
```

最大块：

```text
4KB << 16
=
256MB
```

---

### Buddy Formula

Buddy 算法核心：

```text
buddy = offset XOR block_size
```

实现：

```c
fs_mp_buddy_ptr()
```

---

## Core Objects

### fs_mempool_t

内存池核心对象。

负责：

```text
Pool Metadata
Free Lists
Statistics
Lock
```

管理。

核心成员：

```c
base
total_size
max_order
free_area[]
stats
lock
```

---

### fs_mp_hdr_t

分配头。

记录：

```text
Magic
Order
Flags
Requested Size
```

用于：

```text
Free
Verify
Debug
```

场景。

---

### fs_mp_block_t

空闲块描述符。

仅在：

```text
FREE
```

状态下有效。

用于挂入：

```text
free_area[order]
```

链表。

---

## Allocation Flow

分配流程：

```text
fs_mp_alloc()

    |
    v

calc order

    |
    v

find free block

    |
    v

split if needed

    |
    v

setup header

    |
    v

return user ptr
```

### Order Calculation

用户申请：

```text
size
```

实际需要：

```text
sizeof(hdr)
+
size
```

然后计算最小可容纳 Order。

实现：

```c
fs_mp_calc_order()
```

### Split

如果找到更大的块：

```text
order 8
```

需要：

```text
order 4
```

则执行：

```text
split
split
split
split
```

直到目标阶。

---

## Free Flow

释放流程：

```text
user ptr

    |
    v

hdr

    |
    v

verify

    |
    v

find buddy

    |
    v

merge

    |
    v

insert free list
```

---

## Buddy Merge Algorithm

释放后检查 Buddy 是否空闲。

如果：

```text
同阶
且空闲
```

则：

```text
merge
```

升级到：

```text
order + 1
```

继续尝试。

直到：

```text
无法合并
```

或：

```text
max_order
```

为止。

---

## Global Memory Pool

为了简化使用，提供：

```c
fs_malloc()
fs_free()
fs_zalloc()
fs_realloc()
```

接口。

内部等价：

```c
fs_mp_alloc(fs_mp_global(), ...)
```

### Initialization

启动阶段：

```c
fs_mp_global_init(...)
```

创建全局池。

### Shutdown

退出阶段：

```c
fs_mp_global_fini()
```

销毁全局池。

---

## Statistics

统计结构：

```c
fs_mp_stats_t
```

### Capacity

```text
total_bytes
used_bytes
free_bytes
```

### Allocation

```text
alloc_count
free_count
alloc_fail_count
```

### Buddy Behavior

```text
split_count
merge_count
```

### Runtime

```text
current_allocs
peak_used_bytes
```

---

## Debug Features

### Magic Check

分配：

```text
FS_MP_MAGIC_ALLOC
```

释放：

```text
FS_MP_MAGIC_FREE
```

用于检测：

```text
double free
invalid free
memory corruption
```

---

### Poison

分配：

```text
0xAA
```

释放：

```text
0xDD
```

用于发现：

```text
未初始化访问
释放后访问
```

问题。

---

### Verify

完整一致性检查：

```c
fs_mp_verify()
```

检查：

```text
Free Lists
Statistics
Block State
Buddy Structure
```

是否正确。

---

## Thread Safety

支持：

```text
FS_MP_F_THREAD_SAFE
```

模式。

当前实现：

```text
One Pool
One Mutex
```

即：

```c
fs_mutex_t lock;
```

保护整个 Pool。

### Current Characteristics

优点：

```text
简单
稳定
易调试
```

缺点：

```text
高并发下存在锁竞争
```

---

## Slab Extension

Mempool 已预留：

```c
fs_slab_cache_t
```

接口。

未来实现：

```text
Object Cache
```

用于：

```text
inode
dentry
objmeta
cache entry
```

等固定大小对象。

### Architecture

```text
Buddy Allocator
        |
        +---- Slab Cache
                  |
                  +---- inode
                  +---- dentry
                  +---- objmeta
```

---

## Dependency Relationship

依赖：

```text
list
lock
assert
log
```

关系：

```text
mempool
│
├── list
├── lock
├── assert
└── log
```

属于 MirageFS Common 层核心模块。

---

## Coding Guidelines

推荐：

```c
fs_malloc()
fs_free()
```

作为默认接口。

大对象：

```c
fs_mp_alloc(mp, size)
```

使用专属 Pool。

固定大小对象未来优先使用：

```text
Slab
```

禁止：

```c
malloc()
free()
```

直接出现在业务模块。

---

## Future Roadmap

计划扩展：

```text
Thread Cache

Per-CPU Cache

Slab Allocator

NUMA Awareness

Huge Page Support

Memory Leak Detector

Allocation Trace

Lock-Free Fast Path
```

最终目标：

```text
MirageFS Unified Memory Subsystem
```

统一管理整个文件系统的动态内存生命周期。
