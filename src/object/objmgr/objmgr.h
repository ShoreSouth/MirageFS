#pragma once

#include <stdbool.h>

#include "common/fs_common.h"
#include "object/objmeta/objmeta.h"

/*
 * ============================================================
 * objmgr
 *
 * object lifecycle manager
 * ============================================================
 */

/*
 * ============================================================
 * init
 * ============================================================
 */

/*
 * initialize object manager
 */
int32_t objmgr_init(void);

/*
 * cleanup object manager
 */
void objmgr_deinit(void);

/*
 * ============================================================
 * object lifecycle
 * ============================================================
 */

/*
 * create object
 */
int32_t objmgr_create(
                const obj_meta_t *meta);

/*
 * delete object
 */
int32_t objmgr_delete(
                fuid_t fuid);

/*
 * ============================================================
 * object lookup
 * ============================================================
 */

/*
 * lookup object
 *
 * return:
 *      object metadata pointer
 *
 * note:
 *      returned pointer is managed by objmgr
 */
obj_meta_t *objmgr_lookup(
                fuid_t fuid);

/*
 * check object exists
 */
bool objmgr_exists(
                fuid_t fuid);

/*
 * ============================================================
 * reference count
 * ============================================================
 */

/*
 * acquire reference
 *
 * return:
 *      FS_OK
 *      FS_ERR_NOT_FOUND
 *      FS_ERR_BUSY
 */
int32_t objmgr_get(
                fuid_t fuid);

/*
 * release reference
 */
int32_t objmgr_put(
                fuid_t fuid);

/*
 * get current reference count
 */
int32_t objmgr_refcnt(
                fuid_t fuid);

/*
 * ============================================================
 * object state
 * ============================================================
 */

/*
 * get object state
 */
uint32_t objmgr_state(
                fuid_t fuid);

/*
 * set object state
 */
int32_t objmgr_set_state(
                fuid_t fuid,
                uint32_t state);

/*
 * ============================================================
 * statistics
 * ============================================================
 */

/*
 * current object count
 */
uint32_t objmgr_count(void);