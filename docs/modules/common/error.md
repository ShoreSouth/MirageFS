# Error Subsystem Design

## 1. Overview

MirageFS 使用统一的 32bit 错误码体系。

设计目标：

* 全工程统一返回值类型
* 支持模块级错误定位
* 支持 Linux errno 兼容
* 支持日志与 Trace 系统集成
* 支持后续分布式扩展
* 保持零动态内存开销
* 保持 O(1) 错误解析

所有模块返回：

```c
typedef fs_error_t xxx_ret_t;
```

例如：

```c
typedef fs_error_t lsa_ret_t;
typedef fs_error_t vfs_ret_t;
typedef fs_error_t objmgr_ret_t;
```

统一使用：

```c
fs_error_t
```

作为返回值类型。

---

## 2. Error Layout

错误码固定为 32bit：

```text
 31 30 | 29 -------- 20 | 19 -------- 8 | 7 -------- 0
-------------------------------------------------------
severity|   module id   |   sub error   |    errno
```

字段定义：

| Field     | Bits | Description |
| --------- | ---- | ----------- |
| severity  | 2    | 错误严重等级      |
| module    | 10   | 模块 ID       |
| sub error | 12   | 模块内部错误      |
| errno     | 8    | Linux errno |

总计：

```text
2 + 10 + 12 + 8 = 32 bit
```

---

## 3. Severity

定义：

```c
typedef enum fs_err_severity {

    FS_SEV_INFO = 0,

    FS_SEV_WARN,

    FS_SEV_ERROR,

    FS_SEV_FATAL,

} fs_err_severity_t;
```

说明：

| Severity | Description |
| -------- | ----------- |
| INFO     | 信息          |
| WARN     | 警告          |
| ERROR    | 普通错误        |
| FATAL    | 严重错误        |

成功返回统一使用：

```c
FS_OK
```

而不是使用 Severity 表示成功。

---

## 4. Module ID

每个 MirageFS 模块拥有全局唯一 Module ID。

例如：

```text
COMMON
OBJMETA
OBJTABLE
OBJMGR
FSMGR
VFS
LSA
CACHE
SERVER
CLI
```

Module 定义位于：

```text
common/module/
```

目录。

### 文件结构

```text
common/module/

    fs_module.h
    fs_module.c
    fs_module_table.h
```

使用 X-Macro 自动生成：

```c
FS_MODULE_COMMON
FS_MODULE_OBJMETA
FS_MODULE_OBJTABLE
...
```

以及：

```c
fs_module_name()

fs_module_valid()
```

---

## 5. Linux errno

MirageFS 完全复用 Linux errno 数值。

例如：

```c
FS_ERRNO_ENOENT = ENOENT
FS_ERRNO_EEXIST = EEXIST
FS_ERRNO_ENOMEM = ENOMEM
```

不重新定义 errno 编号。

保证：

```c
FS_ERRNO_ENOENT == ENOENT
```

成立。

### Alias errno

Linux 中部分 errno 为别名：

```c
EWOULDBLOCK == EAGAIN

EDEADLOCK == EDEADLK
```

MirageFS 仅保留主名称：

```c
EAGAIN
EDEADLK
```

避免 switch duplicate case 问题。

---

## 6. Sub Error

Sub Error 用于描述模块内部具体错误。

例如：

```c
typedef enum objtable_sub_error {

    OBJTABLE_ERR_LOOKUP = 1,

    OBJTABLE_ERR_INSERT,

    OBJTABLE_ERR_REMOVE,

} objtable_sub_error_t;
```

原则：

* 每个模块独立维护
* 从 1 开始编号
* 0 保留

统一定义：

```c
#define FS_SUB_NONE 0U
```

用于：

```c
FS_ERR(..., FS_SUB_NONE, ...)
```

---

## 7. Error Construction

统一构造宏：

```c
FS_ERR(
    severity,
    module,
    sub,
    errno
)
```

示例：

```c
return FS_ERR(
            FS_SEV_ERROR,
            FS_MODULE_OBJTABLE,
            OBJTABLE_ERR_LOOKUP,
            FS_ERRNO_ENOENT);
```

---

## 8. Error Parsing

支持快速解析：

```c
fs_err_severity()

fs_err_module()

fs_err_sub()

fs_err_errno()
```

示例：

```c
fs_module_t module;

module = fs_err_module(ret);
```

---

## 9. Standard Values

成功返回：

```c
FS_OK
```

定义：

```c
#define FS_OK ((fs_error_t)0)
```

保留值：

```c
#define FS_SUB_NONE 0U
```

---

## 10. Logging

日志系统应尽量输出完整错误信息。

推荐格式：

```text
severity=ERROR
module=OBJTABLE
sub=LOOKUP
errno=ENOENT
```

而不是仅输出：

```text
ENOENT
```

因为：

```text
ENOENT
```

无法定位具体业务场景。

---

## 11. Module Error Definition

业务模块不应直接返回 Linux errno。

不推荐：

```c
return FS_ERR(
            FS_SEV_ERROR,
            FS_MODULE_OBJTABLE,
            FS_SUB_NONE,
            FS_ERRNO_ENOENT);
```

推荐：

```c
return FS_ERR(
            FS_SEV_ERROR,
            FS_MODULE_OBJTABLE,
            OBJTABLE_ERR_LOOKUP,
            FS_ERRNO_ENOENT);
```

这样可以同时获得：

```text
模块信息
业务错误
系统错误
```

三层上下文。

---

## 12. Directory Layout

```text
common/

├── module/
│   ├── fs_module.h
│   ├── fs_module.c
│   └── fs_module_table.h
│
├── error/
│   ├── fs_error.h
│   ├── fs_error.c
│   ├── fs_errno.h
│   ├── fs_errno.c
│   └── fs_errno_table.h
```

职责划分：

```text
module
    ↓
error

module
    ↓
log

module
    ↓
trace
```

其中：

```text
module
```

作为 MirageFS 全局模块注册中心。

```text
error
```

作为统一错误码封装层。

两者相互独立。
