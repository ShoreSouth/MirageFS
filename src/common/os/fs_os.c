#include "common/os/fs_os.h"

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif

/* =========================
 * TLS 缓存
 * ========================= */

static __thread char tls_thread_name[64] = {0};

/* =========================
 * 线程 / 进程
 * ========================= */

uint64_t fs_get_tid(void)
{
    return (uint64_t)pthread_self();
}

const char *fs_get_thread_name(void)
{
    if (tls_thread_name[0] != '\0')
        return tls_thread_name;

    char raw_name[32] = {0};

    pthread_getname_np(pthread_self(), raw_name, sizeof(raw_name));

    uint64_t tid = fs_get_tid();

    if (raw_name[0] != '\0')
    {
        snprintf(tls_thread_name, sizeof(tls_thread_name), "%s-%lu", raw_name,
                 tid);
    }
    else
    {
        snprintf(tls_thread_name, sizeof(tls_thread_name), "tid-%lu", tid);
    }

    return tls_thread_name;
}

const char *fs_get_process_name(void)
{
    static char proc_name[64] = {0};

    if (proc_name[0] != '\0')
        return proc_name;

    FILE *fp = fopen("/proc/self/comm", "r");
    if (fp)
    {
        if (fgets(proc_name, sizeof(proc_name), fp))
        {
            proc_name[strcspn(proc_name, "\n")] = '\0';
        }
        fclose(fp);
    }

    if (proc_name[0] == '\0')
    {
        snprintf(proc_name, sizeof(proc_name), "miragefs");
    }

    return proc_name;
}

/* =========================
 * 时间
 * ========================= */

uint64_t fs_get_time_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec;
}

uint64_t fs_get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

uint64_t fs_get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000ULL);
}

uint64_t fs_get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64_t)ts.tv_nsec;
}

uint64_t fs_get_monotonic_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec;
}

uint64_t fs_get_monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

uint64_t fs_get_monotonic_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000ULL);
}

uint64_t fs_get_monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)(ts.tv_nsec);
}

const char *fs_time_str(void)
{
    static char buf[80];
    uint64_t ts = fs_get_time_ms();

    time_t sec = (time_t)(ts / 1000);
    int ms = (int)(ts % 1000);

    struct tm tm_info;
    localtime_r(&sec, &tm_info); // 线程安全的转换

    /* 格式化日期时间部分 */
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);

    /* 追加毫秒部分 (.xxx) */
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ".%03d", ms);

    return buf;
}
