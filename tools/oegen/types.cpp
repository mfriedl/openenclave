// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "types.h"
#include <openenclave/host.h>

PredefinedType types[] = {
    {"char", "char", "OE_CHAR_T"},
    // { "uchar", "unsigned char", "OE_UCHAR_T" },
    {"short", "short", "OE_SHORT_T"},
    // { "ushort", "unsigned short", "OE_USHORT_T" },
    {"int", "int", "OE_INT_T"},
    // { "uint", "unsigned int", "OE_UINT_T" },
    {"long", "long", "OE_LONG_T"},
    // { "ulong", "unsigned long", "OE_ULONG_T" },
    // { "llong", "long long", "OE_LLONG_T" },
    // { "ullong", "unsigned long long", "OE_ULLONG_T" },
    // { "intn", "ssize_t", "OE_INTN_T" },
    // { "uintn", "size_t", "OE_UINTN_T" },
    {"int8", "int8_t", "OE_INT8_T"},
    {"uint8", "uint8_t", "OE_UINT8_T"},
    {"int16", "int16_t", "OE_INT16_T"},
    {"uint16", "uint16_t", "OE_UINT16_T"},
    {"int32", "int32_t", "OE_INT32_T"},
    {"uint32", "uint32_t", "OE_UINT32_T"},
    {"int64", "int64_t", "OE_INT64_T"},
    {"uint64", "uint64_t", "OE_UINT64_T"},
    {"float", "float", "OE_FLOAT_T"},
    {"double", "double", "OE_DOUBLE_T"},
    // { "real32", "float", "OE_REAL32_T" },
    // { "real64", "double", "OE_REAL64_T" },
    // { "byte", "uint8_t", "OE_BYTE_T" },
    {"bool", "bool", "OE_BOOL_T"},
    {"size_t", "size_t", "OE_SIZE_T"},
    {"ssize_t", "ssize_t", "OE_SSIZE_T"},
    {"wchar", "wchar_t", "OE_WCHAR_T"},
    {"void", "void", "OE_VOID_T"},
    {"signed char", "signed char", "OE_CHAR_T"},
    {"signed short", "signed short", "OE_SHORT_T"},
    {"signed int", "signed int", "OE_INT_T"},
    {"signed long", "signed long", "OE_ULONG_T"},
    {"unsigned char", "unsigned char", "OE_UCHAR_T"},
    {"unsigned short", "unsigned short", "OE_USHORT_T"},
    {"unsigned int", "unsigned int", "OE_UINT_T"},
    {"unsigned long", "unsigned long", "OE_ULONG_T"},
};

size_t ntypes = OE_COUNTOF(types);
