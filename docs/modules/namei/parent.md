# NAMEI Parent Resolution

`namei_lookup_parent()` 将路径转换为：

```text
parent_fuid + leaf name
```

它用于 create、mkdir、symlink、unlink、rmdir、rename 等路径版薄封装。

v1 约束：

- `/` 没有父目录解析结果；
- leaf name 不能为空；
- leaf name 不能是 `.` 或 `..`；
- leaf name 不能包含 `/`；
- 带 trailing slash 的路径当前按非法 leaf path 处理。
