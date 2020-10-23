/*
 * CyoEncode.c - part of the CyoEncode library
 *
 * Copyright (c) 2009-2017, Graham Bull.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CyoEncode.h"

#include <assert.h>

/********************************** Shared ***********************************/

static size_t cyoBaseXXEncodeGetLength(size_t srcBytes, size_t inputBytes, size_t outputBytes)
{
    return (((srcBytes + inputBytes - 1) / inputBytes) * outputBytes) + 1; /*plus terminator*/
}

/****************************** Base16 Encoding ******************************/

/*
 * output 2 bytes for every 1 input:
 *
 *                 inputs: 1
 * outputs: 1 = ----1111 = 1111----
 *          2 = ----2222 = ----2222
 */

static const size_t BASE16_INPUT = 1;
static const size_t BASE16_OUTPUT = 2;
static const char* const BASE16_TABLE = "0123456789ABCDEF";

size_t cyoBase16EncodeGetLength(size_t srcBytes)
{
    return cyoBaseXXEncodeGetLength(srcBytes, BASE16_INPUT, BASE16_OUTPUT);
}

size_t cyoBase16EncodeA(char* dest, const void* src, size_t srcBytes)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        char* pDest = dest;
        size_t dwSrcSize = srcBytes;
        size_t dwDestSize = 0;
        unsigned char ch;

        while (dwSrcSize >= 1)
        {
            /* 1 input */
            ch = *pSrc++;
            dwSrcSize -= BASE16_INPUT;

            /* 2 outputs */
            *pDest++ = BASE16_TABLE[(ch & 0xf0) >> 4];
            *pDest++ = BASE16_TABLE[ch & 0x0f];
            dwDestSize += BASE16_OUTPUT;
        }
        *pDest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase16EncodeW(wchar_t* dest, const void* src, size_t srcBytes)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        wchar_t* pDest = dest;
        size_t dwSrcSize = srcBytes;
        size_t dwDestSize = 0;
        unsigned char ch;

        while (dwSrcSize >= 1)
        {
            /* 1 input */
            ch = *pSrc++;
            dwSrcSize -= BASE16_INPUT;

            /* 2 outputs */
            *pDest++ = BASE16_TABLE[(ch & 0xf0) >> 4];
            *pDest++ = BASE16_TABLE[ch & 0x0f];
            dwDestSize += BASE16_OUTPUT;
        }
        *pDest++ = L'\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase16EncodeBlockA(char* dest, const void* src)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        char* pDest = dest;
        unsigned char ch;

        /* 1 input */
        ch = pSrc[0];

        /* 2 outputs */
        *pDest++ = BASE16_TABLE[(ch & 0xf0) >> 4];
        *pDest++ = BASE16_TABLE[ch & 0x0f];

        return BASE16_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase16EncodeBlockW(wchar_t* dest, const void* src)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        wchar_t* pDest = dest;
        unsigned char ch;

        /* 1 input */
        ch = pSrc[0];

        /* 2 outputs */
        *pDest++ = BASE16_TABLE[(ch & 0xf0) >> 4];
        *pDest++ = BASE16_TABLE[ch & 0x0f];

        return BASE16_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

/****************************** Base32 Encoding ******************************/

/*
 * output 8 bytes for every 5 input:
 *
 *                 inputs: 1        2        3        4        5
 * outputs: 1 = ---11111 = 11111---
 *          2 = ---222XX = -----222 XX------
 *          3 = ---33333 =          --33333-
 *          4 = ---4XXXX =          -------4 XXXX----
 *          5 = ---5555X =                   ----5555 X-------
 *          6 = ---66666 =                            -66666--
 *          7 = ---77XXX =                            ------77 XXX-----
 *          8 = ---88888 =                                     ---88888
 */

static const size_t BASE32_INPUT = 5;
static const size_t BASE32_OUTPUT = 8;
static const char* const BASE32_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567=";

size_t cyoBase32EncodeGetLength(size_t srcBytes)
{
    return cyoBaseXXEncodeGetLength(srcBytes, BASE32_INPUT, BASE32_OUTPUT);
}

size_t cyoBase32EncodeA(char* dest, const void* src, size_t srcBytes)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        char* pDest = dest;
        size_t dwSrcSize = srcBytes;
        size_t dwDestSize = 0;
        size_t dwBlockSize;
        unsigned char n1, n2, n3, n4, n5, n6, n7, n8;

        while (dwSrcSize >= 1)
        {
            /* Encode inputs */
            dwBlockSize = (dwSrcSize < BASE32_INPUT ? dwSrcSize : BASE32_INPUT);
            n1 = n2 = n3 = n4 = n5 = n6 = n7 = n8 = 0;
            switch (dwBlockSize)
            {
            case 5:
                n8 = (pSrc[4] & 0x1f);
                n7 = ((pSrc[4] & 0xe0) >> 5);
            case 4:
                n7 |= ((pSrc[3] & 0x03) << 3);
                n6 = ((pSrc[3] & 0x7c) >> 2);
                n5 = ((pSrc[3] & 0x80) >> 7);
            case 3:
                n5 |= ((pSrc[2] & 0x0f) << 1);
                n4 = ((pSrc[2] & 0xf0) >> 4);
            case 2:
                n4 |= ((pSrc[1] & 0x01) << 4);
                n3 = ((pSrc[1] & 0x3e) >> 1);
                n2 = ((pSrc[1] & 0xc0) >> 6);
            case 1:
                n2 |= ((pSrc[0] & 0x07) << 2);
                n1 = ((pSrc[0] & 0xf8) >> 3);
                break;

            default:
                assert(0);
            }
            pSrc += dwBlockSize;
            dwSrcSize -= dwBlockSize;

            /* Validate */
            assert(n1 <= 31);
            assert(n2 <= 31);
            assert(n3 <= 31);
            assert(n4 <= 31);
            assert(n5 <= 31);
            assert(n6 <= 31);
            assert(n7 <= 31);
            assert(n8 <= 31);

            /* Padding */
            switch (dwBlockSize)
            {
            case 1: n3 = n4 = 32;
            case 2: n5 = 32;
            case 3: n6 = n7 = 32;
            case 4: n8 = 32;
            case 5:
                break;

            default:
                assert(0);
            }

            /* 8 outputs */
            *pDest++ = BASE32_TABLE[n1];
            *pDest++ = BASE32_TABLE[n2];
            *pDest++ = BASE32_TABLE[n3];
            *pDest++ = BASE32_TABLE[n4];
            *pDest++ = BASE32_TABLE[n5];
            *pDest++ = BASE32_TABLE[n6];
            *pDest++ = BASE32_TABLE[n7];
            *pDest++ = BASE32_TABLE[n8];
            dwDestSize += BASE32_OUTPUT;
        }
        *pDest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase32EncodeW(wchar_t* dest, const void* src, size_t srcBytes)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        wchar_t* pDest = dest;
        size_t dwSrcSize = srcBytes;
        size_t dwDestSize = 0;
        size_t dwBlockSize;
        unsigned char n1, n2, n3, n4, n5, n6, n7, n8;

        while (dwSrcSize >= 1)
        {
            /* Encode inputs */
            dwBlockSize = (dwSrcSize < BASE32_INPUT ? dwSrcSize : BASE32_INPUT);
            n1 = n2 = n3 = n4 = n5 = n6 = n7 = n8 = 0;
            switch (dwBlockSize)
            {
            case 5:
                n8 = (pSrc[4] & 0x1f);
                n7 = ((pSrc[4] & 0xe0) >> 5);
            case 4:
                n7 |= ((pSrc[3] & 0x03) << 3);
                n6 = ((pSrc[3] & 0x7c) >> 2);
                n5 = ((pSrc[3] & 0x80) >> 7);
            case 3:
                n5 |= ((pSrc[2] & 0x0f) << 1);
                n4 = ((pSrc[2] & 0xf0) >> 4);
            case 2:
                n4 |= ((pSrc[1] & 0x01) << 4);
                n3 = ((pSrc[1] & 0x3e) >> 1);
                n2 = ((pSrc[1] & 0xc0) >> 6);
            case 1:
                n2 |= ((pSrc[0] & 0x07) << 2);
                n1 = ((pSrc[0] & 0xf8) >> 3);
                break;

            default:
                assert(0);
            }
            pSrc += dwBlockSize;
            dwSrcSize -= dwBlockSize;

            /* Validate */
            assert(n1 <= 31);
            assert(n2 <= 31);
            assert(n3 <= 31);
            assert(n4 <= 31);
            assert(n5 <= 31);
            assert(n6 <= 31);
            assert(n7 <= 31);
            assert(n8 <= 31);

            /* Padding */
            switch (dwBlockSize)
            {
            case 1: n3 = n4 = 32;
            case 2: n5 = 32;
            case 3: n6 = n7 = 32;
            case 4: n8 = 32;
            case 5:
                break;

            default:
                assert(0);
            }

            /* 8 outputs */
            *pDest++ = BASE32_TABLE[n1];
            *pDest++ = BASE32_TABLE[n2];
            *pDest++ = BASE32_TABLE[n3];
            *pDest++ = BASE32_TABLE[n4];
            *pDest++ = BASE32_TABLE[n5];
            *pDest++ = BASE32_TABLE[n6];
            *pDest++ = BASE32_TABLE[n7];
            *pDest++ = BASE32_TABLE[n8];
            dwDestSize += BASE32_OUTPUT;
        }
        *pDest++ = L'\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase32EncodeBlockA(char* dest, const void* src)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        char* pDest = dest;
        unsigned char n1, n2, n3, n4, n5, n6, n7, n8;

        /* Encode inputs */
        n8 = (pSrc[4] & 0x1f);
        n7 = ((pSrc[4] & 0xe0) >> 5);
        n7 |= ((pSrc[3] & 0x03) << 3);
        n6 = ((pSrc[3] & 0x7c) >> 2);
        n5 = ((pSrc[3] & 0x80) >> 7);
        n5 |= ((pSrc[2] & 0x0f) << 1);
        n4 = ((pSrc[2] & 0xf0) >> 4);
        n4 |= ((pSrc[1] & 0x01) << 4);
        n3 = ((pSrc[1] & 0x3e) >> 1);
        n2 = ((pSrc[1] & 0xc0) >> 6);
        n2 |= ((pSrc[0] & 0x07) << 2);
        n1 = ((pSrc[0] & 0xf8) >> 3);

        /* Validate */
        assert(n1 <= 31);
        assert(n2 <= 31);
        assert(n3 <= 31);
        assert(n4 <= 31);
        assert(n5 <= 31);
        assert(n6 <= 31);
        assert(n7 <= 31);
        assert(n8 <= 31);

        /* 8 outputs */
        *pDest++ = BASE32_TABLE[n1];
        *pDest++ = BASE32_TABLE[n2];
        *pDest++ = BASE32_TABLE[n3];
        *pDest++ = BASE32_TABLE[n4];
        *pDest++ = BASE32_TABLE[n5];
        *pDest++ = BASE32_TABLE[n6];
        *pDest++ = BASE32_TABLE[n7];
        *pDest++ = BASE32_TABLE[n8];

        return BASE32_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase32EncodeBlockW(wchar_t* dest, const void* src)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        wchar_t* pDest = dest;
        unsigned char n1, n2, n3, n4, n5, n6, n7, n8;

        /* Encode inputs */
        n8 = (pSrc[4] & 0x1f);
        n7 = ((pSrc[4] & 0xe0) >> 5);
        n7 |= ((pSrc[3] & 0x03) << 3);
        n6 = ((pSrc[3] & 0x7c) >> 2);
        n5 = ((pSrc[3] & 0x80) >> 7);
        n5 |= ((pSrc[2] & 0x0f) << 1);
        n4 = ((pSrc[2] & 0xf0) >> 4);
        n4 |= ((pSrc[1] & 0x01) << 4);
        n3 = ((pSrc[1] & 0x3e) >> 1);
        n2 = ((pSrc[1] & 0xc0) >> 6);
        n2 |= ((pSrc[0] & 0x07) << 2);
        n1 = ((pSrc[0] & 0xf8) >> 3);

        /* Validate */
        assert(n1 <= 31);
        assert(n2 <= 31);
        assert(n3 <= 31);
        assert(n4 <= 31);
        assert(n5 <= 31);
        assert(n6 <= 31);
        assert(n7 <= 31);
        assert(n8 <= 31);

        /* 8 outputs */
        *pDest++ = BASE32_TABLE[n1];
        *pDest++ = BASE32_TABLE[n2];
        *pDest++ = BASE32_TABLE[n3];
        *pDest++ = BASE32_TABLE[n4];
        *pDest++ = BASE32_TABLE[n5];
        *pDest++ = BASE32_TABLE[n6];
        *pDest++ = BASE32_TABLE[n7];
        *pDest++ = BASE32_TABLE[n8];

        return BASE32_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

/****************************** Base64 Encoding ******************************/

/*
 * output 4 bytes for every 3 input:
 *
 *                 inputs: 1        2        3
 * outputs: 1 = --111111 = 111111--
 *          2 = --22XXXX = ------22 XXXX----
 *          3 = --3333XX =          ----3333 XX------
 *          4 = --444444 =                   --444444
 */

static const size_t BASE64_INPUT = 3;
static const size_t BASE64_OUTPUT = 4;
static const char* const BASE64_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

size_t cyoBase64EncodeGetLength(size_t srcBytes)
{
    return cyoBaseXXEncodeGetLength(srcBytes, BASE64_INPUT, BASE64_OUTPUT);
}

size_t cyoBase64EncodeA(char* dest, const void* src, size_t srcBytes)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        char* pDest = dest;
        size_t dwSrcSize = srcBytes;
        size_t dwDestSize = 0;
        size_t dwBlockSize = 0;
        unsigned char n1, n2, n3, n4;

        while (dwSrcSize >= 1)
        {
            /* Encode inputs */
            dwBlockSize = (dwSrcSize < BASE64_INPUT ? dwSrcSize : BASE64_INPUT);
            n1 = n2 = n3 = n4 = 0;
            switch (dwBlockSize)
            {
            case 3:
                n4 = (pSrc[2] & 0x3f);
                n3 = ((pSrc[2] & 0xc0) >> 6);
            case 2:
                n3 |= ((pSrc[1] & 0x0f) << 2);
                n2 = ((pSrc[1] & 0xf0) >> 4);
            case 1:
                n2 |= ((pSrc[0] & 0x03) << 4);
                n1 = ((pSrc[0] & 0xfc) >> 2);
                break;

            default:
                assert(0);
            }
            pSrc += dwBlockSize;
            dwSrcSize -= dwBlockSize;

            /* Validate */
            assert(n1 <= 63);
            assert(n2 <= 63);
            assert(n3 <= 63);
            assert(n4 <= 63);

            /* Padding */
            switch (dwBlockSize)
            {
            case 1: n3 = 64;
            case 2: n4 = 64;
            case 3:
                break;

            default:
                assert(0);
            }

            /* 4 outputs */
            *pDest++ = BASE64_TABLE[n1];
            *pDest++ = BASE64_TABLE[n2];
            *pDest++ = BASE64_TABLE[n3];
            *pDest++ = BASE64_TABLE[n4];
            dwDestSize += BASE64_OUTPUT;
        }
        *pDest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase64EncodeW(wchar_t* dest, const void* src, size_t srcBytes)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        wchar_t* pDest = dest;
        size_t dwSrcSize = srcBytes;
        size_t dwDestSize = 0;
        size_t dwBlockSize = 0;
        unsigned char n1, n2, n3, n4;

        while (dwSrcSize >= 1)
        {
            /* Encode inputs */
            dwBlockSize = (dwSrcSize < BASE64_INPUT ? dwSrcSize : BASE64_INPUT);
            n1 = n2 = n3 = n4 = 0;
            switch (dwBlockSize)
            {
            case 3:
                n4 = (pSrc[2] & 0x3f);
                n3 = ((pSrc[2] & 0xc0) >> 6);
            case 2:
                n3 |= ((pSrc[1] & 0x0f) << 2);
                n2 = ((pSrc[1] & 0xf0) >> 4);
            case 1:
                n2 |= ((pSrc[0] & 0x03) << 4);
                n1 = ((pSrc[0] & 0xfc) >> 2);
                break;

            default:
                assert(0);
            }
            pSrc += dwBlockSize;
            dwSrcSize -= dwBlockSize;

            /* Validate */
            assert(n1 <= 63);
            assert(n2 <= 63);
            assert(n3 <= 63);
            assert(n4 <= 63);

            /* Padding */
            switch (dwBlockSize)
            {
            case 1: n3 = 64;
            case 2: n4 = 64;
            case 3:
                break;

            default:
                assert(0);
            }

            /* 4 outputs */
            *pDest++ = BASE64_TABLE[n1];
            *pDest++ = BASE64_TABLE[n2];
            *pDest++ = BASE64_TABLE[n3];
            *pDest++ = BASE64_TABLE[n4];
            dwDestSize += BASE64_OUTPUT;
        }
        *pDest++ = L'\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase64EncodeBlockA(char* dest, const void* src)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        char* pDest = dest;
        unsigned char n1, n2, n3, n4;

        /* Encode inputs */
        n4 = (pSrc[2] & 0x3f);
        n3 = ((pSrc[2] & 0xc0) >> 6);
        n3 |= ((pSrc[1] & 0x0f) << 2);
        n2 = ((pSrc[1] & 0xf0) >> 4);
        n2 |= ((pSrc[0] & 0x03) << 4);
        n1 = ((pSrc[0] & 0xfc) >> 2);

        /* Validate */
        assert(n1 <= 63);
        assert(n2 <= 63);
        assert(n3 <= 63);
        assert(n4 <= 63);

        /* 4 outputs */
        *pDest++ = BASE64_TABLE[n1];
        *pDest++ = BASE64_TABLE[n2];
        *pDest++ = BASE64_TABLE[n3];
        *pDest++ = BASE64_TABLE[n4];

        return BASE64_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase64EncodeBlockW(wchar_t* dest, const void* src)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        wchar_t* pDest = dest;
        unsigned char n1, n2, n3, n4;

        /* Encode inputs */
        n4 = (pSrc[2] & 0x3f);
        n3 = ((pSrc[2] & 0xc0) >> 6);
        n3 |= ((pSrc[1] & 0x0f) << 2);
        n2 = ((pSrc[1] & 0xf0) >> 4);
        n2 |= ((pSrc[0] & 0x03) << 4);
        n1 = ((pSrc[0] & 0xfc) >> 2);

        /* Validate */
        assert(n1 <= 63);
        assert(n2 <= 63);
        assert(n3 <= 63);
        assert(n4 <= 63);

        /* 4 outputs */
        *pDest++ = BASE64_TABLE[n1];
        *pDest++ = BASE64_TABLE[n2];
        *pDest++ = BASE64_TABLE[n3];
        *pDest++ = BASE64_TABLE[n4];

        return BASE64_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

/****************************** Base85 Encoding ******************************/

/*
 * output 5 bytes for every 4 input
 */

static const size_t BASE85_INPUT = 4;
static const size_t BASE85_OUTPUT = 5;

#define FOLD_ZERO 1 /*output 'z' instead of '!!!!!'*/
//#define FOLD_SPACES 1 /*output 'y' instead of 4 spaces*/

size_t cyoBase85EncodeGetLength(size_t srcBytes)
{
    return cyoBaseXXEncodeGetLength(srcBytes, BASE85_INPUT, BASE85_OUTPUT);
}

size_t cyoBase85EncodeA(char* dest, const void* src, size_t srcBytes)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        char* pDest = dest;
        size_t dwSrcSize = srcBytes;
        size_t dwDestSize = 0;
        unsigned int n;
        unsigned char n1, n2, n3, n4, n5;
        size_t i;
        int padding;

        while (dwSrcSize >= 1)
        {
            /* Encode inputs */
            n = 0;
            padding = 0;
            for (i = 0; i < BASE85_INPUT; ++i)
            {
                n <<= 8;
                if (dwSrcSize >= 1)
                {
                    n |= *pSrc++;
                    --dwSrcSize;
                }
                else
                    ++padding;
            }
#if FOLD_ZERO
            if (n == 0)
            {
                *pDest++ = 'z';
                dwDestSize += 1;
                continue;
            }
#endif

            n5 = (unsigned char)(n % 85);
            n = (n - n5) / 85;
            n4 = (unsigned char)(n % 85);
            n = (n - n4) / 85;
            n3 = (unsigned char)(n % 85);
            n = (n - n3) / 85;
            n2 = (unsigned char)(n % 85);
            n = (n - n2) / 85;
            n1 = (unsigned char)n;

            /* Validate */
            assert(n1 < 85);
            assert(n2 < 85);
            assert(n3 < 85);
            assert(n4 < 85);
            assert(n5 < 85);

            /* Outputs */
            if (padding == 0)
            {
                /* 5 outputs */
                *pDest++ = (n1 + 33);
                *pDest++ = (n2 + 33);
                *pDest++ = (n3 + 33);
                *pDest++ = (n4 + 33);
                *pDest++ = (n5 + 33);
                dwDestSize += BASE85_OUTPUT;
            }
            else
            {
                /* 1-4 outputs */
                assert(1 <= padding && padding <= 4);
                *pDest++ = (n1 + 33);
                if (padding < 4)
                    *pDest++ = (n2 + 33);
                if (padding < 3)
                    *pDest++ = (n3 + 33);
                if (padding < 2)
                    *pDest++ = (n4 + 33);
                if (padding < 1)
                    *pDest++ = (n5 + 33);
                dwDestSize += (BASE85_OUTPUT - padding);
            }
        }
        *pDest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase85EncodeW(wchar_t* dest, const void* src, size_t srcBytes)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        wchar_t* pDest = dest;
        size_t dwSrcSize = srcBytes;
        size_t dwDestSize = 0;
        unsigned int n;
        unsigned char n1, n2, n3, n4, n5;
        size_t i;
        int padding;

        while (dwSrcSize >= 1)
        {
            /* Encode inputs */
            n = 0;
            padding = 0;
            for (i = 0; i < BASE85_INPUT; ++i)
            {
                n <<= 8;
                if (dwSrcSize >= 1)
                {
                    n |= *pSrc++;
                    --dwSrcSize;
                }
                else
                    ++padding;
            }
#if FOLD_ZERO
            if (n == 0)
            {
                *pDest++ = 'z';
                dwDestSize += 1;
                continue;
            }
#endif

            n5 = (unsigned char)(n % 85);
            n = (n - n5) / 85;
            n4 = (unsigned char)(n % 85);
            n = (n - n4) / 85;
            n3 = (unsigned char)(n % 85);
            n = (n - n3) / 85;
            n2 = (unsigned char)(n % 85);
            n = (n - n2) / 85;
            n1 = (unsigned char)n;

            /* Validate */
            assert(n1 < 85);
            assert(n2 < 85);
            assert(n3 < 85);
            assert(n4 < 85);
            assert(n5 < 85);

            /* Outputs */
            if (padding == 0)
            {
                /* 5 outputs */
                *pDest++ = (n1 + 33);
                *pDest++ = (n2 + 33);
                *pDest++ = (n3 + 33);
                *pDest++ = (n4 + 33);
                *pDest++ = (n5 + 33);
                dwDestSize += BASE85_OUTPUT;
            }
            else
            {
                /* 1-4 outputs */
                assert(1 <= padding && padding <= 4);
                *pDest++ = (n1 + 33);
                if (padding < 4)
                    *pDest++ = (n2 + 33);
                if (padding < 3)
                    *pDest++ = (n3 + 33);
                if (padding < 2)
                    *pDest++ = (n4 + 33);
                if (padding < 1)
                    *pDest++ = (n5 + 33);
                dwDestSize += (BASE85_OUTPUT - padding);
            }
        }
        *pDest++ = L'\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase85EncodeBlockA(char* dest, const void* src)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        char* pDest = dest;
        unsigned int n = 0;
        unsigned char n1, n2, n3, n4, n5;
        size_t i;

        /* Encode inputs */
        for (i = 0; i < BASE85_INPUT; ++i)
        {
            n <<= 8;
            n |= *pSrc++;
        }
#if FOLD_ZERO
        if (n == 0)
        {
            *pDest++ = 'z';
            return 1;
        }
#endif
        n5 = (unsigned char)(n % 85);
        n = (n - n5) / 85;
        n4 = (unsigned char)(n % 85);
        n = (n - n4) / 85;
        n3 = (unsigned char)(n % 85);
        n = (n - n3) / 85;
        n2 = (unsigned char)(n % 85);
        n = (n - n2) / 85;
        n1 = (unsigned char)n;

        /* Validate */
        assert(n1 < 85);
        assert(n2 < 85);
        assert(n3 < 85);
        assert(n4 < 85);
        assert(n5 < 85);

        /* Outputs */
        *pDest++ = (n1 + 33);
        *pDest++ = (n2 + 33);
        *pDest++ = (n3 + 33);
        *pDest++ = (n4 + 33);
        *pDest++ = (n5 + 33);

        return BASE85_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase85EncodeBlockW(wchar_t* dest, const void* src)
{
    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        wchar_t* pDest = dest;
        unsigned int n = 0;
        unsigned char n1, n2, n3, n4, n5;
        size_t i;

        /* Encode inputs */
        for (i = 0; i < BASE85_INPUT; ++i)
        {
            n <<= 8;
            n |= *pSrc++;
        }
#if FOLD_ZERO
        if (n == 0)
        {
            *pDest++ = 'z';
            return 1;
        }
#endif
        n5 = (unsigned char)(n % 85);
        n = (n - n5) / 85;
        n4 = (unsigned char)(n % 85);
        n = (n - n4) / 85;
        n3 = (unsigned char)(n % 85);
        n = (n - n3) / 85;
        n2 = (unsigned char)(n % 85);
        n = (n - n2) / 85;
        n1 = (unsigned char)n;

        /* Validate */
        assert(n1 < 85);
        assert(n2 < 85);
        assert(n3 < 85);
        assert(n4 < 85);
        assert(n5 < 85);

        /* Outputs */
        *pDest++ = (n1 + 33);
        *pDest++ = (n2 + 33);
        *pDest++ = (n3 + 33);
        *pDest++ = (n4 + 33);
        *pDest++ = (n5 + 33);

        return BASE85_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}
