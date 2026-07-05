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

ObjMeta 承担唯一职责：

**定位桥梁**：MirageFS Object ↔ Linux Backend Object

生命周期相关信息（refcnt、state）已移至 obj_runtime_t，
详见 [ObjRuntime 设计文档](../objruntime/objruntime.md)。

```text
MirageFS Object
    ↕
obj_runtime_t  (meta, refcnt, state)
    ↕
obj_meta_t  (key, handle)
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
sizeof(obj_meta_t) == 40
```

字段构成：

```text
obj_key_t        : 16 bytes  (objectid + gen + padding)
obj_handle_t    : 24 bytes  (mount_id + type + len + data[16])
─────────────────────────
Total           : 40 bytes
```

生命周期信息由 obj_runtime_t 管理（sizeof == 48），
obj_meta_t 保持轻量，仅描述对象固有属性。

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

### 5.1 生命周期状态

obj_state_t 枚举及状态迁移逻辑已移至 obj_runtime_t，
详见 [ObjRuntime 设计文档](../objruntime/objruntime.md)。

obj_meta_t 仅保存对象固有属性，不包含运行时状态。

### 5.2 obj_meta_t 结构定义

```c
typedef struct obj_meta {

    obj_key_t       key;    /* MirageFS 对象唯一标识 */
    obj_handle_t   handle; /* Linux backend handle  */

} obj_meta_t;
```

逻辑结构：

```text
+---------------+
| obj_key_t key  |
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
12      4       (key padding)
16      4       handle.mount_id
20      2       handle.type
22      2       handle.len
24      16      handle.data

Total = 40 Bytes
```


---

### 5.4 编译期检查

保证结构尺寸固定：

```c
#define OBJMETA_SIZE 40

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

生命周期状态迁移由 obj_runtime_t 管理，详见 [ObjRuntime 设计文档](../objruntime/objruntime.md)。

obj_meta_t 本身是无状态的值对象，仅保存 key + handle。

### 7.2 创建

objmgr_create() 流程：

```text
objpool_alloc()           → 分配 obj_runtime_t
    ↓
objmeta_init(&rt->meta, fuid, handle)  → 初始化 meta 部分
    ↓
rt->state = INIT          → objmgr 设置初始状态
    ↓
objmgr_insert_locked(rt)  → 注册到 ObjTable
    ↓
objmgr_change_state(rt, ACTIVE)  → 激活
```

其中 `objmeta_init()` 内部完成：
- `objkey_from_fuid()` — 从 FUID 构造内部 key
- handle 整体拷贝

refcnt 和 state 的初始化由 objmgr 在 runtime 层完成。

### 7.3 激活

objmgr 将 state 从 INIT 迁移至 ACTIVE。

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

objmgr 将 state 迁移为 ACTIVE → DELETING，递减 refcnt。

当 refcnt 降为 0 时：
1. `objtable_remove()` — 从对象表中移除
2. `objmeta_reset()` — 清空元数据（memset(0)）
3. `objpool_free()` — 归还内存池

对象释放后即不存在，不再保留任何状态。


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
objmeta_dump()       /* 输出 key + handle */
objruntime_dump()    /* 输出 meta + refcnt + state */
```

objmeta_dump 输出内容：

```text
objmeta: objectid=100, gen=1, mount_id=23, handle_type=1, handle_bytes=8, file_handle=0a01bcff...
```

`objmeta_dump()` 和 `objruntime_dump()` 都保持单行输出。
`objruntime_dump()` 额外输出 refcnt 和 state。
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


## 8. API 命名与返回类型

`objmeta_init()` 是可失败函数，返回类型统一为 `fs_error_t`，调用方应使用
`fs_failed()` / `fs_succeeded()` 判断结果。

`obj_meta_t` 是嵌入式值结构，不拥有外部 heap 资源。清空接口使用
`objmeta_deinit()`，语义是将结构体恢复为零值、未初始化状态。

为了兼容旧调用点，当前仍保留 `objmeta_reset()` 作为薄封装。新代码应优先
使用 `objmeta_deinit()`，后续调用点全部迁移后可以移除旧名称。
