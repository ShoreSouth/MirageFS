#pragma once

#include "common/macros/fs_macros.h"

#include <stddef.h>
#include <stdbool.h>

/* ============================================================
 * list head
 * ============================================================ */

typedef struct fs_list_head {
    struct fs_list_head *next;
    struct fs_list_head *prev;
} fs_list_head_t;

/* ============================================================
 * container_of
 * ============================================================ */

#define FS_LIST_ENTRY(ptr, type, member) \
    FS_CONTAINER_OF(ptr, type, member)

/* ============================================================
 * static initializer
 * ============================================================ */

#define FS_LIST_HEAD_INIT(name) \
    { &(name), &(name) }

#define FS_LIST_HEAD(name) \
    fs_list_head_t name = FS_LIST_HEAD_INIT(name)

/* ============================================================
 * basic helpers
 * ============================================================ */

static inline void fs_list_init(fs_list_head_t *pHead)
{
    pHead->next = pHead;
    pHead->prev = pHead;
}

static inline bool fs_list_empty(const fs_list_head_t *pHead)
{
    return (pHead->next == pHead);
}

static inline bool fs_list_is_singular(const fs_list_head_t *pHead)
{
    return (!fs_list_empty(pHead) &&
            pHead->next == pHead->prev);
}

/* ============================================================
 * internal helpers
 * ============================================================ */

static inline void __fs_list_add(
    fs_list_head_t *pNode,
    fs_list_head_t *pPrev,
    fs_list_head_t *pNext)
{
    pNext->prev = pNode;
    pNode->next = pNext;

    pNode->prev = pPrev;
    pPrev->next = pNode;
}

static inline void __fs_list_del(
    fs_list_head_t *pPrev,
    fs_list_head_t *pNext)
{
    pNext->prev = pPrev;
    pPrev->next = pNext;
}

/* ============================================================
 * add
 * ============================================================ */

/*
 * add node right after head
 *
 * head <-> node <-> old_first
 */
static inline void fs_list_add(
    fs_list_head_t *pNode,
    fs_list_head_t *pHead)
{
    __fs_list_add(
        pNode,
        pHead,
        pHead->next);
}

/*
 * add node before head
 *
 * old_last <-> node <-> head
 */
static inline void fs_list_add_tail(
    fs_list_head_t *pNode,
    fs_list_head_t *pHead)
{
    __fs_list_add(
        pNode,
        pHead->prev,
        pHead);
}

/* ============================================================
 * delete
 * ============================================================ */

static inline void fs_list_del(fs_list_head_t *pNode)
{
    __fs_list_del(
        pNode->prev,
        pNode->next);
}

/*
 * delete node and re-init itself
 */
static inline void fs_list_del_init(fs_list_head_t *pNode)
{
    fs_list_del(pNode);
    fs_list_init(pNode);
}

/* ============================================================
 * replace
 * ============================================================ */

static inline void fs_list_replace(
    fs_list_head_t *pOld,
    fs_list_head_t *pNew)
{
    pNew->next = pOld->next;
    pNew->next->prev = pNew;

    pNew->prev = pOld->prev;
    pNew->prev->next = pNew;
}

static inline void fs_list_replace_init(
    fs_list_head_t *pOld,
    fs_list_head_t *pNew)
{
    fs_list_replace(pOld, pNew);
    fs_list_init(pOld);
}

/* ============================================================
 * move
 * ============================================================ */

static inline void fs_list_move(
    fs_list_head_t *pNode,
    fs_list_head_t *pHead)
{
    __fs_list_del(
        pNode->prev,
        pNode->next);

    fs_list_add(pNode, pHead);
}

static inline void fs_list_move_tail(
    fs_list_head_t *pNode,
    fs_list_head_t *pHead)
{
    __fs_list_del(
        pNode->prev,
        pNode->next);

    fs_list_add_tail(pNode, pHead);
}

/* ============================================================
 * first / last
 * ============================================================ */

static inline fs_list_head_t * fs_list_first(fs_list_head_t *pHead)
{
    return pHead->next;
}

static inline fs_list_head_t * fs_list_last(fs_list_head_t *pHead)
{
    return pHead->prev;
}

/* ============================================================
 * splice
 * ============================================================ */

static inline void __fs_list_splice(
    fs_list_head_t *pList,
    fs_list_head_t *pPrev,
    fs_list_head_t *pNext)
{
    fs_list_head_t *pFirst = pList->next;
    fs_list_head_t *pLast  = pList->prev;

    pFirst->prev = pPrev;
    pPrev->next  = pFirst;

    pLast->next  = pNext;
    pNext->prev  = pLast;
}

static inline void fs_list_splice(
    fs_list_head_t *pList,
    fs_list_head_t *pHead)
{
    if (!fs_list_empty(pList)) {
        __fs_list_splice(
            pList,
            pHead,
            pHead->next);
    }
}

static inline void fs_list_splice_tail(
    fs_list_head_t *pList,
    fs_list_head_t *pHead)
{
    if (!fs_list_empty(pList)) {
        __fs_list_splice(
            pList,
            pHead->prev,
            pHead);
    }
}

static inline void fs_list_splice_init(
    fs_list_head_t *pList,
    fs_list_head_t *pHead)
{
    if (!fs_list_empty(pList)) {
        fs_list_splice(pList, pHead);
        fs_list_init(pList);
    }
}

/* ============================================================
 * iteration macros
 * ============================================================ */

#define FS_LIST_FOR_EACH(pos, head) \
    for ((pos) = (head)->next; \
         (pos) != (head); \
         (pos) = (pos)->next)

#define FS_LIST_FOR_EACH_PREV(pos, head) \
    for ((pos) = (head)->prev; \
         (pos) != (head); \
         (pos) = (pos)->prev)

#define FS_LIST_FOR_EACH_SAFE(pos, n, head) \
    for ((pos) = (head)->next, \
         (n) = (pos)->next; \
         (pos) != (head); \
         (pos) = (n), \
         (n) = (pos)->next)

/* ============================================================
 * typed iteration
 * ============================================================ */

#define FS_LIST_FIRST_ENTRY(ptr, type, member) \
    FS_LIST_ENTRY((ptr)->next, type, member)

#define FS_LIST_LAST_ENTRY(ptr, type, member) \
    FS_LIST_ENTRY((ptr)->prev, type, member)

#define FS_LIST_NEXT_ENTRY(pos, member) \
    FS_LIST_ENTRY((pos)->member.next, typeof(*(pos)), member)

#define FS_LIST_PREV_ENTRY(pos, member) \
    FS_LIST_ENTRY((pos)->member.prev, typeof(*(pos)), member)

#define FS_LIST_FOR_EACH_ENTRY(pos, head, member)                 \
    for ((pos) = FS_LIST_FIRST_ENTRY(head, typeof(*(pos)), member); \
         &(pos)->member != (head);                                \
         (pos) = FS_LIST_NEXT_ENTRY(pos, member))

#define FS_LIST_FOR_EACH_ENTRY_SAFE(pos, n, head, member)         \
    for ((pos) = FS_LIST_FIRST_ENTRY(head, typeof(*(pos)), member), \
         (n) = FS_LIST_NEXT_ENTRY(pos, member);                   \
         &(pos)->member != (head);                                \
         (pos) = (n),                                             \
         (n) = FS_LIST_NEXT_ENTRY(n, member))
