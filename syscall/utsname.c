// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/internal/trace.h>
#include <openenclave/syscall/common.h>
#include <openenclave/syscall/raise.h>
#include <openenclave/syscall/sys/utsname.h>
#include "syscall_t.h"

int oe_uname(struct oe_utsname* buf)
{
    int ret = -1;

    if (oe_syscall_uname_ocall(&ret, (struct oe_utsname*)buf) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

done:

    return ret;
}
