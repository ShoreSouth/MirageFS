# Path Module

## Overview

Path 模块提供路径字符串处理能力。

主要职责：

* 路径拼接
* 路径规范化
* 路径拆分
* 路径属性判断
* 宿主 OS 路径辅助操作

该模块仅处理字符串路径。

不涉及：

* inode
* dentry
* lookup
* 权限检查
* MirageFS 元数据

属于 Common 基础工具模块。

---

## Design Goals

### String Only

Path 模块只处理字符串。

例如：

```text
"/home/user/file.txt"
```

模块不会访问：

```text
inode
directory
filesystem tree
```

因此：

```c
fs_path_normalize()
```

仅进行字符串转换。

不会验证路径是否真实存在。

---

### Independent Of MirageFS

Path 模块不依赖：

* inode
* dentry
* cache
* vfs

等任何文件系统模块。

因此可用于：

* 日志系统
* 配置系统
* 初始化代码
* 调试工具

---

### Safe Path Operations

统一处理：

* 路径拼接
* 路径截断
* 路径解析

避免重复编写字符串逻辑。

---

## Architecture

```text
log
config
trace
tool
init
  │
  ▼
 path
  │
  ▼
 libc
```

Path 位于 Common 层。

不依赖 MirageFS 其他模块。

---

# Path Join

## Basic Join

```c
fs_path_join()
```

功能：

```text
dst = a + "/" + b
```

示例：

```c
fs_path_join(
    buf,
    sizeof(buf),
    "/tmp",
    "test.log");
```

结果：

```text
/tmp/test.log
```

---

## Safe Join

```c
fs_path_join_safe()
```

避免产生重复：

```text
//
```

示例：

输入：

```text
a = "/tmp/"
b = "test.log"
```

结果：

```text
/tmp/test.log
```

而不是：

```text
/tmp//test.log
```

---

## Typical Usage

日志模块：

```c
fs_path_join_safe(
    path,
    sizeof(path),
    log_dir,
    file_name);
```

生成日志文件路径。

---

# Path Normalize

## API

```c
fs_path_normalize()
```

用于路径规范化。

---

## Supported Rules

移除：

```text
.
..
重复 /
```

示例：

输入：

```text
/a//b/./c/../d
```

输出：

```text
/a/b/d
```

---

## Example

输入：

```text
./data/../config
```

输出：

```text
config
```

---

## Notes

当前实现仅进行字符串级处理。

不会：

```text
访问磁盘
检查目录存在性
执行真实路径解析
```

---

# Path Split

## Dirname

```c
fs_path_dirname()
```

功能：

```text
"/a/b/c"
      │
      ▼
"/a/b"
```

---

### Example

输入：

```text
/var/log/fs.log
```

输出：

```text
/var/log
```

---

## Basename

```c
fs_path_basename()
```

功能：

```text
"/a/b/c"
      │
      ▼
"c"
```

---

### Example

输入：

```text
/var/log/fs.log
```

输出：

```text
fs.log
```

---

## Typical Usage

日志模块：

```c
const char *name =
    fs_path_basename(path);
```

获取文件名。

---

# Path Attributes

## Absolute Path

```c
fs_path_is_absolute()
```

判断：

```text
是否以 '/'
开头
```

示例：

```text
/etc/passwd
```

返回：

```text
true
```

---

### Relative Path

```text
config/log.conf
```

返回：

```text
false
```

---

## Empty Path

```c
fs_path_is_empty()
```

判断：

```text
NULL
空字符串
```

示例：

```text
""
```

返回：

```text
true
```

---

# OS Helpers

## Motivation

部分基础设施模块需要访问宿主 OS。

例如：

* 日志目录
* 输出目录
* 配置目录

因此 Path 模块提供少量 OS 辅助能力。

---

## Path Exists

```c
fs_path_exists()
```

功能：

```text
检查宿主 OS 路径是否存在
```

内部：

```c
access(path, F_OK)
```

实现。

---

### Important

该接口检查：

```text
Linux 文件系统
```

而不是：

```text
MirageFS
```

---

## Mkdir Recursive

```c
fs_path_mkdir_recursive()
```

功能类似：

```bash
mkdir -p
```

---

### Example

输入：

```text
output/log/miragefs
```

自动创建：

```text
output
output/log
output/log/miragefs
```

---

### Typical Usage

日志模块：

```c
fs_path_mkdir_recursive(
    dir,
    0777);
```

自动创建日志目录。

---

# Typical Usage

## Log Module

```c
char path[512];

fs_path_join_safe(
    path,
    sizeof(path),
    dir,
    file_name);
```

生成日志文件路径。

---

## Initialization

```c
fs_path_mkdir_recursive(
    "./output/log",
    0777);
```

创建输出目录。

---

## Config

```c
fs_path_normalize(
    buf,
    sizeof(buf),
    user_path);
```

规范化配置路径。

---

# Dependency Relationship

Path 模块依赖：

```text
string.h
stdio.h
unistd.h
sys/stat.h
errno.h
```

不依赖 MirageFS 其他模块。

---

## Used By

主要被以下模块使用：

* log
* config
* init
* tool
* test

以及未来所有涉及路径字符串处理的模块。

---

# Coding Guidelines

推荐：

```c
fs_path_join_safe()
fs_path_normalize()
fs_path_dirname()
fs_path_basename()
```

统一处理路径。

不推荐：

```c
snprintf(...)

strcat(...)

手工解析 '/'
```

散落在业务代码中。

---

# Important Boundary

Path 模块与 VFS 模块职责严格分离。

Path 模块负责：

```text
字符串路径
```

例如：

```text
"/home/a/b"
```

VFS 模块负责：

```text
路径解析
inode 查找
目录遍历
权限检查
```

例如：

```text
lookup("/home/a/b")
```

因此：

```c
fs_path_exists()
```

与未来的：

```c
vfs_lookup()
```

完全不是同一个概念。

前者检查宿主 OS。

后者检查 MirageFS。

两者不可混用。

---

# Future Extensions

未来可扩展：

* path compare
* path hash
* extension parser
* wildcard match
* UTF-8 path helper

保持：

* String Only
* No Filesystem Semantics
* No Metadata Access

设计原则不变。
