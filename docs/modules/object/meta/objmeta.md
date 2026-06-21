# ObjMeta 模块设计文档

## 1. 模块概述

ObjMeta（Object Metadata）用于维护 MirageFS 对象与 Linux 后端对象之间的映射关系。

核心职责：

```text
(objectid, gen)
    ↓
ObjMeta
    ↓
(mount_id + file_handle)
    ↓
Linux Backend Object
```

当前阶段，ObjMeta 承担两项职责：

1. **定位桥梁**：MirageFS Object ↔ Linux Backend Object
2. **生命周期支撑**：提供 refcnt + state 字段，供 objmgr 管理对象生命周期

```text
MirageFS Object
    ↕
ObjMeta  (key, refcnt, state, handle)
    ↕
Linux Backend Object
```


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
sizeof(obj_meta_t) == 48
```

字段构成：

```text
obj_key_t        : 12 bytes  (objectid + gen)
fs_atomic32_t   :  4 bytes  (refcnt)
uint32_t        :  4 bytes  (state)
obj_handle_t    : 24 bytes  (mount_id + type + len + data[16])
padding         :  4 bytes
─────────────────────────
Total           : 48 bytes
```

优势：

- Cache Friendly
- MemPool 管理方便
- HashTable 存储方便
- KV 持久化方便
- WAL 记录方便


---

### 2.4 面向后续扩展

当前保存：

```text
objectid
gen
refcnt
state
mount_id
file_handle
```

refcnt 和 state 为 objmgr 生命周期管理准备，objmeta 层仅提供存储，不对语义做假设。

未来可扩展：

- backend type
- inode cache
- stat cache
- locator version

但不影响现有结构设计。


---

## 3. 模块职责

ObjMeta 负责：

- objectid + gen 映射
- backend locator 保存、比较、校验
- 生命周期字段存储（refcnt + state），供 objmgr 使用
- debug 输出

ObjMeta 不负责：

- refcnt 的原子操作语义（由 objmgr 负责）
- state 的状态迁移（由 objmgr 负责）
- 路径管理
- Namespace 管理
- Dentry 管理
- Snapshot 管理
- 文件属性缓存
- 权限管理


---

## 4. obj_handle_t — Linux Backend Handle

### 设计动机

原先 mount_id、handle_type、handle_bytes、file_handle 作为独立字段
散落在 obj_meta_t 中。引入 obj_handle_t 将它们封装为一个语义整体：

- 隐藏 Linux VFS `name_to_handle_at` / `open_by_handle_at` 的原始细节
- 使 obj_meta_t 结构更清晰，上层 objmgr 操作更直观
- 便于后续扩展不同的 backend locator 类型

### 结构定义

```c
typedef struct obj_handle {
    int32_t  mount_id;                        /* Linux mount ID */
    uint16_t type;                            /* file_handle 类型 */
    uint16_t len;                             /* data[] 实际长度 */
    uint8_t  data[OBJMETA_MAX_HANDLE_SIZE];   /* file_handle 原始数据 */
} obj_handle_t;
```

### 定位方式

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

### 5.1 obj_state_t — 生命周期状态

```c
typedef enum obj_state {
    OBJ_STATE_INIT     = 0,  /* 刚创建，未激活     */
    OBJ_STATE_ACTIVE,        /* 已激活，可正常使用  */
    OBJ_STATE_DELETING,      /* 删除中，阻止新引用  */
    OBJ_STATE_DELETED        /* 已销毁，等待回收    */
} obj_state_t;
```

状态迁移图：

```text
INIT ──→ ACTIVE ──→ DELETING ──→ DELETED
  │                    │
  └────────────────────┘
       (异常路径：直接销毁未激活对象)
```

状态的读写由 objmgr 负责，objmeta 层仅提供存储字段。

### 5.2 obj_meta_t 结构定义

```c
typedef struct obj_meta {

    obj_key_t       key;    /* MirageFS 对象唯一标识 */
    fs_atomic32_t  refcnt; /* 引用计数，原子操作    */
    uint32_t       state;  /* 生命周期状态          */
    obj_handle_t   handle; /* Linux backend handle  */

} obj_meta_t;
```

逻辑结构：

```text
+---------------+
| obj_key_t key  |
+---------------+
| refcnt        |
+---------------+
| state         |
+---------------+
| obj_handle_t  |
|  .mount_id    |
|  .type        |
|  .len         |
|  .data[]      |
+---------------+
```


---

### 5.3 内存布局

```text
Offset  Size    Field
------  ----    ----------------
0       8       key.objectid
8       4       key.gen
12      4       refcnt
16      4       state
20      4       handle.mount_id
24      2       handle.type
26      2       handle.len
28      16      handle.data
44      4       (padding)

Total = 48 Bytes
```


---

### 5.4 编译期检查

保证结构尺寸固定：

```c
_Static_assert(sizeof(obj_meta_t) == OBJMETA_SIZE,
               "obj_meta_t size invalid");
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

### 7.1 状态机

ObjMeta 的生命周期由 `state` 字段驱动，objmgr 负责状态迁移：

```text
INIT ──→ ACTIVE ──→ DELETING ──→ DELETED
  │                    │
  └────────────────────┘
       (异常路径：直接销毁未激活对象)
```

| 状态 | 含义 | 触发操作 |
|------|------|----------|
| INIT | 刚分配，尚未激活 | objmeta_init() 置为此状态 |
| ACTIVE | 已激活，可正常使用 | objmgr 激活对象时迁移 |
| DELETING | 正在删除，阻止新引用 | objmgr 发起删除时迁移 |
| DELETED | 已销毁，等待回收 | objmgr 完成删除后迁移 |

objmeta 层仅通过 `state` 字段保存状态值，不参与状态迁移决策。
refcnt 的增减由 objmgr 在状态迁移前后通过原子操作完成。

### 7.2 创建

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

建立映射，此时 state = INIT, refcnt = 0。

### 7.3 激活

objmgr 将 state 从 INIT 迁移至 ACTIVE，并根据需要增加 refcnt。

### 7.4 查询

通过：

```text
(objectid, gen)
```

查找：

```text
ObjMeta
```

获得后端定位信息及当前 state / refcnt。

### 7.5 重开对象

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
(objectid, gen)
    ↓
ObjMeta
    ↓
open_by_handle_at()
    ↓
fd
```

### 7.6 删除

objmgr 将 state 迁移为 DELETING → DELETED，递减 refcnt。

最终调用：

```c
objmeta_reset()
```

清空所有字段（refcnt 归零、state 归零、handle 归零）。


---

## 8. 相等性判断

两个 ObjMeta 视为相同，需满足：

```text
objectid    相等
gen         相等
mount_id    相等
handle_type 相等
handle_bytes 相等
file_handle 相等
```

注意：refcnt 和 state 不参与相等性判断。它们是生命周期管理字段，
不影响对象身份标识。

对应接口：

```c
objmeta_equal()
```


---

## 9. 有效性判断

合法 ObjMeta 必须满足：

### objkey 有效

```text
objectid != 0
gen      != 0
```

### handle 长度合法

```text
0 < handle_bytes <= 16
```

注意：objmeta_is_valid() 仅检查结构和数据合法性，不检查 state 字段。
状态是否处于 ACTIVE 等可用状态由 objmgr 负责判断。

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

ObjMeta 表示对象定位信息与生命周期状态。

```text
ObjMeta
    ↓
(objectid, gen) → (mount_id, file_handle) + refcnt + state
```

两者职责不同。

```text
           +------+
           | FUID |
           +------+
               |
               | (objectid, gen)
               v

         +------------+
         | ObjMeta    |
         | .key       |
         | .refcnt    |
         | .state     |
         | .handle    |
         +------------+

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
gen          : 1
refcnt       : 2
state        : 1
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

核心职责：

- 对象定位映射：`(objectid, gen) → backend object locator`
- 生命周期支撑：提供 refcnt + state 字段供 objmgr 使用

不参与路径解析和目录树管理。