# 编译构建与运行

本文说明 MirageFS 在 WSL/Ubuntu 本地开发环境下的构建、清理和运行方法。未来如果引入 Docker、安装包或服务化部署，再补充对应章节。

## 环境假设

默认开发环境：

```sh
cd /home/shore/work/github/MirageFS
```

建议先确认当前位置：

```sh
pwd
uname -a
git rev-parse --show-toplevel
```

不要通过 Windows UNC 路径直接执行构建命令，例如 `\\wsl.localhost\Ubuntu\...`。构建、测试和调试都应在 WSL bash 环境中执行。

## 基础依赖

当前构建至少需要：

```sh
sudo apt-get update
sudo apt-get install -y build-essential make gcc binutils
```

UT 和覆盖率还需要：

```sh
sudo apt-get install -y libcmocka-dev pkg-config lcov
```

GDB 调试需要：

```sh
sudo apt-get install -y gdb
```

## 构建命令

构建全部模块和主程序：

```sh
make
# 或
make all
```

当前构建会按模块生成静态库，最后链接主程序：

```text
output/
  obj/       编译中间文件
  lib/       各模块静态库
  bin/       可执行文件
```

主程序路径：

```sh
output/bin/miragefs
```

查看构建变量：

```sh
make print
```

常见变量包括：

```text
ROOT_DIR
OUTPUT_DIR
OBJ_DIR
LIB_DIR
BIN_DIR
```

## 清理命令

清理构建产物、测试产物和覆盖率产物：

```sh
make clean
```

当前 `make clean` 会清理：

- `output/`
- `tests/output/`
- `src` 下残留的 `*.o`、`*.gcda`、`*.gcno`
- Python `__pycache__`

如果刚跑过覆盖率，`output/coverage/html/index.html` 也会随 `output/` 一起删除。

## 运行主程序

构建完成后运行：

```sh
output/bin/miragefs
```

如果程序后续增加参数，优先通过帮助命令查看：

```sh
output/bin/miragefs --help
```

当前项目仍处于学习和演进阶段，日常验证不建议只依赖手动启动主程序观察输出。新增功能后应优先补 UT，并通过 `make test` 或 `tools/test/run-ut.sh` 验证。

## 常用开发循环

修改代码后：

```sh
make
make test
```

需要从干净状态验证：

```sh
make clean
make
make test
```

需要同时查看覆盖率：

```sh
tools/test/run-ut.sh coverage
```

需要执行静态检查：

```sh
python3 tools/check/miragefs_lint.py --all
```

严格模式：

```sh
python3 tools/check/miragefs_lint.py --all --strict
```

## 编译选项

当前公共编译选项定义在 `build/build.mk`：

```text
-Wall
-Wextra
-g
-O2
-fPIC
-D_GNU_SOURCE
```

其中 `-g` 会保留调试符号，因此普通构建产物也可以进入 GDB。覆盖率构建会额外使用：

```text
-O0 --coverage
```

## 本地部署边界

目前“部署”只表示本地开发环境下的可执行文件生成和运行，不包含：

- Docker 镜像
- systemd 服务
- deb/rpm 安装包
- 安装到 `/usr/local/bin`
- CI/CD 发布流程

这些能力如果未来引入，应在本文追加独立章节，并说明和本地构建的差异。
