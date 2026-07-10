# NAMEI Flags

NAMEI 复用 `fs_flags_t`，v1 不新增 LOOKUP_* flag。

当前语义：

- `FS_FLAG_NOFOLLOW` 只影响 final component；
- `FS_FLAG_DIRECTORY` 和 `FS_FLAG_REGULAR` 作为最终对象类型约束传给 FOPS；
- create/open/read/write 等具体操作 flag 由 NAMEI 的路径版薄封装继续透传给 FOPS。

中间 component 始终按目录遍历语义处理。如果 final component 之前出现非目录对象，NAMEI 返回 `ENOTDIR`。
