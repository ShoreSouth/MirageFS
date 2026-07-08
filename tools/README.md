# MirageFS Tools

`tools/check/` 存放项目自定义静态检查工具。当前入口是：

```sh
python3 tools/check/miragefs_lint.py
```

默认检查 `src` 源代码目录，适合作为日常开发后的基础检查。指定路径时只检查对应路径：

```sh
python3 tools/check/miragefs_lint.py src/fops docs/modules/fops tools/check
```

默认模式只让低噪音硬错误失败；使用 `--strict` 时，风格类 WARN 也会导致返回非 0。
全仓库检查使用 `--all`，会覆盖 `src`、`tools` 和 `docs`。
CI 或日志收集场景可使用 `--plain` 关闭 banner、进度条和表格。
`--detail-limit N` 可控制问题明细表最多显示多少条记录。
