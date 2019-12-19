// Copyright (c) Open Enclave SDK contributors._ops
// Licensed under the MIT License.

#ifndef _OE_SYSCALL_CONSOLEFS_H
#define _OE_SYSCALL_CONSOLEFS_H

#include <openenclave/syscall/device.h>

OE_EXTERNC_BEGIN

oe_fd_t* oe_consolefs_create_file(uint32_t fileno);

OE_EXTERNC_END

#endif // _OE_SYSCALL_CONSOLEFS_H
