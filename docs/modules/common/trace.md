# Trace Module

## Overview

Trace 模块用于在 MirageFS 内部建立统一调用链路（Call Chain）。

通过 Trace，可以将一次完整请求涉及的所有模块、所有函数调用关联到同一个上下文中。

例如：

```text
CLI
 └── VFS
      └── LSA
```

即使调用跨越多个模块、多个函数，仍然可以通过同一个 Trace 进行关联和追踪。

---

## Design Goals

### Request Correlation

将一次完整请求关联为一个 Trace。

例如：

```text
create file
lookup file
delete file
```

每次请求拥有独立 Trace。

---

### Automatic Propagation

业务代码无需显式传递：

```c
trace_id
span_id
parent_id
```

Trace Context 自动保存在 TLS 中。

---

### Lightweight

整个实现仅依赖：

```text
TLS
时间戳
线程ID
```

无需额外组件。

---

### Log Integration

Trace 与 Log 深度集成。

所有日志自动附带：

```text
trace_id
span_id
```

用于问题定位。

---

## Core Concepts

### Trace

表示一次完整请求。

例如：

```text
用户执行 mkdir
```

对应：

```text
trace_id = 1001
```

整个请求生命周期保持不变。

---

### Span

表示一次子操作。

例如：

```text
mkdir
 ├── lookup
 ├── inode_create
 └── dentry_insert
```

每个步骤拥有独立 Span。

```text
trace_id = 1001

span_id = 2001
span_id = 2002
span_id = 2003
```

---

### Parent Span

用于记录调用关系。

例如：

```text
mkdir
 └── inode_create
       └── disk_write
```

关系：

```text
span=disk_write

parent=inode_create
```

形成完整调用树。

---

## Trace Context

核心结构：

```c
typedef struct fs_trace_ctx {

    uint64_t trace_id;

    uint64_t span_id;

    uint64_t parent_id;

} fs_trace_ctx_t;
```

字段说明：

| Field     | Description |
| --------- | ----------- |
| trace_id  | 整个请求唯一标识    |
| span_id   | 当前操作标识      |
| parent_id | 调用方 Span    |

---

## TLS Architecture

Trace Context 保存在 TLS 中：

```c
extern __thread
fs_trace_ctx_t g_fs_trace_tls;
```

因此：

```text
每个线程独立 Trace Context
```

互不干扰。

---

## Architecture

```text
Thread
   │
   ▼
TLS
   │
   ▼
fs_trace_ctx_t
   │
   ├── trace_id
   ├── span_id
   └── parent_id
```

任何位置均可通过：

```c
FS_TRACE_GET()
```

获取当前 Trace。

无需层层传参。

---

## Trace Lifecycle

### Create Trace

请求入口创建新 Trace：

```c
fs_trace_ctx_t trace;

FS_TRACE_BEGIN(&trace);
```

生成：

```text
trace_id
span_id
```

并写入 TLS。

---

### Use Trace

业务函数内部：

```c
fs_trace_ctx_t *ctx =
    FS_TRACE_GET();
```

即可获取当前上下文。

---

### End Trace

请求结束：

```c
FS_TRACE_END();
```

清理 TLS。

---

## Span Mechanism

### Why Span

Trace 表示整个请求。

Span 表示请求中的一个步骤。

例如：

```text
create file
 ├── lookup
 ├── inode alloc
 └── disk write
```

Trace 不变：

```text
trace=1001
```

Span 不同：

```text
lookup       span=2001
inode alloc  span=2002
disk write   span=2003
```

---

## Automatic Scope Span

推荐使用：

```c
FS_TRACE_SPAN("inode_create");
```

示例：

```c
int inode_create(...)
{
    FS_TRACE_SPAN("inode_create");

    ...
}
```

进入函数时：

```text
BEGIN span
```

离开函数时：

```text
END span
```

自动执行。

---

## Scope Guard Design

内部实现：

```c
__attribute__((cleanup()))
```

机制。

因此：

```c
return;
goto;
break;
continue;
```

均能自动恢复父 Span。

例如：

```c
int foo(void)
{
    FS_TRACE_SPAN("foo");

    if (error)
        return -1;

    return 0;
}
```

即使提前返回：

```text
span end
```

仍然自动执行。

---

## Trace ID Generation

ID 生成基于：

```text
秒级时间
纳秒时间
线程ID
线程内序列号
```

组合生成。

实现：

```c
fs_trace_gen_id()
```

特点：

```text
轻量
无锁
线程安全
```

满足调试和日志需求。

---

## Log Integration

Log 模块自动获取：

```c
FS_TRACE_GET()
```

并输出：

```text
trace_id
span_id
```

示例：

```text
[INFO]
[trace=0x1001 span=0x2001]
create file success
```

因此业务代码无需：

```c
printf(
    "trace=%lu",
    trace_id);
```

---

## Typical Usage

### Request Entry

```c
int cli_create(...)
{
    fs_trace_ctx_t trace;

    FS_TRACE_BEGIN(&trace);

    ...

    FS_TRACE_END();
}
```

---

### Function Span

```c
int inode_create(...)
{
    FS_TRACE_SPAN(
        "inode_create");

    ...
}
```

---

### Nested Call

```text
trace=1001

cli_create
 └── vfs_create
      └── inode_create
           └── disk_write
```

日志中能够完整体现调用链。

---

## Cross Thread Propagation

默认情况下：

```text
TLS 不会跨线程传递
```

因此：

```text
Thread A
```

创建的 Trace：

不会自动出现在：

```text
Thread B
```

中。

---

### Correct Way

父线程：

```c
fs_trace_ctx_t ctx =
    *FS_TRACE_GET();
```

传递给子线程。

子线程：

```c
fs_trace_set(&ctx);
```

恢复上下文。

---

## Thread Safety

Trace 模块天然线程安全。

原因：

```text
每个线程拥有独立 TLS
```

不存在共享状态。

因此：

```text
无需锁
无需原子变量
```

---

## Relationship With Other Modules

### Used By

Trace 被以下模块使用：

* log
* cli
* ...

---

### Dependency

Trace 依赖：

* os

用于：

* 时间获取
* 线程ID获取

属于 Common 基础设施模块。

---

## Notes

### Request Boundary

原则：

```text
一次用户请求
对应一个 Trace
```

不要在同一请求过程中重新创建 Trace。

---

### Prefer Span

推荐：

```c
FS_TRACE_SPAN(...)
```

不推荐：

```c
fs_trace_span_begin(...)
fs_trace_span_end(...)
```

直接成对调用。

前者能够自动处理异常返回路径。

---

### Log First

Trace 的主要价值在于日志关联。

所有核心流程应同时配合：

```c
FS_TRACE_SPAN(...)
FS_LOG_DUMP_XXX(...)
```

使用。

这样可以快速还原完整执行路径。
