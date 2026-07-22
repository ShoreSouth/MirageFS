#include "object_test_common.h"

obj_handle_t test_object_make_handle_with_seed(uint8_t seed)
{
    obj_handle_t handle;

    memset(&handle, 0, sizeof(handle));
    handle.mount_id = (uint64_t)(88U + seed);
    handle.type = (uint16_t)(1U + seed);
    handle.len = 4;
    handle.data[0] = (uint8_t)(0x11U + seed);
    handle.data[1] = (uint8_t)(0x22U + seed);
    handle.data[2] = (uint8_t)(0x33U + seed);
    handle.data[3] = (uint8_t)(0x44U + seed);
    return handle;
}

obj_handle_t test_object_make_handle(void)
{
    return test_object_make_handle_with_seed(0);
}

fuid_t test_object_make_fuid(ObjectId_t objectid)
{
    return fuid_make(17, objectid, 1, FUID_TYPE_FILE);
}
