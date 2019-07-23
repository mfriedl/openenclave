// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pem.h"
#include <assert.h>
#include <openenclave/bits/safecrt.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/internal/pem.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
#include "bcrypt.h"

static inline oe_result_t _check_pem_args(const void* pem_data, size_t pem_size)
{
    oe_result_t result = OE_OK;

    /* Must have pem_size-1 non-zero characters followed by zero-terminator */
    if (!pem_data || !pem_size || pem_size > OE_INT_MAX ||
        strnlen((const char*)pem_data, pem_size) != pem_size - 1)
        OE_RAISE(OE_INVALID_PARAMETER);
done:
    return result;
}

oe_result_t oe_bcrypt_pem_to_der(
    const uint8_t* pem_data,
    size_t pem_size,
    BYTE** der_data,
    DWORD* der_size)
{
    oe_result_t result = OE_UNEXPECTED;
    BYTE* der_local = NULL;
    DWORD der_local_size = 0;
    BOOL success = FALSE;

    if (der_data)
        *der_data = NULL;

    if (der_size)
        *der_size = 0;

    /* Check parameters */
    OE_CHECK(_check_pem_args(pem_data, pem_size));
    if (!der_data || !der_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Subtract 1, since BCrypt doesn't count the null terminator.*/
    pem_size--;

    /* Size the expected DER output */
    success = CryptStringToBinaryA(
        (const char*)pem_data,
        (DWORD)pem_size,
        CRYPT_STRING_BASE64HEADER,
        NULL,
        &der_local_size,
        NULL,
        NULL);

    /* Should return true and set der_local_size when given a null buffer */
    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptStringToBinaryA failed (err=%#x)\n",
            GetLastError());

    der_local = (BYTE*)malloc(der_local_size);
    if (der_local == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    /* Convert from PEM format to DER format and strip header/footer */
    success = CryptStringToBinaryA(
        (const char*)pem_data,
        (DWORD)pem_size,
        CRYPT_STRING_BASE64HEADER,
        der_local,
        &der_local_size,
        NULL,
        NULL);

    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptStringToBinaryA failed (err=%#x)\n",
            GetLastError());

    *der_size = der_local_size;
    *der_data = der_local;
    der_local = NULL;

    result = OE_OK;

done:
    if (der_local)
    {
        oe_secure_zero_fill(der_local, der_local_size);
        free(der_local);
        der_local_size = 0;
    }

    return result;
}

oe_result_t oe_bcrypt_der_to_pem(
    const BYTE* der_data,
    DWORD der_size,
    uint8_t** pem_data,
    size_t* pem_size)
{
    oe_result_t result = OE_UNEXPECTED;
    uint8_t* pem_local = NULL;
    DWORD pem_local_size = 0;
    BOOL success = FALSE;

    if (pem_data)
        *pem_data = NULL;

    if (pem_size)
        *pem_size = 0;

    /* Check parameters */
    if (!der_data || der_size == 0 || der_size > MAXDWORD || !pem_data ||
        !pem_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    success = CryptBinaryToStringA(
        der_data,
        der_size,
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR,
        NULL,
        &pem_local_size);

    /* Should return true and set pem_local_size when given a null buffer.
     * Resulting pem_local_size includes null terminator. */
    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptBinaryToStringA failed (err=%#x)\n",
            GetLastError());

    /* Need to allocate and write the PEM header/footer manually because
     * BCrypt only supports the cert/CRL/CSR headers.
     * The size also accounts for LF characters at the end of each line. */
    const DWORD pem_headers_size =
        OE_PEM_BEGIN_PUBLIC_KEY_LEN + OE_PEM_END_PUBLIC_KEY_LEN + 2;
    OE_CHECK(
        oe_safe_add_u32(pem_local_size, pem_headers_size, &pem_local_size));

    pem_local = (uint8_t*)malloc(pem_local_size);
    if (pem_local == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    {
        /* Write the begin public key header */
        uint8_t* pos = pem_local;
        DWORD size_left = pem_local_size;

        OE_CHECK(oe_memcpy_s(
            pos,
            size_left,
            OE_PEM_BEGIN_PUBLIC_KEY,
            OE_PEM_BEGIN_PUBLIC_KEY_LEN));
        pos += OE_PEM_BEGIN_PUBLIC_KEY_LEN;
        size_left -= OE_PEM_BEGIN_PUBLIC_KEY_LEN;

        /* Write a LF character */
        *pos++ = 0x0A;
        size_left--;

        /* Write the encoded key data */
        DWORD written_size = size_left;
        success = CryptBinaryToStringA(
            der_data,
            der_size,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR,
            pos,
            &written_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptBinaryToStringA failed (err=%#x)\n",
                GetLastError());

        /* Assert null-terminator at end of string.
         * This method overwrites it to extend the string for the end footer */
        pos += written_size;
        assert(*pos == '\0');
        size_left -= written_size;

        /* Write the end public key footer */
        OE_CHECK(oe_memcpy_s(
            pos, size_left, OE_PEM_END_PUBLIC_KEY, OE_PEM_END_PUBLIC_KEY_LEN));
        pos += OE_PEM_END_PUBLIC_KEY_LEN;
        size_left -= OE_PEM_END_PUBLIC_KEY_LEN;

        /* Add LF and null-terminator */
        OE_CHECK(size_left == 2 ? OE_OK : OE_UNEXPECTED);
        *pos++ = 0x0A;
        *pos++ = 0x00;
    }

    *pem_data = (uint8_t*)pem_local;
    *pem_size = (size_t)pem_local_size;
    result = OE_OK;
    pem_local = NULL;

done:
    if (pem_local)
    {
        oe_secure_zero_fill(pem_local, pem_local_size);
        free(pem_local);
        pem_local_size = 0;
    }

    return result;
}

oe_result_t oe_get_next_pem_cert(
    const void** pem_read_pos,
    size_t* pem_bytes_remaining,
    char** pem_cert,
    size_t* pem_cert_size)
{
    oe_result_t result = OE_UNEXPECTED;
    const char* cert_begin = NULL;
    const char* cert_end = NULL;
    char* found_pem = NULL;
    size_t found_pem_size = 0;
    const void* pem_data_end = NULL;

    if (pem_cert)
        *pem_cert = NULL;

    if (pem_cert_size)
        *pem_cert_size = 0;

    /* Check parameters */
    OE_CHECK(_check_pem_args(*pem_read_pos, *pem_bytes_remaining));
    if (!pem_cert || !pem_cert_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    cert_begin = (unsigned char*)strstr(
        (const char*)*pem_read_pos, OE_PEM_BEGIN_CERTIFICATE);

    if (!cert_begin || *cert_begin == '\0')
        return (OE_NOT_FOUND);

    cert_end = (unsigned char*)strstr(
        (const char*)*pem_read_pos, OE_PEM_END_CERTIFICATE);

    if (!cert_end || *cert_begin == '\0' || cert_end <= cert_begin)
        return (OE_NOT_FOUND);

    /* PEM cert footer must have at least newline or null-terminator in pem_data
     * buffer to be valid. */
    assert(sizeof(void*) == sizeof(uint64_t));
    OE_CHECK(oe_safe_add_u64(
        (uint64_t)cert_end,
        OE_PEM_END_CERTIFICATE_LEN + 1,
        (uint64_t*)&cert_end));
    OE_CHECK(oe_safe_sub_u64(
        (uint64_t)cert_end, (uint64_t)cert_begin, &found_pem_size));

    found_pem = malloc(found_pem_size);
    if (!found_pem)
        OE_RAISE(OE_OUT_OF_MEMORY);

    OE_CHECK(
        oe_memcpy_s(found_pem, found_pem_size, cert_begin, found_pem_size - 1));
    found_pem[found_pem_size - 1] = '\0';

    /* Note that pem_cert offset may not equal the starting read_pos,
     * so we infer the remaining_size from the new read_pos. */
    OE_CHECK(oe_safe_add_u64(
        (uint64_t)*pem_read_pos,
        *pem_bytes_remaining,
        (uint64_t*)&pem_data_end));
    OE_CHECK(oe_safe_sub_u64(
        (uint64_t)pem_data_end, (uint64_t)cert_end, pem_bytes_remaining));

    *pem_read_pos = cert_end;
    *pem_cert = found_pem;
    *pem_cert_size = found_pem_size;

    result = OE_OK;

done:
    if (result != OE_OK && found_pem)
        free(found_pem);

    return result;
}