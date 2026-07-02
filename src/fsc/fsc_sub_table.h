#pragma once

/*
 * ============================================================
 * FSC sub-error table
 *
 * _(enum_name, string_name)
 *
 * ============================================================
 */

#define FSC_SUB_TABLE(_)        \
                                \
    _(NONE,      "NONE")        \
                                \
    _(INIT,      "INIT")        \
                                \
    _(CREATE,    "CREATE")      \
    _(DESTROY,   "DESTROY")     \
                                \
    _(LOOKUP,    "LOOKUP")      \
                                \
    _(INSERT,    "INSERT")      \
    _(REMOVE,    "REMOVE")      \
                                \
    _(ALLOC,     "ALLOC")       \
    _(FREE,      "FREE")        \
                                \
    _(FSID,      "FSID")        \
    _(NAMESPACE, "NAMESPACE")   \
    _(NSPOOL,    "NSPOOL")      \
    _(FSTABLE,   "FSTABLE")     \
    _(FSMGR,     "FSMGR")       \
                                \
    _(STATE,     "STATE")
