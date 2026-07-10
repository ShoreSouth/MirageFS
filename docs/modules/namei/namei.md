# NAMEI 模块总纲

NAMEI 是 MirageFS 的路径命名解析层。它把上层传入的 `root/cwd + path + flags` 转换为目标 FUID，或转换为 FOPS 需要的 `parent_fuid + name` 参数组合。

NAMEI 不直接执行底层文件操作。路径版公开 API 只是薄封装：先解析路径，再调用对应的 FOPS 单步操作。

## 模块职责

NAMEI 负责：

- 绝对路径和相对路径解析；
- `.`、`..`、重复 `/` 的路径分量语义；
- 通过 `fops_lookup_plus()` 逐级遍历目录；
- 为 create、mkdir、link、rename、unlink、rmdir 等流程解析父目录；
- 通过 `fops_readlink()` 展开符号链接；
- 处理 final component 的 `FS_FLAG_NOFOLLOW` 语义。

NAMEI 不负责：

- namespace 生命周期管理；
- objectid/gen 分配；
- ObjMgr 注册或回收；
- backend fd/path 暴露；
- name cache 或 dentry cache；
- 完整 Linux namei 兼容。

## 分层关系

```text
APP / SERVER / CLI
  -> NAMEI
  -> FOPS
  -> FSC / ObjMgr / LSA
```

FSC 保存 namespace root identity。NAMEI 只消费调用方传入的 `namei_ctx_t`，不长期持有借用的 `fsc_namespace_t *`。

## 核心上下文

```c
typedef struct namei_ctx {
    fuid_t root_fuid;
    fuid_t cwd_fuid;
    uint32_t max_symlink_depth;
} namei_ctx_t;
```

绝对路径从 `root_fuid` 开始，相对路径从 `cwd_fuid` 开始。`..` 在 `root_fuid` 处被夹住，不能越过当前 namespace root。

## v1 API

- `namei_lookup()` / `namei_lookup_plus()`：解析完整路径。
- `namei_lookup_parent()`：解析 `parent_fuid + leaf name`。
- `namei_create()` / `namei_mkdir()` / `namei_symlink()`：创建命名对象。
- `namei_readlink()`：按路径读取符号链接内容。
- `namei_readdir()` / `namei_readdirplus()`：按路径读取目录。
- `namei_unlink()` / `namei_rmdir()` / `namei_rename()`：按路径封装命名类变更操作。
- `namei_getattr()` / `namei_open()`：按路径封装对象级操作。

## 符号链接语义

NAMEI 遍历每个 component 时，先使用 `FS_FLAG_NOFOLLOW` lookup，确认该 component 本身是不是符号链接。

- 中间 component 是 symlink：读取链接内容并继续解析。
- final component 是 symlink，且调用方设置 `FS_FLAG_NOFOLLOW`：返回 symlink 本身。
- final component 是 symlink，且调用方未设置 `FS_FLAG_NOFOLLOW`：展开链接目标。

读取链路是：`namei_walk()` -> `fops_readlink()` -> `lsa_readlink()`。

## 文档导航

- [walk.md](walk.md)：路径遍历和符号链接展开。
- [parent.md](parent.md)：父目录解析。
- [flags.md](flags.md)：flag 处理。
- [errors.md](errors.md)：NAMEI 错误模块和 sub-error。
