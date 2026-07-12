#include "runtime/internal/runtime_error.h"

fs_error_t runtime_error(runtime_sub_t sub, int err)
{
    return FS_ERR(FS_SEV_ERROR,
                  FS_MODULE_RUNTIME,
                  sub,
                  (uint8_t)err);
}
