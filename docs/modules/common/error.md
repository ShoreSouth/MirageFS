# Error Module

## Overview

Error 模块是 MirageFS 的统一错误码系统。

用于解决传统 Linux errno 的几个问题：

```text
errno 信息量不足

无法区分来源模块

无法区分严重程度

难以定位具体错误
```

因此 MirageFS 引入：

```text
32bit Structured Error Code
```

统一描述：

```text
错误级别

来源模块

子错误码

Linux errno
```

---

## Design Goals

### Unified Error System

所有模块统一返回：

```c
fs_error_t
```

而不是：

```c
int
```

避免：

```text
-1

-EINVAL

-ENOMEM

-EIO
```

混杂使用。

---

### Preserve Linux Compatibility

内部：

```c
fs_error_t
```

外部：

```c
errno
```

通过：

```c
fs_err_to_errno()
```

转换。

保证：

```text
内部拥有丰富语义

外部兼容 Linux
```

---

### Easy Debugging

错误码携带：

```text
Level

Module

Sub Error

Errno
```

方便：

```text
日志分析

故障定位

统计监控
```

---

## Error Code Layout

错误码长度：

```text
32 bit
```

布局：

```text
+---------+------------+------------+----------+
| Level   | Module ID  | Sub Error  | errno    |
+---------+------------+------------+----------+

31      30 29       20 19        8 7        0
```

---

## Bit Field Definition

### Level

占用：

```text
2 bit
```

范围：

```text
0 ~ 3
```

位置：

```c
31~30
```

---

### Module ID

占用：

```text
10 bit
```

范围：

```text
0 ~ 1023
```

位置：

```c
29~20
```

---

### Sub Error

占用：

```text
12 bit
```

范围：

```text
0 ~ 4095
```

位置：

```c
19~8
```

---

### Errno

占用：

```text
8 bit
```

范围：

```text
0 ~ 255
```

位置：

```c
7~0
```

保存：

```text
Linux errno
```

值。

---

# Error Level

## FS_ERR_LEVEL_OK

```c
FS_ERR_LEVEL_OK
```

表示：

```text
成功
```

对应：

```c
FS_OK
```

---

## FS_ERR_LEVEL_INFO

```c
FS_ERR_LEVEL_INFO
```

表示：

```text
提示信息

非错误状态
```

例如：

```text
对象已存在

无需执行
```

---

## FS_ERR_LEVEL_ERROR

```c
FS_ERR_LEVEL_ERROR
```

表示：

```text
可恢复错误
```

例如：

```text
参数错误

资源不足

对象不存在
```

---

## FS_ERR_LEVEL_FATAL

```c
FS_ERR_LEVEL_FATAL
```

表示：

```text
不可恢复错误
```

例如：

```text
元数据损坏

存储层异常

关键资源丢失
```

---

# Module ID

每个模块拥有唯一编号。

## Common

```c
FS_MODULE_COMMON
```

基础公共模块。

---

## VFS

```c
FS_MODULE_VFS
```

虚拟文件系统层。

---

## FS

```c
FS_MODULE_FS
```

MirageFS 核心逻辑层。

---

## LSA

```c
FS_MODULE_LSA
```

Local Storage Adapter。

---

## Cache

```c
FS_MODULE_CACHE
```

缓存管理模块。

---

## Meta

```c
FS_MODULE_META
```

元数据模块。

---

## Storage

```c
FS_MODULE_STORAGE
```

底层存储模块。

---

## Reserved

最大支持：

```text
1023 个模块
```

定义：

```c
FS_MODULE_MAX
```

---

# Error Construction

统一构造宏：

```c
FS_ERR(
    level,
    module,
    sub,
    errno
)
```

示例：

```c
FS_ERR(
    FS_ERR_LEVEL_ERROR,
    FS_MODULE_CACHE,
    10,
    ENOMEM
);
```

生成：

```text
Level   = ERROR

Module  = CACHE

Sub     = 10

Errno   = ENOMEM
```

---

# Common Helper Macros

## Success

```c
FS_OK
```

等价：

```c
0
```

---

## Common Error

```c
FS_ERR_COMMON(sub, err)
```

示例：

```c
return FS_ERR_COMMON(
    FS_ERR_SUB_INVALID_ARG,
    EINVAL
);
```

---

## Fatal Error

```c
FS_ERR_FATAL(
    module,
    sub,
    err
)
```

示例：

```c
return FS_ERR_FATAL(
    FS_MODULE_META,
    100,
    EIO
);
```

---

## From errno

```c
FS_ERR_FROM_ERRNO(
    module,
    errno
)
```

示例：

```c
return FS_ERR_FROM_ERRNO(
    FS_MODULE_STORAGE,
    ENOSPC
);
```

---

# Error Decode

## Get Level

```c
FS_ERR_GET_LEVEL(err)
```

返回：

```text
OK
INFO
ERROR
FATAL
```

---

## Get Module

```c
FS_ERR_GET_MODULE(err)
```

返回模块编号。

---

## Get Sub Error

```c
FS_ERR_GET_SUB(err)
```

返回：

```text
模块内部错误码
```

---

## Get errno

```c
FS_ERR_GET_ERRNO(err)
```

返回：

```text
Linux errno
```

值。

---

# Error Check Helpers

## Success Check

```c
FS_IS_OK(err)
```

示例：

```c
if (FS_IS_OK(err)) {
    ...
}
```

---

## Error Check

```c
FS_IS_ERROR(err)
```

判断：

```text
是否为可恢复错误
```

---

## Fatal Check

```c
FS_IS_FATAL(err)
```

判断：

```text
是否为不可恢复错误
```

---

## Retryable Check

```c
FS_IS_RETRYABLE(err)
```

当前实现：

```text
ERROR
```

级别均视为可重试。

未来可进一步细分。

---

## Module Check

```c
FS_IS_MODULE(err, module)
```

示例：

```c
if (FS_IS_MODULE(err,
                 FS_MODULE_CACHE))
{
    ...
}
```

---

# Linux errno Compatibility

内部：

```c
fs_error_t
```

外部：

```c
Linux errno
```

转换：

```c
int ret = fs_err_to_errno(err);
```

示例：

```c
FS_ENOENT
    ↓
ENOENT
    ↓
-ENOENT
```

适用于：

```text
CLI

POSIX Interface

FUSE Interface

Kernel Compatible API
```

---

# Common Error Definitions

Common 模块预定义：

```c
FS_ERR_SUB_UNKNOWN
```

未知错误。

---

```c
FS_ERR_SUB_INVALID_ARG
```

非法参数。

---

```c
FS_ERR_SUB_NO_MEMORY
```

内存不足。

---

```c
FS_ERR_SUB_NOT_FOUND
```

对象不存在。

---

```c
FS_ERR_SUB_EXIST
```

对象已存在。

---

```c
FS_ERR_SUB_PERMISSION
```

权限不足。

---

# Common Shortcuts

## FS_EINVAL

等价：

```c
FS_ERR_COMMON(
    FS_ERR_SUB_INVALID_ARG,
    EINVAL
)
```

---

## FS_ENOMEM

等价：

```c
FS_ERR_COMMON(
    FS_ERR_SUB_NO_MEMORY,
    ENOMEM
)
```

---

## FS_ENOENT

等价：

```c
FS_ERR_COMMON(
    FS_ERR_SUB_NOT_FOUND,
    ENOENT
)
```

---

## FS_EEXIST

等价：

```c
FS_ERR_COMMON(
    FS_ERR_SUB_EXIST,
    EEXIST
)
```

---

## FS_EPERM

等价：

```c
FS_ERR_COMMON(
    FS_ERR_SUB_PERMISSION,
    EPERM
)
```

---

# Debug Helpers

## Error Level String

```c
fs_err_level_str(err)
```

返回：

```text
OK

INFO

ERROR

FATAL
```

适用于：

```text
日志输出

错误分析
```

---

## Module Name String

```c
fs_module_str(module)
```

返回：

```text
COMMON

VFS

FS

LSA

CACHE

META

STORAGE
```

用于：

```text
日志

调试

监控平台
```

---

# Example

创建错误：

```c
fs_error_t err;

err = FS_ERR(
    FS_ERR_LEVEL_ERROR,
    FS_MODULE_CACHE,
    100,
    ENOMEM
);
```

解析：

```c
printf("level=%s\n",
       fs_err_level_str(err));

printf("module=%s\n",
       fs_module_str(
           FS_ERR_GET_MODULE(err)));

printf("errno=%u\n",
       FS_ERR_GET_ERRNO(err));
```

---

# Typical Workflow

```text
Module Error
      |
      v

FS_ERR()

      |
      v

Return fs_error_t

      |
      v

Caller Decode

      |
      v

fs_err_to_errno()

      |
      v

Linux Return Code
```

---

# Dependency Relationship

依赖：

```text
stdint.h

errno.h
```

关系：

```text
error
│
├── stdint
└── errno
```

属于 MirageFS Common 基础模块。

---

# Future Roadmap

计划扩展：

```text
Error Registry

Sub Error Dictionary

Stack Trace Integration

Distributed Error Code

Error Statistics

Error Monitoring
```

最终形成：

```text
MirageFS Unified Error Framework
```

统一管理整个文件系统错误处理体系。
