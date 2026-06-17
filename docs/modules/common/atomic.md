# Atomic 模块设计

## 1. 模块定位

Atomic 模块提供 MirageFS 的原子操作能力，用于在多线程环境下实现无锁同步。

该模块属于 Common 基础设施层，与 Lock、List、Hash、Mempool 等模块同级。

目录结构：

```text
common/
├── atomic/
├── lock/
├── list/
├── hash/
└── mempool/
```

Atomic 主要用于：

```text
引用计数（RefCnt）
状态计数（Counter）
统计信息（Statistics）
无锁状态切换（CAS）
```

典型使用场景：

```text
ObjMgr     -> 对象引用计数
Cache      -> 命中统计
Mempool    -> 使用量统计
FSMgr      -> 文件系统计数
VFS        -> 打开文件计数
```

---

## 2. 设计目标

### 2.1 保证线程安全

避免并发修改导致的数据竞争（Race Condition）。

例如：

```c
refcnt++;
```

在多个线程同时执行时可能产生计数错误。

Atomic 模块保证：

```text
读取安全
写入安全
递增安全
递减安全
CAS安全
```

---

### 2.2 固定宽度

MirageFS 内部大量使用固定宽度整数：

```c
uint32_t
int32_t
uint64_t
int64_t
```

因此 Atomic 模块采用：

```c
_Atomic int32_t
_Atomic int64_t
```

而不使用：

```c
atomic_int_fast32_t
atomic_int_fast64_t
```

原因：

```text
int_fast32_t 宽度不固定
不同平台可能不同
不利于文件系统统一设计
```

---

### 2.3 统一封装

业务模块不直接调用：

```c
atomic_fetch_add()
atomic_fetch_sub()
atomic_compare_exchange_strong()
```

统一通过：

```c
fs_atomic_xxx()
```

进行访问。

优点：

```text
接口统一
屏蔽平台差异
便于后续移植
```

---

## 3. 模块结构

```text
common/
└── atomic/
    ├── fs_atomic.h
    └── fs_atomic.c
```

---

## 4. 数据类型

### 4.1 32位原子变量

```c
typedef _Atomic int32_t fs_atomic32_t;
```

---

### 4.2 64位原子变量

```c
typedef _Atomic int64_t fs_atomic64_t;
```

---

## 5. 对外接口

### 5.1 初始化

初始化原子变量。

```c
void fs_atomic32_init(
                fs_atomic32_t *atom,
                int32_t value);

void fs_atomic64_init(
                fs_atomic64_t *atom,
                int64_t value);
```

示例：

```c
fs_atomic32_t refcnt;

fs_atomic32_init(&refcnt, 1);
```

---

### 5.2 读取

获取当前值。

```c
int32_t fs_atomic32_load(
                const fs_atomic32_t *atom);

int64_t fs_atomic64_load(
                const fs_atomic64_t *atom);
```

示例：

```c
int32_t cnt;

cnt = fs_atomic32_load(&refcnt);
```

---

### 5.3 写入

覆盖当前值。

```c
void fs_atomic32_store(
                fs_atomic32_t *atom,
                int32_t value);

void fs_atomic64_store(
                fs_atomic64_t *atom,
                int64_t value);
```

---

### 5.4 自增

执行：

```c
value++
```

返回：

```text
递增后的值
```

接口：

```c
int32_t fs_atomic32_inc(
                fs_atomic32_t *atom);

int64_t fs_atomic64_inc(
                fs_atomic64_t *atom);
```

示例：

```c
new_ref = fs_atomic32_inc(&refcnt);
```

---

### 5.5 自减

执行：

```c
value--
```

返回：

```text
递减后的值
```

接口：

```c
int32_t fs_atomic32_dec(
                fs_atomic32_t *atom);

int64_t fs_atomic64_dec(
                fs_atomic64_t *atom);
```

示例：

```c
new_ref = fs_atomic32_dec(&refcnt);
```

---

### 5.6 加法

执行：

```c
value += delta
```

返回：

```text
修改后的值
```

接口：

```c
int32_t fs_atomic32_add(
                fs_atomic32_t *atom,
                int32_t delta);

int64_t fs_atomic64_add(
                fs_atomic64_t *atom,
                int64_t delta);
```

---

### 5.7 减法

执行：

```c
value -= delta
```

返回：

```text
修改后的值
```

接口：

```c
int32_t fs_atomic32_sub(
                fs_atomic32_t *atom,
                int32_t delta);

int64_t fs_atomic64_sub(
                fs_atomic64_t *atom,
                int64_t delta);
```

---

### 5.8 Compare And Swap

CAS（Compare And Swap）是无锁编程的基础操作。

逻辑：

```text
如果当前值 == expected

    写入 desired
    返回 true

否则

    返回 false
    expected 被更新为当前值
```

接口：

```c
bool fs_atomic32_cas(
                fs_atomic32_t *atom,
                int32_t *expected,
                int32_t desired);

bool fs_atomic64_cas(
                fs_atomic64_t *atom,
                int64_t *expected,
                int64_t desired);
```

示例：

```c
int32_t expected = 0;

if (fs_atomic32_cas(
            &state,
            &expected,
            1))
{
    /* success */
}
```

---

## 6. Memory Order

当前实现统一采用：

```c
memory_order_seq_cst
```

即：

```text
Sequential Consistency
```

特点：

```text
最严格
最安全
最容易理解
```

适用于 MirageFS 当前阶段。

后续如有性能需求，可内部优化：

```text
Acquire
Release
Relaxed
```

但不影响对外接口。

---

## 7. 与 Lock 模块的关系

Atomic 不是 Lock 的替代品。

两者解决的问题不同。

### Atomic

适用于：

```text
简单变量修改
引用计数
状态切换
统计计数
```

例如：

```c
refcnt++
```

---

### Lock

适用于：

```text
多个变量同时修改
链表操作
哈希表操作
复杂事务
```

例如：

```c
插入Hash表
删除Hash表
修改多个字段
```

---

设计原则：

```text
简单操作 -> Atomic

复杂操作 -> Lock
```

---

## 8. 与 ObjMgr 的关系

Atomic 模块的首个核心使用场景是：

```text
ObjMgr 引用计数管理
```

示例：

```c
typedef struct ObjMeta
{
    ...

    fs_atomic32_t refcnt;

} ObjMeta_t;
```

增加引用：

```c
objmgr_get()
{
    fs_atomic32_inc(&meta->refcnt);
}
```

释放引用：

```c
objmgr_put()
{
    if (fs_atomic32_dec(&meta->refcnt) == 0)
    {
        ...
    }
}
```

通过 Atomic 可避免：

```text
引用计数丢失
并发修改错误
对象提前释放
```

---

## 9. 后续扩展

当前版本提供：

```text
init
load
store

inc
dec

add
sub

cas
```

后续可根据需要增加：

```text
inc_not_zero
dec_and_test

exchange

fetch_add
fetch_sub
```

原则：

```text
保持接口简单
按需扩展
避免过度设计
```

---

## 10. 总结

Atomic 模块是 MirageFS 的基础同步设施。

职责：

```text
提供轻量级线程安全原子操作
```

特点：

```text
固定宽度
跨平台
统一封装
线程安全
无锁同步
```

主要应用：

```text
ObjMgr
Cache
Mempool
FSMgr
VFS
```

推荐原则：

```text
简单共享变量 -> Atomic

复杂共享资源 -> Lock
```