# Handle 操作

handle 类操作负责打开、关闭和获取文件句柄。

## open

`fops_open()` 根据对象身份打开文件或目录。

必要输入：

- `target_fuid`：目标对象。
- `flags`：打开控制标志。

输出：

- `out_handle`：FOPS 句柄。

推荐 flag：

- `FS_FLAG_READ`：读权限。
- `FS_FLAG_WRITE`：写权限。
- `FS_FLAG_APPEND`：追加写。
- `FS_FLAG_TRUNCATE`：打开时截断。
- `FS_FLAG_SYNC`：同步写。
- `FS_FLAG_DIRECT`：直接 I/O 倾向。
- `FS_FLAG_DIRECTORY`：要求目录。
- `FS_FLAG_REGULAR`：要求普通文件。

`DIRECTORY` 与 `REGULAR` 互斥。

## openhandle

`fops_openhandle()` 使用已有底层句柄或 handle 描述打开 FOPS 句柄。它适合恢复、桥接或测试场景。

## gethandle

`fops_gethandle()` 查询对象当前可用 handle 信息，但不一定打开新句柄。

## close

`fops_close()` 关闭 FOPS 句柄，释放对应引用。

关闭必须具备幂等或明确错误语义：重复关闭不能导致双重释放。

## 生命周期

句柄持有期间，ObjMgr 中的运行时对象不能被提前销毁。unlink 之后仍有句柄打开时，对象应进入可回收但未释放状态。
