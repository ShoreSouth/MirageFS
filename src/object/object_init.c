#include "object/object_init.h"

#include "common/error/fs_common_sub.h"
#include "common/error/fs_sub.h"
#include "object/obj_sub.h"
#include "object/objmgr/objmgr.h"
#include "object/objpool/objpool.h"

/*
 * ============================================================
 * Object Module Bootstrap
 * ============================================================
 */

fs_error_t object_init(void)
{
    fs_error_t ret;

    /*
     * 注册 COMMON 模块的 sub-error → name 转换函数。
     */
    fs_sub_register(FS_MODULE_COMMON, (fs_sub_name_fn)fs_common_sub_name);

    /*
     * 注册 Object Layer 的 sub-error → name 转换函数。
     */
    fs_sub_register(FS_MODULE_OBJECT, (fs_sub_name_fn)obj_sub_name);

    ret = objpool_init();
    if (ret != FS_OK)
    {
        return ret;
    }

    ret = objmgr_init();
    if (ret != FS_OK)
    {
        objpool_deinit();
        return ret;
    }

    return FS_OK;
}

void object_deinit(void)
{
    objmgr_deinit();
    objpool_deinit();
}
