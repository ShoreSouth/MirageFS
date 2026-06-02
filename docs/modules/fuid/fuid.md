# FUID 模块设计文档

## 1. 模块概述

FUID（File Unique Identity）是 MirageFS 中所有对象的统一身份标识。

系统中的文件、目录、符号链接、设备节点等对象，均通过 FUID 唯一标识。

FUID 的职责类似于 Linux VFS 中的 inode number，但相比 inode，FUID 具有更强的全局唯一性和扩展能力：

- 文件系统范围内唯一
- 支持对象重建检测（generation）
- 支持 Snapshot 视图
- 支持 Qtree / Tenant 隔离
- 支持 Shard 部署
- 固定长度结构
- 支持序列化与持久化

MirageFS 上层模块均通过 FUID 引用对象，而不是直接依赖底层路径。


---

## 2. 设计目标

### 2.1 全局唯一

任意两个不同对象必须拥有不同 FUID。

```text
(fsid, objectid, gen)
```

共同构成对象唯一身份。


### 2.2 防止 Stale Handle

对象删除后重新创建，即使 objectid 被复用，也必须能够检测失效句柄。

例如：

```text
create fileA
objectid=100
gen=1

delete fileA

create fileB
objectid=100
gen=2
```

此时：

```text
(100,1) != (100,2)
```

旧 FUID 自动失效。


### 2.3 支持多视图

同一个对象可能存在于：

- 不同 Snapshot
- 不同 Qtree
- 不同 Shard

因此需要保存视图信息。


### 2.4 固定长度

FUID 统一设计为：

```c
sizeof(Fuid_t) == 64
```

优势：

- Cache Friendly
- 网络传输方便
- WAL 记录方便
- 后续版本扩展方便


---

## 3. FUID 架构

### 3.1 Identity

Identity 用于唯一标识对象。

```text
+--------+-----------+------+
| fsid   | objectid  | gen  |
+--------+-----------+------+
```

其中：

| 字段 | 说明 |
|--------|--------|
| fsid | 文件系统ID |
| objectid | 对象ID |
| gen | generation |

Identity 一旦确定，不会因为 Snapshot 等视图变化而改变。


---

### 3.2 View

View 描述对象当前访问视图。

```text
+---------+---------+---------+
| qtreeid | snapid  | shardid |
+---------+---------+---------+
```

用于：

- 多租户隔离
- Snapshot 访问
- 分片部署

View 不参与对象唯一性判断。


---

### 3.3 Attributes

对象属性。

```text
+--------+---------+--------+
| type   | version | flags  |
+--------+---------+--------+
```

包含：

- 文件类型
- FUID结构版本
- 对象特性标记


---

## 4. FUID 布局

### 4.1 内存布局

```text
Offset  Size    Field
------  ----    ----------------
0       8       fsid
8       8       objectid
16      4       gen

20      4       qtreeid
24      4       snapid
28      4       shardid

32      1       type
33      1       version
34      2       flags

36      12      reserved0
48      16      reserved1

Total = 64 Bytes
```

编译期检查：

```c
_Static_assert(sizeof(Fuid_t) == 64);
```


---

## 5. 文件类型设计

支持常见 POSIX 文件类型。

```c
typedef enum {
    FUID_TYPE_INVALID = 0,

    FUID_TYPE_FILE,
    FUID_TYPE_DIR,
    FUID_TYPE_SYMLINK,

    FUID_TYPE_FIFO,
    FUID_TYPE_SOCK,

    FUID_TYPE_BLK,
    FUID_TYPE_CHR,
} fuid_type_t;
```

对应关系：

| 类型 | 含义 |
|--------|--------|
| FILE | 普通文件 |
| DIR | 目录 |
| SYMLINK | 符号链接 |
| FIFO | 命名管道 |
| SOCK | Socket |
| BLK | 块设备 |
| CHR | 字符设备 |

MirageFS 后续新增对象类型时，可继续扩展。


---

## 6. Flags 设计

用于描述对象附加属性。

### 当前定义

```c
FUID_FLAG_COMPRESSED
FUID_FLAG_ENCRYPTED
FUID_FLAG_CLONED
```

含义：

| Flag | 说明 |
|--------|--------|
| COMPRESSED | 数据已压缩 |
| ENCRYPTED | 数据已加密 |
| CLONED | Clone对象 |

### 操作接口

```c
fuid_flag_test()
fuid_flag_set()
fuid_flag_clear()
```

统一封装位操作。


---

## 7. Identity 比较规则

FUID 比较仅比较 Identity 部分。

即：

```text
(fsid, objectid, gen)
```

相同即认为对象相同。

不比较：

```text
qtreeid
snapid
shardid
flags
version
```

原因：

这些字段属于视图信息或属性信息。

对应接口：

```c
fuid_equal()
```


---

## 8. Hash 设计

Hash 用于：

- HashTable
- Cache
- ObjectMap
- DirectoryEntry

输入：

```text
fsid
objectid
gen
```

输出：

```c
uint64_t
```

要求：

- 分布均匀
- 计算开销低
- 支持大规模对象


---

## 9. 生命周期

### 创建

对象创建时：

```c
fuid_build()
```

生成 FUID。

示例：

```text
fsid     = 1
objectid = 100
gen      = 1

type     = FILE
```

得到：

```text
(1,100,1)
```


### 删除

对象删除：

```text
generation++
```

旧 FUID 自动失效。


### 访问

上层模块通过：

```c
Fuid_t
```

定位对象。

不依赖路径。


---

## 10. Snapshot 支持

同一对象在多个 Snapshot 中可拥有不同视图。

例如：

```text
Identity:

(1,100,1)

Snapshot A:
snapid = 10

Snapshot B:
snapid = 20
```

Identity 不变。

View 不同。


---

## 11. Qtree 支持

用于实现：

- Tenant
- Namespace
- Volume

例如：

```text
TenantA
qtreeid = 1

TenantB
qtreeid = 2
```

同一 objectid 可在不同 Qtree 中拥有不同访问视图。


---

## 12. Shard 支持

用于未来横向扩展。

```text
Shard0
Shard1
Shard2
...
```

对象所属分片记录于：

```c
shardid
```

便于：

- 路由
- 负载均衡
- 分布式部署


---

## 13. Debug 输出

统一提供：

```c
fuid_to_str()
```

输出格式：

```text
fs=1,obj=100,gen=1,type=file
```

用途：

- 日志打印
- CLI显示
- 故障定位


---

## 14. 模块职责

FUID 模块仅负责：

- 对象身份定义
- 对象类型定义
- Identity 比较
- Hash 计算
- Flag 操作
- Debug 输出

不负责：

- 路径解析
- 元数据管理
- 对象查找
- Snapshot 管理
- Qtree 管理


---

## 15. 依赖关系

```text
                +---------+
                |  FUID   |
                +---------+
                     ^
                     |
    +----------------+----------------+
    |                |                |
    v                v                v

 ObjMeta         Dentry          VFS

    ^
    |
 Inode / Object
```

FUID 属于 MirageFS 最基础对象模型模块之一。

其上层所有元数据对象均通过 FUID 建立关联。