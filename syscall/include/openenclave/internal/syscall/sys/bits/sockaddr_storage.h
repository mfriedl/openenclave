// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

struct __OE_SOCKADDR_STORAGE
{
    sa_family_t ss_family;
    char __ss_padding[128 - sizeof(long) - sizeof(sa_family_t)];
    unsigned long __ss_align;
};
