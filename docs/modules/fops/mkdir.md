# Mkdir 操作

mkdir 类操作负责创建目录。

## 接口

- `fops_mkdir()`：创建目录并返回对象身份。
- `fops_mkdir_plus()`：创建目录并返回对象身份和属性。

## 参数语义

必要输入：

- `parent_fuid`：父目录对象。
- `name`：新目录名。
- `attr`：创建属性，至少包含目录 mode。
- `flags`：目录创建控制标志。

输出：

- `out_fuid`：新目录对象身份。
- `out_attr`：仅 plus 接口需要。

## flag 语义

mkdir 推荐支持：

- `FS_FLAG_EXCLUSIVE`：目标已存在则失败。
- `FS_FLAG_DIRECTORY`：声明创建目标是目录。

如果上层传入 `REGULAR`，应由 spec 校验拒绝。

## 特殊语义

新目录创建后必须满足：

- `.` 指向自身。
- `..` 指向父目录。
- 根目录的父目录仍是自身或由 FSC 根规则决定。

这些语义可以由底层文件系统天然保证，也可以由 MirageFS 元数据层补齐。
