# Assert Module

## Overview

Assert 模块提供 MirageFS 统一断言机制。

主要职责：

* 运行时条件检查
* 开发期错误发现
* 契约（Contract）验证
* 参数合法性检查
* 异常路径保护

该模块基于：

```text
Log Module
+
Abort
+
Compiler Configuration
```

构建。

---

## Design Goals

### Fail Fast

开发阶段发现非法状态时：

```text
立即记录日志
立即停止执行
```

避免错误继续传播。

例如：

```c
FS_ASSERT(obj != NULL);
```

发现异常后立即终止。

---

### Unified Assertion Framework

统一 MirageFS 中的：

* 参数检查
* 状态检查
* 内部一致性检查

避免：

```c
if (!ptr)
    abort();

if (!obj)
    exit(1);
```

等风格混乱的实现。

---

### Log First

所有断言失败都会先记录日志：

```text
ASSERT FAIL
```

随后再执行：

```text
abort
return
goto
```

等行为。

保证问题可追踪。

---

## Architecture

```text
application
     │
     ▼
assert
     │
     ▼
log
     │
     ▼
os
```

Assert 依赖 Log 模块。

所有断言信息统一进入日志系统。

---

# Configuration

## FS_ENABLE_ASSERT

控制断言是否启用：

```c
FS_ENABLE_ASSERT
```

---

### Debug Build

默认：

```text
Enabled
```

即：

```c
#ifndef NDEBUG
#define FS_ENABLE_ASSERT 1
#endif
```

---

### Release Build

默认：

```text
Disabled
```

即：

```c
#ifdef NDEBUG
#define FS_ENABLE_ASSERT 0
#endif
```

---

## Design Philosophy

Debug 环境：

```text
发现错误
立即终止
```

Release 环境：

```text
记录日志
继续运行
```

避免生产环境因断言直接退出。

---

# Basic Assertion

## FS_ASSERT

最基础断言：

```c
FS_ASSERT(cond)
```

示例：

```c
FS_ASSERT(ptr != NULL);
```

---

### Debug Mode

执行：

```text
记录错误日志
abort()
```

---

### Release Mode

执行：

```text
记录错误日志
继续运行
```

---

## Typical Usage

内部状态检查：

```c
FS_ASSERT(cache != NULL);

FS_ASSERT(node->refcnt > 0);

FS_ASSERT(obj->magic == OBJ_MAGIC);
```

---

# Assertion With Message

## FS_ASSERT_MSG

带附加信息：

```c
FS_ASSERT_MSG(
    cond,
    fmt,
    ...);
```

---

### Example

```c
FS_ASSERT_MSG(
    size <= MP_MAX_BLOCK_SIZE,
    "size=%lu",
    size);
```

日志：

```text
ASSERT FAIL: (size <= MP_MAX_BLOCK_SIZE)
size=8388608
```

---

## Typical Usage

复杂条件失败时提供更多上下文。

推荐：

```c
FS_ASSERT_MSG(
    inode != NULL,
    "fuid=%s",
    fuid_str);
```

---

# Return Assertions

## Motivation

很多情况下：

```text
失败后不需要 abort
```

而是直接返回。

因此提供返回型断言。

---

## FS_ASSERT_RET

定义：

```c
FS_ASSERT_RET(cond, ret)
```

---

### Example

```c
FS_ASSERT_RET(
    ptr != NULL,
    -EINVAL);
```

等价：

```c
if (!ptr) {
    log();
    return -EINVAL;
}
```

---

## Typical Usage

参数检查：

```c
int cache_insert(...)
{
    FS_ASSERT_RET(obj != NULL, -EINVAL);

    ...
}
```

---

## FS_ASSERT_RET_VOID

用于：

```c
void
```

函数。

示例：

```c
FS_ASSERT_RET_VOID(node != NULL);
```

---

# Goto Assertions

## FS_ASSERT_GOTO

定义：

```c
FS_ASSERT_GOTO(cond, label)
```

失败时：

```text
记录日志
goto label
```

---

### Example

```c
FS_ASSERT_GOTO(buf != NULL, out);

FS_ASSERT_GOTO(cache != NULL, out);

...
out:
    cleanup();
```

---

## Typical Usage

资源清理路径。

例如：

```text
malloc
lock
open
create
```

等流程。

---

# Optimized Assertions

## FS_ASSERT_UNLIKELY

定义：

```c
FS_ASSERT_UNLIKELY(cond)
```

内部：

```c
FS_UNLIKELY()
```

实现。

---

## Purpose

告诉编译器：

```text
该条件失败概率极低
```

帮助优化分支预测。

---

### Example

```c
FS_ASSERT_UNLIKELY(ptr != NULL);
```

适用于：

```text
内存损坏
对象损坏
逻辑不一致
```

等极少发生场景。

---

# Logging Behavior

所有断言统一调用：

```c
FS_LOG_DUMP_ERROR(...)
```

因此日志中包含：

```text
时间
线程
trace id
span id
文件名
行号
函数名
```

便于定位问题。

---

## Example

日志示例：

```text
[ERROR]
ASSERT FAIL:
(cache != NULL)
```

同时包含调用位置。

---

# Typical Usage

## Parameter Validation

```c
FS_ASSERT_RET(
    buf != NULL,
    -EINVAL);
```

---

## Internal State Check

```c
FS_ASSERT(
    obj->magic == OBJ_MAGIC);
```

---

## Resource Cleanup

```c
FS_ASSERT_GOTO(
    fp != NULL,
    out);
```

---

## Critical Invariant

```c
FS_ASSERT_UNLIKELY(
    refcnt > 0);
```

---

# Dependency Relationship

依赖：

```text
fs_log.h
stdlib.h
assert.h
```

其中：

```text
log
```

为核心依赖。

---

# Coding Guidelines

推荐：

```c
FS_ASSERT()
```

用于：

```text
理论上绝不应该失败
```

的条件。

---

推荐：

```c
FS_ASSERT_RET()
```

用于：

```text
参数校验
输入检查
```

场景。

---

推荐：

```c
FS_ASSERT_GOTO()
```

用于：

```text
统一清理路径
```

场景。

---

不推荐：

```c
FS_ASSERT(user_input != NULL);
```

处理外部输入。

因为外部输入属于正常错误路径。

应使用：

```c
FS_ASSERT_RET(
    user_input != NULL,
    -EINVAL);
```

---

# Important Boundary

Assert 用于：

```text
开发期错误
内部状态错误
逻辑不一致
```

不用于：

```text
业务错误
用户输入错误
正常异常流程
```

例如：

```c
FS_ASSERT(node != NULL);
```

合理。

---

```c
FS_ASSERT(file_not_exist == false);
```

不合理。

文件不存在属于正常业务情况。

应返回：

```c
-ENOENT
```

而不是触发断言。

---

# Future Extensions

未来可扩展：

```text
FS_BUG()

FS_BUG_ON()

FS_WARN_ON()

FS_ONCE_ASSERT()

FS_RATE_LIMIT_ASSERT()
```

进一步增强 MirageFS 调试能力。

保持：

```text
Fail Fast
Log First
Debug Friendly
```

设计原则不变。
