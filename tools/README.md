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

`tools/check/format.py` 是 `clang-format` 包装器，默认检查 `src` 和 `tests` 下的 C/H 文件：

```sh
python3 tools/check/format.py
python3 tools/check/format.py --fix src/object tests/object
```

`tools/check/check-all.sh` 是本地一键静态检查入口，当前会执行：

```text
python3 tools/check/miragefs_lint.py --all --plain
python3 tools/check/format.py
tools/test/list-ut.py --check
```

运行：

```sh
tools/check/check-all.sh
```

它只做静态检查和 UT 编号检查，不替代 `make test`。


## UT 工具

`tools/test/run-ut.sh` 是日常 UT 运行入口，支持全部、单模块、单 case、覆盖率和 case 列表。

```sh
tools/test/run-ut.sh all
tools/test/run-ut.sh fops
tools/test/run-ut.sh fops 0x06041001
tools/test/run-ut.sh coverage
```

`tools/test/list-ut.py` 负责从源码中提取 UT case 索引，支持按模块、`list_no`、`case_no`、函数名检索，也可以输出 JSON。

```sh
tools/test/list-ut.py --module fops
tools/test/list-ut.py --list 0x06041000
tools/test/list-ut.py --case 0x06041001
tools/test/list-ut.py --check
```
