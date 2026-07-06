#pragma once

/*
 * ============================================================
 * Object Layer sub-error table
 *
 * _(enum_name, string_name)
 *
 * 用于 fs_error_t 的 sub 字段，
 * 标识 Object Layer 内部具体执行的操作。
 * ============================================================
 */

#define OBJ_SUB_TABLE(_)         \
                                 \
    _(NONE,     "NONE")          \
                                 \
    _(INIT,     "INIT")          \
                                 \
    _(CREATE,   "CREATE")        \
    _(DELETE,   "DELETE")        \
                                 \
    _(LOOKUP,   "LOOKUP")        \
    _(ALLOC,    "ALLOC")         \
                                 \
    _(INSERT,   "INSERT")        \
    _(REMOVE,   "REMOVE")        \
    _(HANDLE,   "HANDLE")        \
                                 \
    _(GET,      "GET")           \
    _(PUT,      "PUT")           \
    _(ACQUIRE,  "ACQUIRE")       \
    _(RELEASE,  "RELEASE")       \
                                 \
    _(STATE,    "STATE")
