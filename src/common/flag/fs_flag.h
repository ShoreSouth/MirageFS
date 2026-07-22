#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================
 * 通用操作 flag
 *
 * flag 是“附加要求”或“操作选项”的位集合。调用方可以把多个 flag
 * 用按位或组合起来，例如：
 *
 *      FS_FLAG_READ | FS_FLAG_WRITE
 *
 * 下层模块会按具体操作解释这些位。不是每个操作都支持所有 flag，
 * 例如 mkdir 不应该带 READ，lookup 可以带 NOFOLLOW。
 * ============================================================
 */

typedef uint32_t fs_flags_t;

/* 无附加选项。用于表达“按默认语义执行”。 */
#define FS_FLAG_NONE           0U

/*
 * 创建/替换约束。
 *
 * REPLACE：
 *      如果目标已存在，允许复用或替换目标。典型例子是 rename 覆盖
 *      目标，或 create 复用已有普通文件。
 *
 * EXCLUSIVE：
 *      要求目标必须不存在。典型例子是“只在文件不存在时创建”。
 *
 * 二者互斥：
 *      REPLACE 说“存在也可以”，EXCLUSIVE 说“存在就失败”，所以同一
 *      个请求里不能同时设置。
 */
#define FS_FLAG_REPLACE        (1U << 0)
#define FS_FLAG_EXCLUSIVE      (1U << 1)

/*
 * 不跟随最后一个路径分量的符号链接。
 *
 * 示例：
 *      /a/link -> target
 *
 *      不带 NOFOLLOW：操作 target。
 *      带 NOFOLLOW：操作 link 这个符号链接对象本身。
 *
 * 注意：
 *      该 flag 只约束路径最后一段，中间路径分量通常仍需要解析到目录。
 */
#define FS_FLAG_NOFOLLOW       (1U << 2)

/*
 * 写入/打开时的 I/O 行为提示。
 *
 * SYNC：
 *      请求更强的同步语义，数据更早落到后端，但通常更慢。
 *
 * DIRECT：
 *      请求 direct I/O，尽量绕过内核页缓存。是否真正生效取决于后端
 *      文件系统和打开方式。
 */
#define FS_FLAG_SYNC           (1U << 3)
#define FS_FLAG_DIRECT         (1U << 4)

/*
 * 目标对象类型约束。
 *
 * DIRECTORY：
 *      要求目标必须是目录。常用于 readdir、rmdir、打开目录等。
 *
 * REGULAR：
 *      要求目标必须是普通文件。常用于 open/read/write/truncate 等。
 *
 * 二者互斥：
 *      同一个对象不可能同时既是目录又是普通文件。
 */
#define FS_FLAG_DIRECTORY      (1U << 5)
#define FS_FLAG_REGULAR        (1U << 6)

/*
 * 文件创建/打开修饰。
 *
 * TRUNCATE：
 *      打开或创建时把普通文件截断为 0 字节。
 *
 * APPEND：
 *      写入默认追加到文件末尾。
 */
#define FS_FLAG_TRUNCATE       (1U << 7)
#define FS_FLAG_APPEND         (1U << 8)

/*
 * 文件访问方向。
 *
 * READ：
 *      打开后允许读取。
 *
 * WRITE：
 *      打开后允许写入。
 *
 * READ | WRITE：
 *      打开为读写模式。
 */
#define FS_FLAG_READ           (1U << 9)
#define FS_FLAG_WRITE          (1U << 10)

/*
 * 判断 flags 中是否包含指定 flag。
 *
 * 注意：
 *      flag 参数通常传单个位，例如 FS_FLAG_READ。传组合值时，只有
 *      组合里任意一位存在就会返回 true。
 */
static inline bool fs_flag_test(
                    fs_flags_t flags,
                    fs_flags_t flag)
{
    return (flags & flag) != 0U;
}
