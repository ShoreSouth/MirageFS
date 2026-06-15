# LSA 设计文档

## 1. 模块简介

LSA（Linux System Access Layer）是 MirageFS 最底层的文件系统访问模块。

LSA 的职责是封装 Linux 文件系统相关系统调用，对上提供统一、稳定的文件系统访问接口，对下完成 Linux ABI 的适配与转换。

架构位置如下：

```text
VFS
 ↓
LSA
 ↓
Linux Kernel
```

LSA 是 MirageFS 唯一允许直接访问 Linux 文件系统接口的模块。

除 LSA 外，其它模块不应直接调用：

```text
openat()
fstatat()
renameat2()
getdents64()
getxattr()
name_to_handle_at()
...
```

等 Linux 特定接口。

---

## 2. 设计目标

### 2.1 抽象优先

LSA 的首要目标是：

```text
MirageFS Abstraction First
```

上层模块不应该感知 Linux ABI。

例如：

```text
FS_FLAG             替代 O_xxx

FS_TYPE             替代 S_IFxxx

lsa_device_t        替代 makedev()

lsa_file_handle_t   替代 struct file_handle
```

这样即使未来底层实现发生变化，上层代码仍然保持稳定。

---

### 2.2 文件系统语义优先

LSA 对外暴露文件系统语义，而不是系统调用语义。

例如：

```text
lookup()

create()

mkdir()

getattr()

readdir()
```

而不是：

```text
openat()

fstatat()

getdents64()
```

这样可以避免 Linux 细节向上传播。

---

### 2.3 轻量化设计

LSA 仅负责：

```text
参数检查

系统调用封装

错误码转换

平台适配
```

LSA 不负责：

```text
对象管理

元数据管理

缓存管理

引用计数

快照管理

权限策略
```

这些职责属于：

```text
VFS

ObjMgr

ObjTable

ObjMeta
```

等上层模块。

---

## 3. 模块边界

### LSA负责

```text
目录项操作

文件操作

属性操作

目录遍历

扩展属性

文件句柄

文件系统信息
```

---

### LSA不负责

```text
ObjectId管理

FUID管理

ObjMeta管理

缓存管理

对象生命周期管理

业务逻辑
```

---

## 4. 核心抽象

### 4.1 文件类型

MirageFS 使用统一文件类型定义：

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

LSA 负责与 Linux 类型进行转换。

例如：

```text
DT_DIR

S_IFDIR

DT_REG

S_IFREG
```

等 Linux 类型不会暴露给上层。

---

### 4.2 操作标志

MirageFS 定义统一 Flag：

```c
FS_FLAG_REPLACE

FS_FLAG_EXCLUSIVE

FS_FLAG_NOFOLLOW

FS_FLAG_SYNC

FS_FLAG_DIRECT
```

LSA 根据不同接口映射到底层实现。

例如：

```text
FS_FLAG_EXCLUSIVE

    -> O_EXCL

    -> XATTR_CREATE
```

---

### 4.3 设备节点

Linux 的设备号封装为：

```c
typedef struct lsa_device {

    uint32_t major_id;

    uint32_t minor_id;

} lsa_device_t;
```

避免上层直接依赖：

```c
makedev()
major()
minor()
```

等 Linux 宏。

---

### 4.4 文件句柄

Linux 文件句柄统一封装为：

```c
typedef struct lsa_file_handle {

    uint32_t handle_bytes;

    int32_t handle_type;

    uint8_t data[LSA_HANDLE_MAX_SIZE];

} lsa_file_handle_t;
```

上层禁止直接使用：

```c
struct file_handle
```

---

## 5. Namespace 操作

目录项相关操作：

```text
lookup

create

mkdir

mknod

unlink

rmdir

rename

link

symlink
```

主要接口：

```text
lsa_lookup()

lsa_create()

lsa_mkdir()

lsa_mknod()

lsa_unlink()

lsa_rmdir()

lsa_rename()

lsa_link()

lsa_symlink()
```

---

## 6. File 操作

文件描述符相关操作：

```text
open

close

read

write

pread

pwrite

lseek

fsync

truncate
```

主要接口：

```text
lsa_open()

lsa_close()

lsa_read()

lsa_write()

lsa_pread()

lsa_pwrite()

lsa_fsync()

lsa_ftruncate()
```

---

## 7. Attribute 操作

文件属性相关操作：

```text
getattr

setattr

access
```

主要接口：

```text
lsa_fstat()

lsa_fstatat()

lsa_fchmod()

lsa_fchown()

lsa_access()
```

---

## 8. XAttr 操作

扩展属性相关操作：

```text
getxattr

setxattr

listxattr

removexattr
```

主要接口：

```text
lsa_getxattr()

lsa_setxattr()

lsa_listxattr()

lsa_removexattr()
```

---

## 9. Handle 操作

文件句柄相关操作：

```text
name_to_handle_at

open_by_handle_at
```

主要接口：

```text
lsa_name_to_handle_at()

lsa_open_by_handle_at()
```

该能力是未来 ObjMeta 模块的重要基础。

访问链路：

```text
ObjectId
    ↓
ObjMeta
    ↓
lsa_file_handle_t
    ↓
open_by_handle_at()
```

---

## 10. Filesystem 操作

文件系统级操作：

```text
statfs

syncfs
```

主要接口：

```text
lsa_statfs()

lsa_syncfs()
```

---

## 11. Directory Iterator 设计

### 11.1 设计目标

LSA 不直接暴露：

```text
DIR *

readdir()

telldir()

seekdir()
```

而是提供统一目录迭代器。

核心思想：

```text
目录
    ↓
Iterator
    ↓
DirEntry
```

---

### 11.2 Cookie

目录位置抽象为：

```c
typedef struct lsa_dir_cookie {

    uint64_t value;

} lsa_dir_cookie_t;
```

Cookie 用于记录目录扫描位置。

支持：

```text
断点续扫

分页遍历

远程目录访问

NFS风格遍历
```

---

### 11.3 Iterator

目录扫描状态由 Iterator 保存：

```text
目录fd

当前cookie

扫描状态

getdents缓冲区

当前偏移
```

Iterator 对上层保持透明。

---

### 11.4 getdents64 实现

当前实现基于：

```text
getdents64()
```

而不是：

```text
readdir()
```

原因：

```text
可直接获取cookie

性能更高

控制力更强

便于未来扩展
```

未来即使切换为：

```text
io_uring

远程元数据服务

分布式目录服务
```

上层 API 仍然无需修改。

---

## 12. 错误处理

LSA 全面接入 MirageFS Error。

返回值类型：

```c
typedef fs_error_t lsa_ret_t;
```

错误格式：

```text
31~30 severity

29~20 module

19~8 sub

7~0 errno
```

LSA 不直接向上返回原始 errno。

示例：

```text
FS_ERR(
    ERROR,
    LSA,
    FS_OP_LOOKUP,
    ENOENT)
```

错误语义：

```text
WHO
    module

WHERE
    operation

WHAT
    errno
```

---

## 13. 未来规划

计划支持：

```text
io_uring 后端

目录批量遍历优化

持久化目录 Cookie

远程文件系统访问

快照感知 Handle

分布式元数据访问
```

LSA 对外接口保持稳定，内部实现可持续演进。

---

## 14. 总结

LSA 是 MirageFS 的 Linux 适配层。

其核心职责是：

```text
隔离 Linux ABI

提供统一文件系统抽象

向上暴露稳定接口

向下封装系统调用
```

LSA 只负责访问文件系统，不负责管理文件系统。

对象管理、缓存管理、元数据管理等职责由上层模块承担。
