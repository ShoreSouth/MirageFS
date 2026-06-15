#pragma once

#include "lsa/include/lsa_api.h"

/* ============================================================
 * iterator
 * ============================================================
 */

struct lsa_dir_iter {

    int dirfd; /* directory fd */

    bool eof; /* end-of-directory reached */

    lsa_dir_cookie_t cookie; /* current directory cookie*/

    uint32_t offset; /* current directory offset */

    uint32_t bytes; /* valid bytes in buffer*/

    uint32_t buffer_size; /* buffer size */

    uint8_t *buffer; /* directory entry buffer*/
};
