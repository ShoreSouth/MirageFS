#include "common_test_common.h"

#include "common/metrics/fs_metrics.h"

static int test_metrics_records_count_bytes_errors_and_latency(void)
{
    fs_metric_id_t metric_id;
    fs_metrics_token_t token;
    fs_metrics_snapshot_t snapshot;

    fs_metrics_deinit();
    fs_metrics_init(FS_METRICS_CORE);
    TEST_ASSERT_TRUE(fs_metrics_register("fops", "READ", false, &metric_id));
    fs_metrics_freeze();

    token = fs_metrics_begin(metric_id);
    TEST_ASSERT_TRUE(token.active);
    fs_metrics_end(token, true, 4096U);
    token = fs_metrics_begin(metric_id);
    fs_metrics_end(token, false, 0U);

    fs_metrics_snapshot(&snapshot);
    TEST_ASSERT_EQ_INT(1, snapshot.count);
    TEST_ASSERT_EQ_INT(2, snapshot.values[0].count);
    TEST_ASSERT_EQ_INT(1, snapshot.values[0].errors);
    TEST_ASSERT_EQ_INT(4096, snapshot.values[0].bytes);
    TEST_ASSERT_TRUE(snapshot.values[0].total_ns > 0U);
    TEST_ASSERT_TRUE(snapshot.values[0].peak_inflight >= 1U);
    TEST_ASSERT_TRUE(fs_metrics_percentile_ns(&snapshot.values[0], 95U) > 0U);

    fs_metrics_deinit();
    return 0;
}

static int test_metrics_modes_disable_or_filter_hot_path(void)
{
    fs_metric_id_t core_id;
    fs_metric_id_t detailed_id;
    fs_metrics_token_t token;
    fs_metrics_snapshot_t snapshot;

    fs_metrics_init(FS_METRICS_OFF);
    TEST_ASSERT_TRUE(fs_metrics_register("fops", "READ", false, &core_id));
    TEST_ASSERT_TRUE(
            fs_metrics_register("lsa", "READ", true, &detailed_id));
    fs_metrics_freeze();

    token = fs_metrics_begin(core_id);
    TEST_ASSERT_FALSE(token.active);

    fs_metrics_set_mode(FS_METRICS_CORE);
    token = fs_metrics_begin(detailed_id);
    TEST_ASSERT_FALSE(token.active);
    token = fs_metrics_begin(core_id);
    fs_metrics_end(token, true, 1U);

    fs_metrics_set_mode(FS_METRICS_DETAILED);
    token = fs_metrics_begin(detailed_id);
    fs_metrics_end(token, true, 2U);

    fs_metrics_snapshot(&snapshot);
    TEST_ASSERT_EQ_INT(1, snapshot.values[core_id].count);
    TEST_ASSERT_EQ_INT(1, snapshot.values[detailed_id].count);

    fs_metrics_deinit();
    return 0;
}

const test_case_t COMMON_METRICS_CASES[] = {
        TEST_CASE(
                UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_METRICS, 0x1),
                UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_METRICS, 0x1,
                           0x001),
                test_metrics_records_count_bytes_errors_and_latency,
                "Metrics 累计统计",
                "注册指标并记录成功、失败、字节数和延迟",
                "快照包含次数、错误、字节、延迟、并发峰值和分位数"),
        TEST_CASE(
                UT_LIST_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_METRICS, 0x1),
                UT_CASE_NO(UT_MOD_COMMON, TEST_COMMON_COMPONENT_METRICS, 0x1,
                           0x002),
                test_metrics_modes_disable_or_filter_hot_path,
                "Metrics 模式开关",
                "依次使用 OFF、CORE、DETAILED 模式执行核心和详细指标",
                "极速模式不采样，CORE 过滤详细指标，DETAILED 全量采样"),
};

const size_t COMMON_METRICS_CASE_COUNT =
        sizeof(COMMON_METRICS_CASES) / sizeof(COMMON_METRICS_CASES[0]);
