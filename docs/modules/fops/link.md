# Link 与 Symlink 操作

link 类操作负责创建硬链接；symlink 类操作负责创建符号链接。

## hard link 接口

- `fops_link()`：创建硬链接并返回新目录项对应对象身份。
- `fops_link_plus()`：创建硬链接并返回属性。

必要输入：

- `target_fuid`：被链接对象。
- `parent_fuid`：新目录项所在父目录。
- `name`：新目录项名称。
- `flags`：创建控制标志。

硬链接成功后，新目录项和原对象共享同一个对象身份或共享同一底层对象映射，链接计数应增加。

## symlink 接口

- `fops_symlink()`：创建符号链接并返回符号链接对象身份。
- `fops_symlink_plus()`：创建符号链接并返回属性。

必要输入：

- `parent_fuid`：新符号链接所在父目录。
- `name`：符号链接名称。
- `target_path`：符号链接保存的目标路径文本。
- `attr`：创建属性。
- `flags`：创建控制标志。

符号链接保存的是路径文本，不保证目标当时存在。

## flag 语义

link / symlink 推荐支持：

- `FS_FLAG_EXCLUSIVE`：目标名称已存在则失败。

## plus 语义

`link_plus` 和 `symlink_plus` 返回的属性必须是新目录项语义下可见的属性。对硬链接来说通常等价于目标对象属性；对符号链接来说应返回符号链接本身属性，而不是被链接目标属性。
