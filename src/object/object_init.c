#include "object/object_init.h"

#include "common/error/fs_sub.h"
#include "object/obj_sub.h"
#include "object/objmgr/objmgr.h"

/*
 * ============================================================
 * Object Module Bootstrap
 * ============================================================
 */

int object_init(void)
{
    /*
     * 注册 Object Layer 的 sub-error → name 转换函数。
     *
     * 此后 fs_error_str() 即可正确解析
     * FS_MODULE_OBJECT 的 sub 字段。
     */
    fs_sub_register(FS_MODULE_OBJECT,
                    (fs_sub_name_fn)obj_sub_name);

    return objmgr_init();
}

void object_deinit(void)
{
    objmgr_deinit();
}
