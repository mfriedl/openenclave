// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef _OE_BITS_EXT_H
#define _OE_BITS_EXT_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>

#define OE_EXT_SECTION __attribute__((section(".oeext")))

#define OE_EXT_HASH_SIZE 32

#define OE_EXT_KEY_SIZE 384

#define OE_EXT_MODULUS_SIZE 384

#define OE_EXT_EXPONENT_SIZE 4

typedef struct _oe_ext_hash
{
    /* 256-bit hash */
    uint8_t buf[OE_EXT_HASH_SIZE];
} oe_ext_hash_t;

/* 3072-bit RSA public key. */
typedef struct _oe_ext_pubkey
{
    /* 3072-bit modulus. */
    uint8_t modulus[OE_EXT_KEY_SIZE];

    /* 32-bit exponent . */
    uint8_t exponent[OE_EXT_EXPONENT_SIZE];
} oe_ext_pubkey_t;

/* A policy written to an enclave with the 'oeext update' tool. */
typedef struct _oe_ext_policy
{
    /* The public key of the signer. */
    oe_ext_pubkey_t pubkey;

    /* The signer's ID (the SHA-256 public signing key) */
    oe_ext_hash_t signer;

    /* The hash of the user-defined extension structure (EXTSTRUCT) */
    oe_ext_hash_t extid;

} oe_ext_policy_t;

typedef struct _oe_ext_signature
{
    uint8_t buf[OE_EXT_KEY_SIZE];
} oe_ext_signature_t;

/* A signature generated by 'oeext sign' tool. */
typedef struct _oe_ext_sigstruct
{
    /* The signer's ID (the SHA-256 of the public signing key). */
    oe_ext_hash_t signer;

    /* The hash of the user-defined extension structure (EXTSTRUCT) */
    oe_ext_hash_t extid;

    /* The hash of the extension. */
    oe_ext_hash_t exthash;

    /* The signature of hash(exthash|extid). */
    oe_ext_signature_t signature;
} oe_ext_sigstruct_t;

oe_result_t oe_ext_verify_signature(
    oe_ext_pubkey_t* pubkey,
    const oe_ext_hash_t* extid,
    const oe_ext_hash_t* exthash,
    const oe_ext_signature_t* signature);

oe_result_t oe_ext_ascii_to_hash(const char* ascii, oe_ext_hash_t* hash);

void oe_ext_dump_hash(const char* name, const oe_ext_hash_t* hash);

void oe_ext_dump_policy(const oe_ext_policy_t* policy);

void oe_ext_dump_sigstruct(const oe_ext_sigstruct_t* sigstruct);

oe_result_t oe_ext_save_sigstruct(
    const char* path,
    const oe_ext_sigstruct_t* sigstruct);

oe_result_t oe_ext_load_sigstruct(
    const char* path,
    oe_ext_sigstruct_t* signature);

#endif /* _OE_BITS_EXT_H */
