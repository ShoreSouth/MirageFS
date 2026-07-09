# Read / Write 操作

rw 类操作负责文件数据读写。

## read / pread

- `fops_read()`：按句柄当前位置读取，并推进当前位置。
- `fops_pread()`：按指定 offset 读取，不改变句柄当前位置。

必要输入：

- `handle`：已打开句柄。
- `buffer`：输出缓冲区。
- `size`：期望读取字节数。
- `offset`：仅 pread 使用。

输出：

- `out_size`：实际读取字节数。

EOF 不是错误；读取到 0 字节且返回成功表示到达文件末尾。

## write / pwrite

- `fops_write()`：按句柄当前位置写入，并推进当前位置。
- `fops_pwrite()`：按指定 offset 写入，不改变句柄当前位置。

必要输入：

- `handle`：已打开句柄。
- `buffer`：输入缓冲区。
- `size`：期望写入字节数。
- `offset`：仅 pwrite 使用。

输出：

- `out_size`：实际写入字节数。

## flag 语义

读写通常继承 open 句柄上的模式。单次 read/write 如果支持额外 flag，应保持克制，避免和 open flag 产生冲突。

## 实现约束

- 不允许对目录执行普通 read/write，除非明确实现目录流语义。
- 写入必须校验句柄具备写权限。
- append 句柄的 write 应写到文件末尾；pwrite 是否允许绕过 append 需要明确约定。
- 短读短写应通过 `out_size` 表达，不应一律视为错误。
