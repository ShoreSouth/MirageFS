# ObjMeta 模块设计文档

## 1. 模块概述

ObjMeta（Object Metadata）用于维护 MirageFS 对象与 Linux 后端对象之间的映射关系。

核心职责：

```text
objectid
    ↓
ObjMeta
    ↓
(mount_id + file_handle)
    ↓
Linux Backend Object
```

当前阶段，ObjMeta 不承担完整元数据管理职责。

它仅作为：

```text
MirageFS Object
        ↔
Linux Backend Object
```

之间的定位桥梁。


---

## 2. 设计目标

### 2.1 稳定定位后端对象

MirageFS 内部所有对象均拥有唯一：

```text
objectid
```

而 Linux 后端对象通过：

```text
mount_id + file_handle
```

进行稳定定位。

ObjMeta 负责建立两者映射关系。


---

### 2.2 不依赖路径

路径是不稳定的。

例如：

```text
/a/test.txt

rename

/b/test.txt
```

路径发生变化，但文件对象未变化。

因此 ObjMeta 不保存：

```text
path
name
parent
```

等信息。


---

### 2.3 固定长度

ObjMeta 设计为固定长度结构：

```c
sizeof(ObjMeta_t) == 32
```

优势：

- Cache Friendly
- MemPool 管理方便
- HashTable 存储方便
- KV 持久化方便
- WAL 记录方便


---

### 2.4 面向后续扩展

当前仅保存：

```text
objectid
mount_id
file_handle
```

未来可扩展：

- backend type
- inode cache
- stat cache
- locator version

但不影响现有结构设计。


---

## 3. 模块职责

ObjMeta 负责：

- objectid 映射
- backend locator 保存
- backend locator 比较
- backend locator 校验
- debug 输出

ObjMeta 不负责：

- 路径管理
- Namespace 管理
- Dentry 管理
- Snapshot 管理
- 文件属性缓存
- 权限管理


---

## 4. Linux Backend Locator

当前阶段采用：

```text
mount_id + file_handle
```

作为后端对象定位方式。

即：

```text
mount_id
    +
file_handle
```

唯一定位 Linux 对象。


---

### 4.1 mount_id

Linux 挂载点唯一标识。

来源：

```c
name_to_handle_at()
```

返回信息。

作用：

```text
定位具体文件系统实例
```

例如：

```text
/dev/sda1
mount_id=20

/dev/sdb1
mount_id=30
```


---

### 4.2 file_handle

Linux VFS 提供的稳定对象句柄。

来源：

```c
name_to_handle_at()
```

获得。

特点：

- 与路径无关
- rename 不失效
- 支持 reopen
- 可持久化保存

后续可通过：

```c
open_by_handle_at()
```

重新打开对象。


---

## 5. 数据结构设计

### 5.1 结构定义

```c
typedef struct ObjMeta {

    uint64_t objectid;

    int32_t mount_id;

    uint16_t handle_type;
    uint16_t handle_bytes;

    uint8_t file_handle[16];

} ObjMeta_t;
```

逻辑结构：

```text
+-----------+
| objectid  |
+-----------+

+-----------+
| mount_id  |
+-----------+

+-----------+
| type      |
| length    |
+-----------+

+-----------+
| handle    |
+-----------+
```


---

### 5.2 内存布局

```text
Offset  Size    Field
------  ----    ----------------
0       8       objectid
8       4       mount_id
12      2       handle_type
14      2       handle_bytes
16      16      file_handle

Total = 32 Bytes
```


---

### 5.3 编译期检查

保证结构尺寸固定：

```c
_Static_assert(
    sizeof(ObjMeta_t) == 32,
    "ObjMeta_t size invalid");
```


---

## 6. Handle 设计

### 当前限制

```c
#define OBJMETA_MAX_HANDLE_SIZE 16
```

原因：

当前 MirageFS 为实验项目。

主流文件系统：

- ext4
- xfs

返回的 file_handle 通常远小于该值。


---

### 有效性判断

必须满足：

```text
handle_bytes > 0

AND

handle_bytes <= 16
```

否则视为非法句柄。

对应内部函数：

```c
objmeta_handle_valid()
```


---

## 7. 生命周期

### 创建

创建对象后：

```c
name_to_handle_at()
```

获取：

```text
mount_id
handle_type
file_handle
```

随后构造：

```c
objmeta_init()
```

建立映射。


---

### 查询

通过：

```text
objectid
```

查找：

```text
ObjMeta
```

获得后端定位信息。


---

### 重开对象

利用：

```c
open_by_handle_at()
```

通过：

```text
mount_id
file_handle
```

重新获得文件描述符。

实现：

```text
objectid
    ↓
ObjMeta
    ↓
open_by_handle_at()
    ↓
fd
```


---

### 删除

对象删除后：

```c
objmeta_reset()
```

清空记录。

对应映射失效。


---

## 8. 相等性判断

ObjMeta 相同需满足：

```text
objectid
mount_id
handle_type
handle_bytes
file_handle
```

全部一致。

对应接口：

```c
objmeta_equal()
```


---

## 9. 有效性判断

合法 ObjMeta 必须满足：

### objectid 有效

```text
objectid != 0
```

### handle 长度合法

```text
0 < handle_bytes <= 16
```

对应接口：

```c
objmeta_is_valid()
```


---

## 10. 与 FUID 的关系

FUID 表示对象身份。

```text
FUID
    ↓
(fsid, objectid, gen)
```

ObjMeta 表示对象定位信息。

```text
ObjMeta
    ↓
(mount_id, file_handle)
```

两者职责不同。

```text
           +------+
           | FUID |
           +------+
               |
               | objectid
               v

         +-----------+
         | ObjMeta   |
         +-----------+

               |
               v

      mount_id + file_handle
               |
               v

      Linux Backend Object
```


---

## 11. Debug 支持

提供：

```c
objmeta_dump()
```

输出内容：

```text
========== ObjMeta ==========
objectid     : 100
mount_id     : 23
handle_type  : 1
handle_bytes : 8
file_handle  : 0a01bcff...
=============================
```

用于：

- Trace
- Debug
- 故障定位
- 日志分析


---

## 12. 模块依赖关系

```text
                 +--------+
                 |  FUID  |
                 +--------+
                      |
                      v

                 +--------+
                 |ObjMeta |
                 +--------+
                      |
                      v

             Linux File Handle

                      |
                      v

             open_by_handle_at()
```

ObjMeta 是 MirageFS 元数据层中最底层的对象定位模块。

其职责仅是：

```text
objectid
      →
backend object locator
```

不参与路径解析和目录树管理。