# OS Module

## Overview

OS 模块为 MirageFS 提供统一运行时环境接口。

主要负责：

* 线程信息获取
* 进程信息获取
* 时间获取
* 时间格式化

该模块屏蔽平台差异，为上层模块提供统一接口。

常用于：

* Log
* Trace
* Scheduler
* Performance Statistics

等基础设施模块。

---

## Design Goals

### Unified Runtime APIs

统一获取：

```text
线程ID
线程名称
进程名称
时间戳
格式化时间
```

避免业务代码直接依赖：

```c
pthread_self()
clock_gettime()
getpid()
localtime_r()
```

---

### Lightweight

所有接口均为轻量级封装。

无复杂状态管理。

无动态内存分配。

---

### Cache Friendly

高频访问数据采用缓存机制。

例如：

```text
线程名称
进程名称
```

只获取一次。

后续直接复用。

---

### Thread Safe

所有公开接口均可安全用于多线程环境。

---

## Architecture

```text
Business Module
       │
       ▼
     fs_os
       │
       ├── pthread
       ├── clock_gettime
       ├── /proc
       └── localtime_r
```

OS 模块作为 Common 层基础运行时组件。

---

# Thread APIs

## Thread ID

获取当前线程唯一标识：

```c
uint64_t fs_get_tid(void);
```

示例：

```c
uint64_t tid = fs_get_tid();
```

当前实现：

```c
pthread_self()
```

封装返回。

---

## Thread Name

获取当前线程名称：

```c
const char* fs_get_thread_name(void);
```

示例：

```c
const char *name =
    fs_get_thread_name();
```

返回结果：

```text
worker-12345
```

或者：

```text
tid-12345
```

---

### TLS Cache

线程名称采用 TLS 缓存：

```c
static __thread
char tls_thread_name[64];
```

首次调用：

```text
pthread_getname_np()
```

后续直接返回缓存。

避免重复系统调用。

---

### Naming Strategy

若线程已设置名称：

```text
worker
```

最终格式：

```text
worker-12345
```

其中：

```text
12345
```

为线程 ID。

若线程没有名称：

```text
tid-12345
```

---

# Process APIs

## Process Name

获取当前进程名称：

```c
const char*
fs_get_process_name(void);
```

示例：

```c
const char *proc =
    fs_get_process_name();
```

返回：

```text
miragefs
```

---

### Implementation

当前实现通过：

```text
/proc/self/comm
```

读取进程名称。

例如：

```text
miragefs
```

---

### Global Cache

进程名称采用进程级缓存：

```c
static char proc_name[64];
```

首次读取后永久缓存。

避免重复访问：

```text
/proc/self/comm
```

---

# Time APIs

OS 模块提供两类时间：

```text
Real Time
Monotonic Time
```

---

## Real Time

真实世界时间。

来源：

```c
CLOCK_REALTIME
```

适用于：

* 日志
* 文件时间
* 用户显示

---

### Seconds

```c
uint64_t fs_get_time_s(void);
```

---

### Milliseconds

```c
uint64_t fs_get_time_ms(void);
```

---

### Microseconds

```c
uint64_t fs_get_time_us(void);
```

---

### Nanoseconds

```c
uint64_t fs_get_time_ns(void);
```

---

## Monotonic Time

单调时间。

来源：

```c
CLOCK_MONOTONIC
```

特点：

```text
不会被系统校时影响
不会回退
```

适用于：

* 性能统计
* 超时计算
* Benchmark

---

### Seconds

```c
uint64_t fs_get_monotonic_s(void);
```

---

### Milliseconds

```c
uint64_t fs_get_monotonic_ms(void);
```

---

### Microseconds

```c
uint64_t fs_get_monotonic_us(void);
```

---

### Nanoseconds

```c
uint64_t fs_get_monotonic_ns(void);
```

---

# Time String

## API

```c
const char* fs_time_str(void);
```

返回格式：

```text
YYYY-MM-DD HH:MM:SS.mmm
```

示例：

```text
2026-05-30 15:30:25.123
```

---

## Usage

典型用于日志：

```c
FS_LOG_DUMP_INFO(
    "create success");
```

内部：

```text
[2026-05-30 15:30:25.123]
```

---

## Implementation

实现流程：

```text
get_time_ms()
      │
      ▼
localtime_r()
      │
      ▼
strftime()
      │
      ▼
append milliseconds
```

最终生成字符串。

---

# Typical Usage

## Log Module

```c
fprintf(fp,
    "[%s]",
    fs_time_str());
```

用于生成日志时间。

---

## Trace Module

```c
uint64_t id =
    fs_trace_gen_id();
```

内部依赖：

```c
fs_get_time_s();
fs_get_time_ns();
```

生成 Trace ID。

---

## Performance Statistics

```c
uint64_t start =
    fs_get_monotonic_us();

...

uint64_t cost =
    fs_get_monotonic_us() - start;
```

计算执行耗时。

---

# Thread Safety

## Thread Name

线程名称使用：

```c
__thread
```

缓存。

线程之间互不影响。

---

## Process Name

进程名称只读缓存。

初始化后不会修改。

线程安全。

---

## Time APIs

底层：

```c
clock_gettime()
```

天然线程安全。

---

# Dependency Relationship

OS 模块位于 Common 层最底部。

依赖：

```text
libpthread
libc
Linux /proc
```

---

## Used By

主要被以下模块使用：

* log
* trace
* scheduler
* worker
* benchmark

以及未来所有需要：

```text
时间
线程
进程
```

信息的模块。

---

# Design Principles

推荐：

```c
fs_get_tid()
fs_get_thread_name()
fs_get_time_ms()
fs_get_monotonic_us()
```

不推荐：

```c
pthread_self()
clock_gettime()
pthread_getname_np()
```

直接散落在业务代码中。

统一通过 OS 模块访问运行时环境信息。

这样能够保证：

* 接口统一
* 代码整洁
* 后续易于移植
* 便于扩展和维护
