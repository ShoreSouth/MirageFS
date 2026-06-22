#pragma once

/*
 * ============================================================
 * module table
 *
 * _(enum_name, string_name)
 *
 * ============================================================
 */

#define FS_MODULE_TABLE(_)          \
                                    \
    _(COMMON,  "COMMON")            \
                                    \
    _(OBJECT,    "OBJECT")          \
                                    \
    _(STORAGE, "STORAGE")           \
    _(CACHE,   "CACHE")             \
                                    \
    _(FSMGR,   "FSMGR")             \
    _(VFS,     "VFS")               \
                                    \
    _(LSA,     "LSA")               \
    _(SERVER,  "SERVER")            \
    _(CLI,     "CLI")
