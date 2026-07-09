# Lookup 操作

lookup 类操作负责在父目录中解析一个目录项名称，返回对应对象身份。

## 接口

- `fops_lookup()`：轻量查询，只返回 `obj_fuid_t`。
- `fops_lookup_plus()`：查询并返回属性，适合上层随后需要展示属性的场景。

## 参数语义

必要输入：

- `parent_fuid`：父目录对象。
- `name`：目录项名称。
- `flags`：只允许 lookup 支持的 flag。

输出：

- `out_fuid`：解析到的对象身份。
- `out_attr`：仅 plus 接口需要。

## name 规则

lookup 允许 `.` 和 `..`。

- `.` 返回当前父目录对象。
- `..` 返回父目录的父目录；根目录的 `..` 应保持在根目录。

如果底层 LSA 已支持 `.` / `..`，FOPS 可以直接复用；如果 LSA 不能完整表达 MirageFS 语义，FOPS 必须在自身层补齐。

## flag 语义

lookup 推荐支持：

- `FS_FLAG_NOFOLLOW`：遇到符号链接时不跟随。
- `FS_FLAG_DIRECTORY`：要求结果是目录。
- `FS_FLAG_REGULAR`：要求结果是普通文件。

`DIRECTORY` 与 `REGULAR` 互斥。

## 实现约束

lookup 应优先调用 LSA 封装好的 `lsa_lookup()`，避免 FOPS 直接拼接路径或重复底层解析逻辑。
