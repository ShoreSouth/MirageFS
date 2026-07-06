# Namespace 模块设计

## 1. 模块定位

`namespace/` 定义 FSC 的运行时对象：

```text
fsc_namespace_t
```

Namespace 表示 MirageFS 中一个 filesystem instance 的控制面入口。
它不是路径树，也不是目录项缓存，而是文件系统生命周期和根对象身份的
运行时承载体。

## 2. 模块职责

Namespace 模块负责：

- 定义 `fsc_namespace_t`；
- 定义 namespace 生命周期状态；
- 初始化和清理 namespace 对象；
- 校验 namespace 对象和名称；
- 提供状态机迁移；
- 提供单行 debug dump。

Namespace 模块不负责：

- 内存分配；
- hash 索引；
- FSID 分配；
- 创建或删除后端目录；
- 打开 fd；
- mount / policy 语义。

## 3. 数据结构

核心结构如下：

```c
typedef struct fsc_namespace {

    fsc_fsid_t      fsid;
    char            name[FSC_NAMESPACE_NAME_MAX];

    fuid_t          root_fuid;
    obj_handle_t    root_handle;

    fs_atomic32_t   refcnt;
    uint32_t        state;

    uint8_t         reserved[24];

} fsc_namespace_t;
```

字段说明：

| 字段 | 含义 |
| --- | --- |
| `fsid` | filesystem identity |
| `name` | namespace 名称，也是 sysroot 下的根目录名 |
| `root_fuid` | 文件系统根目录对象的 FUID，对外查询优先使用 |
| `root_handle` | 根目录 backend handle，供 Object/LSA 衔接 |
| `refcnt` | 引用计数，预留给后续 acquire/release |
| `state` | 生命周期状态 |
| `reserved` | 预留字段，保持结构体大小稳定 |

结构体不保存 fd/path。FSC 上层的身份流转以 FUID 为主；需要访问后端时，
应通过 FUID 找到元数据中的 handle，再进入 LSA 边界打开临时 fd。

当前结构体大小固定为：

```text
FSC_NAMESPACE_SIZE == 192
```

## 4. 根对象身份

每个 filesystem root 使用保留 object id：

```text
objectid = FSC_NAMESPACE_ROOT_OBJECT_ID
.gen     = FSC_NAMESPACE_ROOT_GEN
.type    = FUID_TYPE_DIR
```

FSID 来自 `fsid_alloc()`。因此根目录 FUID 形如：

```text
(fsid, objectid=1, gen=1, type=dir)
```

后续普通对象的 object id 分配不应复用这个保留值。

`fsc_namespace_t` 同时保存 `fsid` 和 `root_fuid`，这是刻意保留的
语义冗余：

- `fsid` 是 namespace 控制面的主索引；
- `root_fuid` 是根目录对象的完整身份，未来可承载 qtree、snapshot、
  shard 等视图信息。

初始化和有效性检查必须保证：

```text
ns->fsid == ns->root_fuid.fsid
ns->root_fuid.type == FUID_TYPE_DIR
ns->root_fuid.objectid == FSC_NAMESPACE_ROOT_OBJECT_ID
ns->root_fuid.gen == FSC_NAMESPACE_ROOT_GEN
```

## 5. 生命周期状态

当前状态：

```text
FSC_NAMESPACE_STATE_INVALID
FSC_NAMESPACE_STATE_INIT
FSC_NAMESPACE_STATE_ACTIVE
FSC_NAMESPACE_STATE_DELETING
```

允许迁移：

```text
INIT -> ACTIVE -> DELETING
```

状态修改必须通过 `fsc_namespace_change_state()` 完成，不能直接改写
`ns->state`。

## 6. 初始化语义

`fsc_namespace_init()` 只填充字段：

```c
fs_error_t fsc_namespace_init(
                fsc_namespace_t *ns,
                fsc_fsid_t fsid,
                const char *name,
                const fuid_t *root_fuid,
                const obj_handle_t *root_handle);
```

它不负责：

- 分配 namespace 内存；
- 创建 sysroot 下的目录；
- 插入 fstable；
- 设置 ACTIVE；
- 增加引用计数。

这些动作由 `fsmgr_create()` 编排。

## 7. 所有权

Namespace 对象必须来自：

```text
nspool_alloc()
```

释放必须通过：

```text
nspool_free()
```

`fstable` 只保存借用指针。`fsmgr` 是当前唯一负责 namespace 生命周期编排的模块。
