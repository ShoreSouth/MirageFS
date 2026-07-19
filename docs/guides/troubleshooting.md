# 常见问题

本文记录 MirageFS 本地开发、构建、测试和调试中容易遇到的问题。

## `framework/test_framework.h` file not found

现象：

```c
#include "framework/test_framework.h"
```

VSCode 或 clangd 报：

```text
'framework/test_framework.h' file not found
```

常见原因是语言服务器不知道 `tests` 是 include path。实际 UT 编译使用：

```text
-I$(ROOT_DIR)/tests
```

如果命令行 `make test` 能通过，而 VSCode 爆红，通常只是编辑器索引问题。

处理方式：

1. 确认 `.clangd` 中包含 `-Itests`。
2. 在 VSCode 执行 `clangd: Restart language server`。
3. 必要时重新打开窗口。

## `make[1]: Leaving directory` 是报错吗

不是。

类似输出：

```text
make[1]: Leaving directory '/home/shore/work/github/MirageFS/tests'
make: Leaving directory '/home/shore/work/github/MirageFS'
```

这是 GNU Make 在进入和离开子目录时打印的提示，表示子 make 已结束。是否失败要看命令最终退出码，以及前面是否有 `error`、`failed`、`FAIL` 等信息。

如果 UT 汇总显示全部 PASS，就不是错误。

## 覆盖率报告在哪里

执行：

```sh
tools/test/run-ut.sh coverage
```

HTML 报告路径：

```text
output/coverage/html/index.html
```

数据文件：

```text
output/coverage/coverage.info
output/coverage/coverage.filtered.info
```

如果执行过 `make clean`，这些文件会被删除。

## `make clean` 后测试输出还在吗

当前 `make clean` 会删除：

```text
output/
tests/output/
```

也会删除源码目录下残留的覆盖率插桩文件：

```text
*.gcda
*.gcno
*.o
```

如果仍看到旧文件，先确认是否在 WSL 的项目目录下执行：

```sh
pwd
```

应为：

```text
/home/shore/work/github/MirageFS
```

## 文档或脚本中文显示乱码

文件应保持 UTF-8 编码。WSL bash、VSCode 通常能正常显示。

如果在某些 Windows 工具输出里看到乱码，常见原因是终端编码或工具转码问题。优先确认：

```sh
file docs/guides/unit-testing.md
```

以及 VSCode 右下角编码是否为 `UTF-8`。

命令、路径和 API 名称应保持英文，中文主要用于说明文字。

## `lcov` 或 `genhtml` 找不到

安装覆盖率工具：

```sh
sudo apt-get update
sudo apt-get install -y lcov
```

确认：

```sh
lcov --version
genhtml --version
```

## `cmocka` 找不到

当前 UT 框架暂未强依赖 cmocka，但后续可能用于 mock、fixture 或 fault injection。

安装：

```sh
sudo apt-get install -y libcmocka-dev pkg-config
```

确认：

```sh
pkg-config --libs cmocka
pkg-config --cflags cmocka
```

## 单模块测试失败但全量构建正常

先确认模块名是否在支持列表中：

```sh
tools/test/run-ut.sh list
```

当前支持：

```text
config common lsa object fsc fops namei runtime msh
```

单模块测试命令示例：

```sh
tools/test/run-ut.sh fops
```

如果是链接失败，检查该模块是否依赖其他静态库，以及 `tests/Makefile` 的 `LINK_LIBS` 是否包含完整库列表。

## 单 case 没有被执行

确认 case 名完全一致：

```sh
tools/test/run-ut.sh list fops
```

然后执行：

```sh
tools/test/run-ut.sh fops test_case_name
```

底层使用环境变量：

```sh
MIRAGEFS_TEST_CASE=test_case_name make test-fops
```

如果名字写错，runner 会过滤掉所有 case，表现为没有目标 case 执行。

## GDB 里变量显示 `<optimized out>`

普通构建使用 `-O2`，即使带 `-g`，部分局部变量也可能被优化掉。

可以临时用 `-O0` 重新构建：

```sh
make clean
make CFLAGS="-Wall -Wextra -g -O0 -fPIC -D_GNU_SOURCE"
```

测试二进制也可以通过覆盖率路径间接使用 `-O0`，但一般调试建议直接使用 `-O0` 普通构建。

## WSL 路径和 Windows 路径混用

项目约定在 WSL bash 中操作：

```sh
cd /home/shore/work/github/MirageFS
```

不要在 PowerShell 中用 UNC 路径执行 `make`。这会增加路径、编码、权限和换行符问题的概率。
