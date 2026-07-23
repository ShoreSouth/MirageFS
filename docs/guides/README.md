# MirageFS 指导手册

本目录存放 MirageFS 的日常开发指导手册。文件名使用英文，正文以中文为主；命令、路径、API 名称保持原文，方便复制执行和检索。

这些文档面向本地开发环境，默认系统为 WSL/Ubuntu，仓库路径为：

```sh
/home/shore/work/github/MirageFS
```

## 阅读顺序

如果是第一次接触项目，建议按下面顺序阅读：

1. [编译构建与运行](build-run.md)
2. [模块地图](module-map.md)
3. [读代码路线](code-reading.md)
4. [UT 测试使用](unit-testing.md)
5. [开发工具](dev-tools.md)
6. [GDB 调试](gdb-debugging.md)
7. [常见问题](troubleshooting.md)

## 文档边界

`docs/guides/` 关注“怎么做”，例如怎么编译、怎么跑测试、怎么调试、从哪里开始读代码。

`docs/modules/` 关注“模块是什么”，例如模块职责、结构体、API、错误码和调用约束。

`docs/testing/ut-framework.md` 关注“UT 框架怎么设计”，而 [UT 测试使用](unit-testing.md) 关注“日常怎么跑、怎么新增 case、怎么看覆盖率”。

## 后续维护约定

- 新增模块时，同步更新 [模块地图](module-map.md) 和 [读代码路线](code-reading.md)。
- 新增构建目标、脚本或输出目录时，同步更新 [编译构建与运行](build-run.md)。
- 调整 UT 命令、覆盖率策略或 case 规范时，同步更新 [UT 测试使用](unit-testing.md)。
- 新增格式化、静态检查、调试、生成类工具时，同步更新 [开发工具](dev-tools.md) 和 `tools/README.md`。
- 调试流程中沉淀出稳定经验时，同步更新 [GDB 调试](gdb-debugging.md) 或 [常见问题](troubleshooting.md)。
