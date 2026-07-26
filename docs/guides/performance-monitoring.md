# 性能统计与运行报告

MirageFS 在正常退出时可以生成本次进程生命周期的 JSON 和 HTML 性能报告。
默认输出到：

```text
reports/miragefs-<run-id>.json
reports/miragefs-<run-id>.html
```

JSON 是规范数据，HTML 是便于直接阅读的单文件报告。报告写入发生在最终指标
快照冻结之后，因此生成报告自身产生的 I/O 不计入本次统计。

## 两级生命周期

`Run` 从 Metrics 初始化完成开始，到 Runtime 退出阶段冻结最终快照结束。它
包含本次进程的全部文件系统操作。

`Filesystem Session` 从 `fs enter NAME` 开始，到 `fs leave`、切换 filesystem
或程序退出结束。同一个 filesystem 被多次进入时会形成多个 session；报告同时
保留 FSID 和当时的 namespace 名称。

递归销毁其他 filesystem 时，Runtime 会临时进入目标 namespace，因此报告中也
会出现相应的内部 session。这反映真实 Runtime 生命周期，不是重复统计。

## 指标层次

```text
filesystem  MirageFS 完成的逻辑文件操作
namei       路径解析阶段
lsa         Linux syscall 适配阶段
device      Linux 块设备物理指标（当前尚未采集）
```

报告中的 filesystem READ/WRITE 可以计算逻辑 IOPS、吞吐量、平均 I/O 大小和
读写比例，但不能当作物理磁盘 IOPS。页缓存可能让一次逻辑读取产生零次或多次
块设备 I/O。

## 配置归属

`FsConfig_t.metrics` 保存统计和报告配置，包括：

- `mode`：`OFF`、`CORE`、`DETAILED`；
- `sample_interval_ms`：内存历史采样周期；
- `history_capacity`：环形历史快照容量；
- JSON/HTML 报告开关；
- 报告输出目录。

Config 只负责配置数据。Runtime 负责启动统计、管理 session、周期采样并在退出
时生成报告；`common/metrics` 只负责通用统计原语。

单元测试构建会关闭自动报告，避免测试进程污染工作目录。生产构建默认启用
CORE 模式和退出报告。`OFF` 模式不启动历史采样器，也不生成无意义的零值性能
报告。

## 当前限制

- 正常退出、EOF 和 `msh -c` 可生成报告；`SIGKILL` 和断电无法生成最终报告。
- session 的 P50/P95/P99 可由累计 histogram bucket 做差得到；窗口 min/max
  暂不提供。
- LSA 首版详细埋点覆盖 read/write/pread/pwrite/full I/O 和 close；其他
  syscall 阶段会逐步补齐。
- 内存历史在进程退出后丢失，但最终 JSON 会带出保留窗口内的汇总历史点。

