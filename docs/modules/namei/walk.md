# NAMEI Walk

`namei_walk()` 是 NAMEI 的内部遍历核心，不作为公开 API 暴露。

遍历规则：

1. 绝对路径从 `ctx->root_fuid` 开始。
2. 相对路径从 `ctx->cwd_fuid` 开始。
3. 重复 `/` 会被跳过。
4. `.` 会被跳过。
5. `..` 在 `ctx->root_fuid` 处停留；其他位置通过 FOPS lookup 解析。
6. 普通 component 先用 `FS_FLAG_NOFOLLOW` lookup，确认它本身是否为 symlink。
7. symlink 展开次数受 `ctx->max_symlink_depth` 限制；为 0 时使用 `NAMEI_SYMLINK_MAX`。

NAMEI 不使用 `fs_path_normalize()` 做语义遍历。字符串层提前折叠 `..` 会绕过 namespace root 边界，也会吞掉 symlink 相关语义。
