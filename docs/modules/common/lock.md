# Lock Module

## Overview

Lock 模块是 MirageFS 的基础同步原语封装层。

基于 Linux/POSIX `pthread` 实现，对外提供统一的：

* Mutex（互斥锁）
* RWLock（读写锁）

接口。

同时增加：

* Magic 校验
* Debug 能力
* Owner 记录
* Recursive Mutex 支持

为后续：

* 锁竞争统计
* 锁追踪（Lock Trace）
* 死锁检测（Deadlock Detection）

预留扩展空间。

---

## Design Goals

### Unified Lock Interface

业务代码统一使用：

```c
fs_mutex_xxx()
fs_rwlock_xxx()
```

而不是直接调用：

```c
pthread_mutex_xxx()
pthread_rwlock_xxx()
```

降低平台耦合。

---

### Debug Friendly

锁对象携带：

```text
Magic
Name
Owner
Flags
```

便于：

* 错误定位
* 未初始化检查
* 野指针检查
* 锁状态分析

---

### Future Expandability

后续可扩展：

```text
Lock Statistics

Contention Statistics

Lock Trace

Deadlock Detection

Performance Analysis
```

无需修改业务代码。

---

## Architecture

```text
Application
      |
      v
+----------------+
| fs_mutex       |
| fs_rwlock      |
+----------------+
      |
      v
pthread
      |
      v
Linux Kernel
```

---

# Supported Lock Types

当前支持：

```text
Mutex
RWLock
```

---

## Mutex

互斥锁。

特点：

```text
单线程持有
独占访问
阻塞等待
```

适用于：

```text
共享变量

统计信息

配置管理

链表操作

内存池
```

### Internal Structure

```c
typedef struct fs_mutex {

    uint32_t magic;

    uint32_t flags;

    const char *name;

    pthread_t owner;

    pthread_mutex_t mutex;

} fs_mutex_t;
```

---

### Member Description

#### magic

对象合法性校验。

用于检测：

```text
未初始化锁

已销毁锁

非法内存访问
```

有效值：

```c
FS_MUTEX_MAGIC
```

---

#### flags

锁属性。

支持：

```c
FS_LOCK_F_DEBUG
FS_LOCK_F_RECURSIVE
```

---

#### name

锁名称。

主要用于：

```text
日志

调试

故障分析
```

例如：

```c
"inode_lock"

"dentry_cache_lock"

"mempool_lock"
```

---

#### owner

当前持锁线程。

仅调试用途。

用于：

```text
锁分析

错误定位
```

---

#### mutex

底层 pthread mutex。

业务层不可直接访问。

---

## Read Write Lock

读写锁。

特点：

```text
多读并发

写独占
```

适用于：

```text
读多写少场景
```

例如：

```text
inode cache

dentry cache

metadata cache
```

---

### Internal Structure

```c
typedef struct fs_rwlock {

    uint32_t magic;

    uint32_t flags;

    const char *name;

    pthread_rwlock_t rwlock;

} fs_rwlock_t;
```

---

### Reader Behavior

多个线程可同时持有：

```text
Read Lock
```

示例：

```text
Thread A  ---- RDLOCK
Thread B  ---- RDLOCK
Thread C  ---- RDLOCK
```

允许并发执行。

---

### Writer Behavior

写锁独占。

示例：

```text
Thread A ---- WRLOCK

Thread B ---- BLOCK

Thread C ---- BLOCK
```

直到：

```text
Thread A Unlock
```

之后才能继续。

---

# Magic Validation

所有锁对象均包含：

```c
magic
```

创建时：

```text
Mutex  -> FS_MUTEX_MAGIC

RWLock -> FS_RWLOCK_MAGIC
```

销毁后：

```text
memset(...,0)
```

后续访问：

```c
FS_ASSERT(lock->magic == ...)
```

能够快速发现：

```text
未初始化对象

重复销毁

非法访问
```

问题。

---

# Lock Flags

## FS_LOCK_F_DEBUG

启用调试能力。

当前：

```text
Magic Check

Owner Record
```

未来：

```text
Lock Statistics

Lock Trace

Deadlock Detection
```

---

## FS_LOCK_F_RECURSIVE

递归互斥锁。

允许：

```text
同线程重复加锁
```

示例：

```text
lock()

    └── funcA()

            └── lock()
```

不会死锁。

底层：

```c
PTHREAD_MUTEX_RECURSIVE
```

实现。

### Recommendation

仅在必要场景使用。

一般业务代码优先：

```text
普通 Mutex
```

避免隐藏设计问题。

---

# Mutex Lifecycle

## Initialization

```c
fs_mutex_init(
    &lock,
    "mempool_lock",
    FS_LOCK_F_DEBUG
);
```

内部：

```text
pthread_mutex_init()

magic初始化

owner清空
```

---

## Lock

```c
fs_mutex_lock(&lock);
```

行为：

```text
获取成功
    ↓

记录owner

进入临界区
```

若锁被占用：

```text
当前线程阻塞等待
```

---

## Try Lock

```c
if (fs_mutex_trylock(&lock)) {

}
```

特点：

```text
非阻塞

立即返回
```

返回：

```text
true  获取成功

false 获取失败
```

---

## Unlock

```c
fs_mutex_unlock(&lock);
```

行为：

```text
清空owner

释放mutex
```

---

## Destroy

```c
fs_mutex_destroy(&lock);
```

要求：

```text
无持锁线程

无等待线程
```

否则行为未定义。

---

# RWLock Lifecycle

## Initialization

```c
fs_rwlock_init(
    &lock,
    "inode_cache",
    FS_LOCK_F_DEBUG
);
```

---

## Read Lock

```c
fs_rwlock_rdlock(&lock);
```

允许：

```text
多个Reader同时进入
```

---

## Write Lock

```c
fs_rwlock_wrlock(&lock);
```

要求：

```text
无Reader

无Writer
```

---

## Try Read Lock

```c
if (fs_rwlock_tryrdlock(&lock)) {

}
```

非阻塞。

---

## Try Write Lock

```c
if (fs_rwlock_trywrlock(&lock)) {

}
```

非阻塞。

---

## Unlock

```c
fs_rwlock_unlock(&lock);
```

统一释放：

```text
Read Lock

Write Lock
```

---

## Destroy

```c
fs_rwlock_destroy(&lock);
```

销毁底层：

```c
pthread_rwlock_destroy()
```

---

# Thread Safety

Lock 模块本身完全线程安全。

内部依赖：

```text
pthread mutex

pthread rwlock
```

实现同步。

---

# Typical Usage

## Protect Shared Counter

```c
static fs_mutex_t counter_lock;

fs_mutex_lock(&counter_lock);

counter++;

fs_mutex_unlock(&counter_lock);
```

---

## Protect Cache

```c
static fs_rwlock_t cache_lock;
```

读路径：

```c
fs_rwlock_rdlock(&cache_lock);

lookup_cache();

fs_rwlock_unlock(&cache_lock);
```

写路径：

```c
fs_rwlock_wrlock(&cache_lock);

update_cache();

fs_rwlock_unlock(&cache_lock);
```

---

# Dependency Relationship

依赖：

```text
pthread

assert
```

关系：

```text
lock
│
├── pthread
└── assert
```

属于 MirageFS Common 基础设施层。

---

# Current Limitations

当前实现：

```text
单纯封装pthread
```

暂未实现：

```text
Lock Statistics

Contention Statistics

Deadlock Detection

Lock Trace

Lock Timeout
```

---

# Future Roadmap

计划扩展：

```text
Lock Contention Statistics

Per-Lock Wait Time

Deadlock Detection

Lock Dependency Graph

Trace Integration

Performance Analysis
```

最终形成：

```text
MirageFS Lock Subsystem
```

统一管理整个文件系统同步机制。
