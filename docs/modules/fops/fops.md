# FOPS

FOPS 是 MirageFS 的 File Operations Layer。

它接收已经解析好的 `fuid_t`，以及单级路径分量，完成一次文件系统操作。FOPS 不解析完整路径，不维护 cwd，不处理 mount 名称，也不面向 server 协议；这些职责应由更上层模块承担。

## 模块定位

```text
上层 / 未来 NAMEI
        |
        v
FOPS: 一个 FUID / 一个父目录 + 一个名字 / 一个 OP
        |
        +--> ObjMgr: 运行时对象身份、handle 索引、objectid/gen 分配
        |
        +--> LSA: Linux syscall 边界
```

后端稳定身份是 `mount_id + file_handle`。MirageFS 运行时身份是 `fuid_t` 中的 `objectid + gen`。

FOPS 发现对象时，先用后端 handle 查询 ObjMgr。已存在对象复用原有 `objectid/gen`；新发现对象由 ObjMgr 分配 `obj_key_t`，再注册 handle 映射。

## 对外入口

FOPS 现在提供两层接口：

1. 统一调度入口：`fops_dispatch(fops_args_t *args)`。
2. 细粒度接口：`fops_lookup()`、`fops_create()`、`fops_rename()` 等。

上层推荐优先依赖统一调度入口。调用路径如下：

```text
fops_dispatch(args)
    -> fops_validate_args(args)
        -> 查询 fops_op_spec_t
        -> 校验 op / flags / 基础指针 / name 规则
    -> g_fops_ops[args->op](args)
        -> 调用现有细粒度 fops_lookup/create/... 实现
```

细粒度接口仍保留，主要用于 FOPS 内部复用、测试、以及少量直接调用场景。

## 统一参数结构

`fops_args_t` 是统一 OP 参数对象。公共区只放高频且语义稳定的字段：

```c
fs_op_t op;
fs_flags_t flags;
const fuid_t *fuid;
const fuid_t *parent_fuid;
const char *name;
```

每个 OP 的特有参数放入 `union`。例如：

```c
args.op = FS_OP_CREATE;
args.parent_fuid = parent;
args.name = "file";
args.flags = FS_FLAG_EXCLUSIVE;
args.u.create.attr = &attr;
args.u.create.out = &result;

err = fops_dispatch(&args);
```

这样上层可以用统一函数指针调度所有 OP，同时避免单个函数参数持续膨胀。

## OP Spec 表

FOPS 内部使用 `fops_op_spec_t` 管理每个 OP 的通用约束：

- 操作字是否支持。
- 允许哪些 `FS_FLAG_*`。
- 哪些 flag 互斥。
- 是否允许 `.` / `..`。
- 是否要求 `fuid`、`parent_fuid`、`name`。

具体业务规则仍留在对应 OP 实现中。例如 `mknod` 的 `device` 只在 `BLK/CHR` 时必需，这类规则不放入通用 spec 表。

## 对象身份

`objectid` 由 MirageFS 分配，不复制 Linux inode/stat 字段。ObjMgr 拥有分配器并返回 `obj_key_t`：

```text
obj_key_t = objectid slot + generation
fuid_t    = fsid + objectid + gen + type + view fields
```

对象运行时回收时，ObjMgr 释放 slot 并推进 `gen`。旧 generation 的 FUID 在 slot 复用后不会匹配新对象。

## 名字规则

FOPS 只接受单级路径分量，拒绝包含 `/` 的名字。

lookup 支持 `.` 和 `..`：

- `.` 解析为父目录自身，并读取父目录属性。
- `..` 交给 LSA/Linux 后端目录语义处理。

create、mkdir、mknod、unlink、rmdir、rename、link、symlink 等修改类 API 拒绝 `.` 和 `..`。

## Flags

公共 flags 定义在 `src/common/flag/fs_flag.h`。

| Flag | 语义 |
| --- | --- |
| `FS_FLAG_NONE` | 无附加约束。 |
| `FS_FLAG_REPLACE` | 允许复用或覆盖已存在目标；仅具体 OP 支持时有效。 |
| `FS_FLAG_EXCLUSIVE` | 目标必须不存在；与 `REPLACE` 冲突。 |
| `FS_FLAG_NOFOLLOW` | 不跟随最终 symlink 分量。 |
| `FS_FLAG_SYNC` | 打开/创建时使用同步写语义。 |
| `FS_FLAG_DIRECT` | 请求 direct I/O。 |
| `FS_FLAG_DIRECTORY` | 目标必须是目录。 |
| `FS_FLAG_REGULAR` | 目标必须是普通文件。 |
| `FS_FLAG_TRUNCATE` | 打开/创建时截断普通文件。 |
| `FS_FLAG_APPEND` | 追加写。 |
| `FS_FLAG_READ` | 以读方向打开。 |
| `FS_FLAG_WRITE` | 以写方向打开。 |

每个 OP 支持的 flag 由 `fops_op_spec_t` 统一维护。未知或不支持的 flag 会返回 `EINVAL`。

## 属性结构

`fops_attr_t` 是输出属性快照，由 `getattr`、`lookup_plus`、`create_plus`、`mkdir_plus`、`readdirplus` 等接口返回。

`fops_create_attr_t` 是创建类请求。`valid_mask` 决定哪些字段有效：

| Mask | 适用 OP | 语义 |
| --- | --- | --- |
| `FOPS_CREATE_ATTR_MODE` | `create`、`mkdir`、部分 `mknod` | 创建权限。 |
| `FOPS_CREATE_ATTR_UID` | `create`、`mkdir` | owner uid。 |
| `FOPS_CREATE_ATTR_GID` | `create`、`mkdir` | owner gid。 |
| `FOPS_CREATE_ATTR_SIZE` | `create` | 初始普通文件大小。 |

`fops_setattr_t` 用于属性修改，语义类似，但面向已有对象。

## Plus 接口

轻量接口只返回 `fuid_t`：

```c
fops_lookup(..., fuid_t *out_fuid);
fops_create(..., fuid_t *out_fuid);
fops_mkdir(..., fuid_t *out_fuid);
```

plus 接口在同一次操作中返回 FUID 和属性：

```c
fops_lookup_plus(..., fops_object_result_t *out);
fops_create_plus(..., fops_object_result_t *out);
fops_mkdir_plus(..., fops_object_result_t *out);
```

当前 lookup/create/mkdir/link/symlink/mknod 的轻量接口都以 plus 路径为核心。

## 文件句柄

`fops_open()` 和 `fops_openhandle()` 返回 opaque `fops_file_t`。调用方必须用 `fops_close()` 释放。

FOPS 不向上层暴露 Linux fd。读写类操作接收 `fops_file_t *`：

```c
fops_read(file, buf, size, &actual);
fops_write(file, buf, size, &actual);
fops_pread(file, buf, size, offset, &actual);
fops_pwrite(file, buf, size, offset, &actual);
```

## OP 覆盖

| OP | FOPS API |
| --- | --- |
| `LOOKUP` | `fops_lookup`、`fops_lookup_plus`、`fops_dispatch` |
| `CREATE` | `fops_create`、`fops_create_plus`、`fops_dispatch` |
| `MKDIR` | `fops_mkdir`、`fops_mkdir_plus`、`fops_dispatch` |
| `MKNOD` | `fops_mknod`、`fops_mknod_plus`、`fops_dispatch` |
| `UNLINK` | `fops_unlink`、`fops_dispatch` |
| `RMDIR` | `fops_rmdir`、`fops_dispatch` |
| `RENAME` | `fops_rename`、`fops_dispatch` |
| `LINK` | `fops_link`、`fops_link_plus`、`fops_dispatch` |
| `SYMLINK` | `fops_symlink`、`fops_symlink_plus`、`fops_dispatch` |
| `OPEN` / `OPENHANDLE` | `fops_open`、`fops_openhandle`、`fops_dispatch` |
| `CLOSE` | `fops_close`、`fops_dispatch` |
| `GETHANDLE` | `fops_gethandle`、`fops_dispatch` |
| `GETATTR` / `SETATTR` | `fops_getattr`、`fops_setattr`、`fops_dispatch` |
| `ACCESS` | `fops_access`、`fops_dispatch` |
| `READ` / `WRITE` | `fops_read`、`fops_write`、`fops_pread`、`fops_pwrite`、`fops_dispatch` |
| `TRUNCATE` | `fops_truncate`、`fops_dispatch` |
| `GETXATTR` / `SETXATTR` | `fops_getxattr`、`fops_setxattr`、`fops_dispatch` |
| `LISTXATTR` / `REMOVEXATTR` | `fops_listxattr`、`fops_removexattr`、`fops_dispatch` |
| `READDIR` / `READDIRPLUS` | `fops_readdir`、`fops_readdirplus`、`fops_dispatch` |
| `STATFS` / `SYNCFS` | `fops_statfs`、`fops_syncfs`、`fops_dispatch` |

## 替换语义

创建类名字操作采用保守替换语义：

- `create` 支持 `REPLACE`，因为 Linux open 能原子复用已有普通文件。
- `rename` 支持 `REPLACE`，成功后会从 ObjMgr 删除被覆盖目标的对象映射。
- `link`、`symlink`、`mknod` 不支持 `REPLACE`；目标已存在时返回 `EEXIST`。

## FSC 边界说明

FSC 将每个 filesystem root 注册为普通 ObjMgr 对象，使 FOPS 能统一操作 root 和非 root 目录。
FSC 仍然拥有 namespace 生命周期和 `fsid`；ObjMgr 只负责运行时对象身份与后端 handle 查询。

## 源码文件组织

FOPS 的 `ops/` 目录按语义族组织，而不是按实现时间组织：

```text
ops/fops_lookup.c    lookup / lookup_plus
ops/fops_create.c    create / create_plus
ops/fops_mkdir.c     mkdir / mkdir_plus
ops/fops_mknod.c     mknod / mknod_plus
ops/fops_attr.c      getattr / setattr / access
ops/fops_readdir.c   readdir / readdirplus
ops/fops_unlink.c    unlink
ops/fops_rmdir.c     rmdir
ops/fops_rename.c    rename
ops/fops_link.c      link / symlink
ops/fops_handle.c    gethandle / open / openhandle / close
ops/fops_rw.c        read / write / pread / pwrite / truncate
ops/fops_xattr.c     xattr 操作族
ops/fops_fs.c        statfs / syncfs
```

`fops_helper.c` 承载跨 OP 复用的内部 helper，例如 name/flag 校验、对象打开、parent 目录打开、
handle 转换等。单个 OP 文件不应复制这些公共逻辑。

## 静态检查

FOPS 开发阶段的默认检查命令：

```sh
python3 tools/check/miragefs_lint.py
```

默认检查 `src/fops` 和 `tools/check`，只让硬错误导致失败。指定路径时可扩展到文档：

```sh
python3 tools/check/miragefs_lint.py src/fops docs/modules/fops tools/check
```

`--strict` 会把 WARN 也视为失败；`--all` 用于全仓库检查，适合后续清理历史问题后纳入门禁。
