# 程序启动与命令速查

本文说明 MirageFS 主程序如何启动、启动后如何进入一个可操作的 filesystem namespace，以及当前 `msh` 支持哪些命令。

## 启动前提

默认在 WSL/Ubuntu 中从仓库根目录执行：

```sh
cd /home/shore/work/github/MirageFS
make
```

主程序生成在：

```sh
output/bin/miragefs
```

如果需要从干净状态重新体验：

```sh
make clean
make
```

`make clean` 会删除本地构建产物、测试产物和默认后端根目录 `miragefs.root/`。

## 启动模式

### 交互模式

不带参数启动会进入 Mirage Shell：

```sh
output/bin/miragefs
```

启动后会看到类似提示符：

```text
filesystems: none
msh>
```

交互模式启动时会先展示当前 runtime 已恢复/已注册的 filesystem 列表。刚启动时 runtime 已初始化，但还没有进入任何 filesystem namespace。此时 `pwd`、`ls`、`mkdir` 等文件命令会提示：

```text
msh: no filesystem selected
```

需要先创建并进入 namespace。

### 单命令模式

`-c` 可以执行一条 `msh` 命令后退出：

```sh
output/bin/miragefs -c version
```

当前只支持 `-c command` 和无参数 REPL 两种启动方式；还没有实现 `--help` 参数。命令帮助请在 REPL 内执行 `help`，或使用：

```sh
output/bin/miragefs -c help
```

注意：当前 namespace 管理表是进程内运行态。`-c` 每次都会启动一个新进程，因此更适合执行 `version`、`help` 或单条实验命令；连续文件操作建议使用交互模式。

## 第一次玩转项目

下面是一条从空项目开始的最小体验流程：

```text
msh> fs create demo
created demo
msh> fs enter demo
msh:demo:/> pwd
/
msh:demo:/> mkdir /docs
msh:demo:/> write /docs/readme.txt "hello MirageFS"
msh:demo:/> ls /docs
readme.txt
msh:demo:/> ll /docs
-rw-r--r--       14 readme.txt
msh:demo:/> cat /docs/readme.txt
hello MirageFS
msh:demo:/> stat /docs/readme.txt
type: REG
mode: -rw-r--r-- (0644)
uid: ...
gid: ...
size: 14
nlink: ...
msh:demo:/> mv /docs/readme.txt /docs/intro.txt
msh:demo:/> cd /docs
msh:demo:/docs> pwd
/docs
msh:demo:/docs> ls
intro.txt
msh:demo:/docs> rm intro.txt
msh:demo:/docs> cd /
msh:demo:/> rmdir /docs
msh:demo:/> fs destroy demo
destroy filesystem demo? type demo to confirm: demo
destroyed demo
msh> quit
```

运行时会在当前工作目录下创建默认 sysroot：

```text
miragefs.root/
```

`fs create demo` 会在 `miragefs.root/` 下创建同名后端根目录。`fsmgr_deinit()` 只释放内存索引，不主动删除磁盘上的 namespace 根目录；下一次启动时 runtime 会恢复已存在的根目录。需要真实删除整个文件系统时，使用 `fs destroy NAME`，并按提示输入同名 namespace 确认。

## 命令解析规则

当前 `msh` 解析器规则比较简单：

- 空行会被忽略。
- 支持以空白分隔参数。
- 支持单引号和双引号包裹参数，例如 `mkdir "my dir"`。
- 支持 `#` 注释；解析到 `#` 后会忽略本行剩余内容。
- 单行最大长度为 `MSH_LINE_MAX`，当前为 `1024` 字节。
- 单条命令最多 `MSH_ARG_MAX` 个参数，当前为 `32` 个。
- 引号只做成对截取，不支持转义序列。

## Shell 命令

| 命令 | 作用 |
| --- | --- |
| `help` | 打印当前命令帮助 |
| `version` | 打印 `msh` 版本 |
| `exit` | 退出 REPL |
| `quit` | 退出 REPL |

## Filesystem 命令

| 命令 | 作用 | 说明 |
| --- | --- | --- |
| `fs create NAME` | 创建 filesystem namespace | `NAME` 是 namespace 名称；建议使用不含空格和 `/` 的简单名称 |
| `fs list` | 列出 filesystem namespace | 未创建任何 namespace 时输出 `none` |
| `fs enter NAME` | 进入 filesystem namespace | 进入后 cwd 会回到该 namespace 根目录 `/` |
| `fs leave` | 离开当前 filesystem namespace | 清空当前 root/cwd 会话 |
| `fs current` | 查看当前 filesystem namespace | 未进入 namespace 时输出 `none` |
| `fs rename OLD NEW` | 重命名 filesystem namespace | 会同步重命名后端根目录；如果当前会话在 `OLD` 内，会更新当前 namespace 名称 |
| `fs destroy NAME` | 递归删除 filesystem namespace | 会删除根目录下所有子目录和对象，再销毁 namespace；执行前必须输入 `NAME` 确认 |

## 文件与目录命令

这些命令都要求先 `fs enter NAME` 进入 namespace。

| 命令 | 作用 | 说明 |
| --- | --- | --- |
| `pwd` | 显示当前目录 | 输出 runtime 保存的显示路径 |
| `cd PATH` | 切换当前目录 | 目标必须是目录 |
| `ls [PATH]` | 列出目录项 | 不带参数时默认列出 `.` |
| `ll [PATH]` | 列出目录项和属性 | 不带参数时默认列出 `.` |
| `mkdir PATH` | 创建目录 | 使用默认目录权限 |
| `rmdir PATH` | 删除空目录 | 目标必须是空目录 |
| `touch PATH` | 创建或复用普通文件 | 使用默认文件权限 |
| `mkfifo PATH` | 创建 FIFO | 对应 `mknod` 的 FIFO 形态 |
| `lookup PATH` | 查看对象 FUID 和属性 | 不跟随最后一段 symlink |
| `stat PATH` | 查看对象属性 | 输出类型、权限、uid/gid、大小、链接数 |
| `rm PATH` | 删除非目录对象 | 不跟随最后一段 symlink；删除目录应使用 `rmdir` |
| `mv OLD_PATH NEW_PATH` | 重命名或移动对象 | 当前使用 replace 语义 |
| `ln OLD NEW` | 创建硬链接 | 新名字必须不存在 |
| `ln -s TARGET LINK` | 创建符号链接 | 新名字必须不存在 |
| `symlink TARGET LINK` | 创建符号链接 | `ln -s` 的显式命令形态 |
| `readlink PATH` | 读取符号链接目标 | 输出 link target |
| `cat PATH` | 输出文件内容 | 内部使用 open/read/close |
| `write PATH TEXT` | 写入文件内容 | 文件不存在会创建；已存在会截断后写入 |
| `append PATH TEXT` | 追加文件内容 | 文件不存在会创建；已存在会追加 |
| `chmod MODE PATH` | 修改权限 | `MODE` 使用八进制，例如 `0644` |
| `chown UID GID PATH` | 修改 owner id | 受当前进程权限限制 |
| `truncate PATH SIZE` | 修改文件大小 | `SIZE` 是字节数 |
| `access PATH MASK` | 检查访问权限 | `MASK` 为 `f`、`r`、`w`、`x` 或组合如 `rw` |
| `xattr list PATH` | 列出扩展属性 | 逐行输出 xattr 名称 |
| `xattr get PATH NAME` | 读取扩展属性 | 输出属性值 |
| `xattr set PATH NAME VALUE` | 设置扩展属性 | 默认创建或覆盖由后端支持情况决定 |
| `xattr remove PATH NAME` | 删除扩展属性 | 属性不存在会返回错误 |
| `statfs [PATH]` | 查看文件系统统计信息 | 不带参数默认 `.` |
| `syncfs [PATH]` | 同步对象所在文件系统 | 不带参数默认 `.` |

## 当前未暴露为命令的能力

`gethandle` / `openhandle` 是内部适配器能力，保留为 C API 和 UT 覆盖，不作为用户可见 shell 命令。`open` / `read` / `write` / `close` 也不直接暴露 raw handle，而是通过 `cat`、`write`、`append` 提供用户友好的读写入口。

新增 shell 命令时，应同步更新本文和 `src/msh/core/msh_cmd_meta.c` 的帮助输出。

## 和构建、测试文档的关系

- 构建、清理、编译选项见 [编译构建与运行](build-run.md)。
- 自动化验证见 [UT 测试使用](unit-testing.md)。
- 想理解命令如何穿过 runtime/namei/fops/lsa，见 [读代码路线](code-reading.md) 和 [模块地图](module-map.md)。
