# Xattr 操作

xattr 类操作负责扩展属性读取、写入、枚举和删除。

## 接口

- `fops_getxattr()`：读取指定扩展属性。
- `fops_setxattr()`：写入指定扩展属性。
- `fops_listxattr()`：枚举扩展属性名称。
- `fops_removexattr()`：删除指定扩展属性。

## 参数语义

公共输入：

- `target_fuid`：目标对象。
- `name`：扩展属性名称，listxattr 不需要。
- `buffer`：值缓冲区或名称列表缓冲区。
- `size`：缓冲区大小。
- `flags`：操作控制标志。

输出：

- `out_size`：实际读取或需要的大小。

## flag 语义

setxattr 推荐支持：

- `FS_FLAG_EXCLUSIVE`：属性已存在则失败。
- `FS_FLAG_REPLACE`：属性不存在则失败。

`EXCLUSIVE` 与 `REPLACE` 互斥。

## 错误语义

- 属性不存在：返回 not found 类错误。
- 缓冲区过小：返回 range/buffer too small 类错误，并通过 `out_size` 提示需要大小。
- 后端不支持 xattr：返回 unsupported 类错误。
