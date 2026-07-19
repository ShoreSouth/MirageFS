# GDB 调试

本文说明 MirageFS 在 WSL/Ubuntu 下使用 GDB 调试主程序和 UT case 的常用方法。

## 安装

```sh
sudo apt-get update
sudo apt-get install -y gdb
```

## 构建调试符号

普通构建已经包含 `-g`：

```sh
make
```

如果需要减少优化带来的断点跳转和变量被优化问题，可以临时使用 `-O0`：

```sh
make clean
make CFLAGS="-Wall -Wextra -g -O0 -fPIC -D_GNU_SOURCE"
```

覆盖率构建也会使用 `-O0 --coverage`，但调试普通问题时不建议优先使用覆盖率构建，避免插桩代码干扰阅读。

## 调试主程序

启动 GDB：

```sh
gdb --args output/bin/miragefs
```

常用命令：

```gdb
run
bt
frame 1
list
next
step
continue
print variable_name
ptype variable_name
quit
```

如果主程序后续支持参数，把参数放在 `--args` 后面：

```sh
gdb --args output/bin/miragefs --some-option value
```

## 调试单个 UT case

先构建对应模块测试：

```sh
make test-fops
```

然后带 `MIRAGEFS_TEST_CASE` 进入 GDB：

```sh
MIRAGEFS_TEST_CASE=test_fops_validate_name_rejects_bad_input \
gdb --args output/tests/bin/fops_test
```

进入 GDB 后：

```gdb
break test_fops_validate_name_rejects_bad_input
run
```

如果需要断到被测函数：

```gdb
break fops_validate_name
run
```

## 常用断点位置

按调用链调试时，可以优先在这些层级打断点：

```gdb
break main
break runtime_init
break namei_lookup
break fops_dispatch
break lsa_lookup
```

如果只调试错误码传播，常用断点：

```gdb
break fs_error_make
break fs_error_str
break fops_error
break namei_error
break lsa_error
```

具体函数名以当前源码为准，可以用 `grep` 或 `rg` 查找：

```sh
rg "fs_error_t .*namei" src/namei
```

## 查看结构体和指针

查看结构体类型：

```gdb
ptype obj_meta_t
ptype fuid_t
```

查看指针内容：

```gdb
print *meta
print *runtime
```

查看数组或 buffer：

```gdb
x/32xb buffer
x/s path
```

查看错误码十六进制：

```gdb
print/x err
```

## 调试崩溃

如果程序崩溃，先看栈：

```gdb
run
bt full
```

常见排查顺序：

1. 最顶层崩溃函数是否拿到了 `NULL` 指针。
2. 当前对象是否已经 deinit/destroy。
3. `fs_error_t` 是否被当成 `int` 或 `-1` 使用。
4. 资源释放路径是否重复释放。
5. 上层是否绕过了统一入口，例如绕过 `fops_dispatch()`。

## core dump

临时打开 core dump：

```sh
ulimit -c unlimited
```

运行程序或 UT 触发崩溃后，如果生成 core 文件：

```sh
gdb output/bin/miragefs core
```

或针对测试二进制：

```sh
gdb output/tests/bin/fops_test core
```

不同 WSL/Ubuntu 环境的 core 文件路径可能受系统配置影响，可以查看：

```sh
cat /proc/sys/kernel/core_pattern
```

## 调试建议

- 优先调试单 case，而不是整个 suite。
- 优先在模块入口打断点，例如 `fops_dispatch()`、`namei_lookup()`。
- 遇到错误码问题时，同时打印 `err` 的十六进制值和 `fs_error_str(err)`。
- 遇到资源生命周期问题时，沿着 `init/deinit`、`create/destroy`、`acquire/release` 成对检查。
- 临时调试输出应优先使用项目日志设施，问题修复后删除无意义输出。
