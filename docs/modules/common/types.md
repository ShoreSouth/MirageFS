# Types Module

## Overview

Types 模块定义 MirageFS 全局统一类型系统。

主要职责：

* 定义基础语义类型
* 定义文件类型抽象
* 定义权限模型抽象
* 提供类型转换辅助函数

该模块是 MirageFS 所有模块共享的基础数据模型之一。

---

## Design Goals

### Semantic First

避免业务代码大量出现：

```c
uint64_t
uint32_t
```

这种缺乏语义的信息。

例如：

```c
uint64_t id;
```

无法判断：

```text
inode ?
object ?
snapshot ?
filesystem ?
```

而：

```c
ObjectId_t object_id;
```

语义明确。

---

### Type Consistency

整个系统统一使用：

```c
Fsid_t
ObjectId_t
GenId_t
QtreeId_t
SnapId_t
ShardId_t
```

避免同一种对象在不同模块出现不同定义。

---

### Linux Compatibility

文件类型与权限模型兼容 Linux。

便于：

* VFS 实现
* POSIX 兼容
* 底层 LSA 映射

---

### Future Extensibility

未来若需要：

```text
128-bit Object ID
Distributed FS ID
Snapshot Namespace
```

仅需修改 Types 模块。

业务代码无需调整。

---

# Semantic Types

## Filesystem Identifier

```c
typedef uint64_t Fsid_t;
```

表示文件系统实例 ID。

用于区分：

```text
FS #1
FS #2
FS #3
```

等多个独立文件系统。

---

## Object Identifier

```c
typedef uint64_t ObjectId_t;
```

表示对象唯一标识。

典型对象：

```text
inode
dentry
file
directory
symlink
```

ObjectId 在同一文件系统内唯一。

---

## Generation Identifier

```c
typedef uint32_t GenId_t;
```

用于对象版本控制。

典型场景：

```text
inode recycle
object reuse
stale handle detection
```

例如：

```text
ObjectId = 100

Gen = 1
Gen = 2
Gen = 3
```

即使 ObjectId 相同，也可区分不同生命周期对象。

---

## Qtree Identifier

```c
typedef uint32_t QtreeId_t;
```

表示 Quota Tree ID。

未来用于：

```text
tenant
namespace
quota domain
```

隔离。

---

## Snapshot Identifier

```c
typedef uint32_t SnapId_t;
```

表示快照标识。

用于：

```text
snapshot
clone
rollback
```

功能。

---

## Shard Identifier

```c
typedef uint32_t ShardId_t;
```

表示数据分片标识。

未来用于：

```text
distributed storage
hash shard
metadata shard
```

等场景。

---

# File Type System

## Motivation

Linux 中：

```c
mode_t
```

同时保存：

```text
文件类型
权限位
特殊权限
```

语义混杂。

MirageFS 提供独立：

```c
fs_type_t
```

用于表达对象类型。

---

## File Types

```c
typedef enum {
    FS_TYPE_UNKNOWN = 0,

    FS_TYPE_REG,
    FS_TYPE_DIR,
    FS_TYPE_LNK,
    FS_TYPE_FIFO,
    FS_TYPE_SOCK,
    FS_TYPE_BLK,
    FS_TYPE_CHR,

} fs_type_t;
```

---

## Type Mapping

| Type            | Description      |
| --------------- | ---------------- |
| FS_TYPE_REG     | Regular File     |
| FS_TYPE_DIR     | Directory        |
| FS_TYPE_LNK     | Symbolic Link    |
| FS_TYPE_FIFO    | FIFO             |
| FS_TYPE_SOCK    | Socket           |
| FS_TYPE_BLK     | Block Device     |
| FS_TYPE_CHR     | Character Device |
| FS_TYPE_UNKNOWN | Unknown          |

---

## Usage

获取文件类型：

```c
fs_type_t type =
    fs_type_from_mode(mode);
```

打印：

```c
FS_LOG_DUMP_INFO(
    "type=%s",
    fs_type_to_str(type));
```

---

# Permission Model

## fs_mode_t

```c
typedef mode_t fs_mode_t;
```

当前直接兼容 Linux：

```c
mode_t
```

未来可扩展为：

```c
uint32_t
```

或独立权限模型。

---

## Structure

Linux Mode 由：

```text
File Type
Permission
Special Bits
```

组成。

例如：

```text
0755
0644
040755
0100644
```

---

# Type Helpers

## File Type Check

封装 Linux：

```c
S_ISREG()
S_ISDIR()
...
```

统一使用：

```c
FS_IS_REG(mode)
FS_IS_DIR(mode)
FS_IS_LNK(mode)
```

示例：

```c
if (FS_IS_DIR(mode)) {

}
```

---

## Extract File Type

获取类型位：

```c
FS_MODE_TYPE(mode)
```

等价：

```c
mode & S_IFMT
```

---

## Extract Permissions

获取权限部分：

```c
FS_PERM(mode)
```

示例：

```c
0755
0644
0700
```

---

# Permission Constants

## User Permissions

```c
FS_IRUSR
FS_IWUSR
FS_IXUSR
```

对应：

```text
r
w
x
```

---

## Group Permissions

```c
FS_IRGRP
FS_IWGRP
FS_IXGRP
```

---

## Other Permissions

```c
FS_IROTH
FS_IWOTH
FS_IXOTH
```

---

## Special Permissions

```c
FS_ISUID
FS_ISGID
FS_ISVTX
```

对应：

```text
setuid
setgid
sticky bit
```

---

# Default Modes

## Regular File

```c
FS_MODE_FILE_DEFAULT
```

等价：

```c
S_IFREG | 0644
```

即：

```text
-rw-r--r--
```

---

## Directory

```c
FS_MODE_DIR_DEFAULT
```

等价：

```c
S_IFDIR | 0755
```

即：

```text
drwxr-xr-x
```

---

# Conversion Helpers

## Mode → Type

```c
fs_type_t type =
    fs_type_from_mode(mode);
```

示例：

```c
mode = S_IFDIR | 0755
```

结果：

```c
FS_TYPE_DIR
```

---

## Type → String

```c
fs_type_to_str(type);
```

结果：

```text
REG
DIR
LNK
```

适用于：

* 日志
* Debug
* CLI 输出

---

# Typical Usage

## Inode Creation

```c
inode->type =
    fs_type_from_mode(mode);
```

---

## Permission Check

```c
if (FS_PERM(mode) & FS_IWUSR) {

}
```

---

## Logging

```c
FS_LOG_DUMP_INFO(
    "type=%s",
    fs_type_to_str(type));
```

---

# Dependency Relationship

Types 模块属于 Common 最底层模块。

依赖：

```text
stdint.h
sys/types.h
sys/stat.h
```

不依赖 MirageFS 其他模块。

---

## Used By

几乎所有核心模块：

* fuid
* objmeta
* inode
* dentry
* cache
* vfs
* lsa

均依赖 Types 模块。

属于 MirageFS 基础类型定义中心。

---

# Coding Rules

推荐：

```c
ObjectId_t object_id;
Fsid_t fsid;
GenId_t gen;
fs_type_t type;
fs_mode_t mode;
```

不推荐：

```c
uint64_t id;
uint32_t gen;
int type;
mode_t mode;
```

优先使用语义类型，而非裸基础类型。

这样能够提高代码可读性，并减少不同 ID 混用风险。
