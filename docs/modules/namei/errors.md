# NAMEI Errors

NAMEI 使用独立模块 ID：

```text
FS_MODULE_NAMEI
```

当前 sub-error 分类：

- `NAMEI_SUB_INIT`
- `NAMEI_SUB_CTX`
- `NAMEI_SUB_PATH`
- `NAMEI_SUB_WALK`
- `NAMEI_SUB_LOOKUP`
- `NAMEI_SUB_PARENT`
- `NAMEI_SUB_OP`

下层 FOPS/LSA 返回的错误保持原样传播。NAMEI 只在上下文校验、路径解析、路径遍历、父目录解析失败时构造自己的错误码。
