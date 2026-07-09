# Create 操作

create 类操作负责在父目录下创建普通文件。

## 接口

- `fops_create()`：创建成功后返回对象身份。
- `fops_create_plus()`：创建成功后返回对象身份和属性。

## 参数语义

必要输入：

- `parent_fuid`：父目录对象。
- `name`：新文件名。
- `attr`：创建属性，至少包含 mode；未来可扩展 uid/gid/time/size 等受支持字段。
- `flags`：创建控制标志。

输出：

- `out_fuid`：新对象身份。
- `out_attr`：仅 plus 接口需要。

## attr 而不是 mode

创建类 OP 使用 `fops_attr_t` 比单独使用 mode 更合理。原因是创建语义天然接近“create + setattr”：除权限位外，还可能需要 owner、group、时间戳、初始大小或特殊属性。

当前实现可以只消费 mode，但接口层应保留 attr 形态，避免后续 ABI 频繁扩张。

## flag 语义

create 推荐支持：

- `FS_FLAG_EXCLUSIVE`：目标已存在则失败。
- `FS_FLAG_REPLACE`：允许替换已存在目标。
- `FS_FLAG_TRUNCATE`：目标存在且可写时截断。
- `FS_FLAG_NOFOLLOW`：不跟随末端符号链接。
- `FS_FLAG_SYNC`：创建后要求同步语义。
- `FS_FLAG_DIRECT`：为后续打开语义预留直接 I/O 倾向。
- `FS_FLAG_APPEND`：为后续打开语义预留追加倾向。
- `FS_FLAG_REGULAR`：要求创建普通文件。

`EXCLUSIVE` 与 `REPLACE` 互斥。

## plus 语义

`create_plus` 创建成功后应返回最终属性。属性应来自同一次底层结果或紧随其后的 getattr，不允许返回调用者传入 attr 的简单拷贝。
