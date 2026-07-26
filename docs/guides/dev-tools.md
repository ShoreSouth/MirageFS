# 开发工具

本文记录 MirageFS 日常开发推荐使用的工具、安装方式和脚本入口。后续新增稳定工具时，应同步更新本文和 `tools/README.md`。

## 基础安装

推荐在 WSL/Ubuntu 中安装：

```sh
sudo apt-get update
sudo apt-get install -y build-essential make gcc binutils clang-format
```

UT、覆盖率和调试工具：

```sh
sudo apt-get install -y libcmocka-dev pkg-config lcov gdb
```

## clang-format

`clang-format` 是 C/C++/Objective-C 等语言的自动排版工具。MirageFS 在仓库根目录提供 `.clang-format`，基于成熟的 LLVM 风格调整为项目本地规则：

- 4 空格缩进。
- 80 列行宽。
- 左花括号另起一行。
- 指针星号靠近变量名，例如 `char *path`。
- 不自动排序 include，保留项目已有 include 分组语义。
- 不自动重排注释文字，避免中文注释被工具改写。

检查格式：

```sh
python3 tools/check/format.py
```

格式化指定路径：

```sh
python3 tools/check/format.py --fix src/object tests/object
```

格式化全部默认范围：

```sh
python3 tools/check/format.py --fix
```

建议把大规模格式化作为独立提交，不和功能改动混在同一个 diff 中。

## Git commit 模板

仓库根目录的 `.gitmessage` 提供 MirageFS 提交信息提示。首次使用时在仓库
根目录执行：

```sh
git config --local commit.template "$(git rev-parse --show-toplevel)/.gitmessage"
git config --local core.editor vim
```

之后执行：

```sh
git commit
```

Git 会在 Vim 中打开模板。填写完成后使用 `:wq` 保存并提交，使用 `:cq`
退出并取消提交。`git commit -m "..."` 会绕过模板和交互编辑器。

小提交只需要一行标题：

```text
fix(namei): 修复根目录下父路径解析
```

较大的提交按需增加背景、改动、验证和影响：

```text
feat(msh): 增强 stat 的 Linux 风格属性展示

背景：
- 原 stat 缺少块信息、纳秒时间和 birth time。

改动：
- LSA 增加 statx 封装。
- 使用 FSID/FUID 替代后端 Device/Inode。

验证：
- check-all.sh 和全量测试通过。

影响：
- 扩展 FOPS 属性快照和 Runtime stat 入口。
```

推荐的标题格式是 `<type>(<scope>): <简洁中文说明>`。可用 type 和 scope
列表以 `.gitmessage` 中的最新说明为准。

## MirageFS lint

`tools/check/miragefs_lint.py` 是项目自定义轻量静态检查工具，用于检查编码、空白、行宽、命名、裸错误返回、大栈数组等项目规则。

默认检查 `src`：

```sh
python3 tools/check/miragefs_lint.py
```

检查全仓库：

```sh
python3 tools/check/miragefs_lint.py --all
```

严格模式会把 WARN 也视为失败：

```sh
python3 tools/check/miragefs_lint.py --all --strict
```

## 一键静态检查

`tools/check/check-all.sh` 是本地快速门禁入口，当前包含：

```text
miragefs_lint.py --all --plain
format.py
list-ut.py --check
```

运行：

```sh
tools/check/check-all.sh
```

它不运行 UT，只覆盖静态检查和 UT 编号检查。需要运行测试时继续使用：

```sh
make test
```

## 工具维护约定

新增实用工具时，应同步补充：

- `tools/README.md`：说明脚本入口、参数和适用场景。
- `docs/guides/dev-tools.md`：说明安装方式、工具用途和日常使用方法。
- `.agents/skills/miragefs-core/SKILL.md`：如果工具影响代码风格、检查门禁或日常工作流，应补充对应约束。
