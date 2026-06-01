# Log Module

## Overview

Log 模块负责 MirageFS 全局日志输出。

该模块提供：

* 多级别日志
* 自动 Trace 关联
* 按进程分类
* 按线程分类
* TLS 日志文件管理
* 统一日志格式

业务代码无需关心：

* 日志文件路径
* 文件创建
* Trace 获取
* 时间格式化
* 线程隔离

仅需使用：

```c
FS_LOG_DUMP_DEBUG(...)
FS_LOG_DUMP_INFO(...)
FS_LOG_DUMP_WARN(...)
FS_LOG_DUMP_ERROR(...)
```

即可完成日志输出。

---

## Design Goals

### Unified Logging

整个系统使用统一日志格式。

避免：

```c
printf(...)
fprintf(...)
```

散落在各模块中。

---

### Trace Integration

日志自动关联当前 Trace。

业务代码无需显式传递：

```c
trace_id
span_id
```

日志系统自动从 TLS Trace Context 获取。

---

### Thread Isolation

每个线程独立日志文件。

避免：

```text
线程A日志
线程B日志
线程A日志
线程B日志
```

互相穿插导致难以排查问题。

---

### Zero Business Intrusion

业务代码仅负责记录业务信息：

```c
FS_LOG_DUMP_INFO(
    "create success");
```

日志框架负责补充：

* 时间
* Trace
* 文件
* 行号
* 函数名

---

## Architecture

```text
Business Module
       │
       ▼
FS_LOG_DUMP_XXX
       │
       ▼
fs_log_write()
       │
       ├── FS_TRACE_GET()
       │
       ├── fs_time_str()
       │
       ├── fs_get_process_name()
       │
       └── fs_get_thread_name()
                │
                ▼
           log file
```

---

## Log Levels

### DEBUG

调试日志。

用于：

* 流程跟踪
* 状态打印
* 参数检查

示例：

```c
FS_LOG_DUMP_DEBUG(
    "inode=%lu",
    inode_id);
```

---

### INFO

业务事件。

示例：

```c
FS_LOG_DUMP_INFO(
    "file create success");
```

---

### WARN

可恢复异常。

示例：

```c
FS_LOG_DUMP_WARN(
    "cache miss");
```

---

### ERROR

错误日志。

示例：

```c
FS_LOG_DUMP_ERROR(
    "open failed");
```

---

## Log Directory Layout

日志目录按照：

```text
base_dir/
    process/
        thread.log
```

组织。

例如：

```text
output/
└── log/
    └── miragefs/
        ├── main.log
        ├── worker_0.log
        ├── worker_1.log
        └── worker_2.log
```

其中：

```text
miragefs
```

来自：

```c
fs_get_process_name()
```

线程名称来自：

```c
fs_get_thread_name()
```

---

## TLS Design

### Per Thread File Handle

每个线程拥有独立 FILE。

```c
static __thread FILE *tls_fp;
```

首次写日志时：

```c
fs_log_get_fp()
```

自动创建对应日志文件。

后续直接复用。

---

### Lazy Open

日志文件采用懒加载。

线程启动时：

```text
不创建文件
```

首次写日志时：

```text
创建文件
打开文件
缓存FILE*
```

减少无效文件创建。

---

## Trace Integration

日志模块与 Trace 模块深度集成。

每次输出日志：

```c
fs_trace_ctx_t *ctx =
    FS_TRACE_GET();
```

自动获取：

```text
trace_id
span_id
```

无需业务代码传递。

---

### Example

业务代码：

```c
FS_LOG_DUMP_INFO(
    "create file success");
```

日志输出：

```text
[INFO]
[2026-05-01 12:30:15]
[trace=0x1001 span=0x2001]
[file.c:123 create_file]
create file success
```

---

## Log Format

当前格式：

```text
[level]
[time]
[trace]
[file:line func]
message
```

实际输出示例：

```text
[INFO][2026-05-01 12:30:15]
[trace=0x1001 span=0x2001]
[file.c:123 create_file]
create file success
```

单行形式：

```text
[INFO][2026-05-01 12:30:15][trace=0x1001 span=0x2001][file.c:123 create_file] create file success
```

---

## Initialization

系统启动时初始化：

```c
fs_log_init(
    "./output/log",
    FS_LOG_INFO);
```

参数：

| 参数       | 说明     |
| -------- | ------ |
| base_dir | 日志根目录  |
| level    | 最低输出级别 |

---

## Filtering

日志级别过滤：

```c
if (level < g_log_level)
    return;
```

例如：

```c
g_log_level = FS_LOG_WARN;
```

则：

```text
DEBUG 忽略
INFO  忽略
WARN  输出
ERROR 输出
```

---

## Thread Safety

日志模块无需全局锁。

原因：

每个线程写自己的文件：

```text
worker_0.log
worker_1.log
worker_2.log
```

不存在多个线程同时写同一个 FILE。

因此：

```text
无竞争
无锁
无阻塞
```

性能优于全局日志锁方案。

---

## Typical Usage

### Function Entry

```c
FS_LOG_DUMP_DEBUG(
    "enter create()");
```

---

### State Change

```c
FS_LOG_DUMP_INFO(
    "cache size=%u",
    cache->count);
```

---

### Error Path

```c
FS_LOG_DUMP_ERROR(
    "lookup failed");
```

---

## Relationship With Other Modules

### Dependency

Log 模块依赖：

* os
* path
* trace

用于：

* 获取线程名称
* 获取进程名称
* 创建目录
* 获取 Trace Context

---

### Used By

Log 模块被所有业务模块使用：

* mempool
* list
* hash
* cache
* objmeta
* inode
* dentry
* vfs
* cli

属于 MirageFS 核心基础设施模块。

---

## Future Extensions

当前实现：

```text
同步写文件
```

未来可扩展：

* Async Logger
* Log Rotation
* Daily Log
* Compression
* JSON Log
* Remote Collector

扩展不影响业务代码。

业务层始终只使用：

```c
FS_LOG_DUMP_XXX(...)
```

接口。
