/*
 * CyoDecode.c - part of the CyoEncode library
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

#include "CyoDecode.h"

#include <assert.h>

/********************************** Shared ***********************************/

static int cyoBaseXXValidateA(const char* src, size_t srcChars, size_t inputBytes,
    size_t maxPadding, unsigned char maxValue, const unsigned char table[])
{
    /*
     * returns 0 if the source is a valid baseXX encoding
     */

    if (!src)
        return -1; /*ERROR - NULL pointer*/

    if (srcChars % inputBytes != 0)
        return -1; /*ERROR - extra characters*/

    /* check the bytes */
    for (; srcChars >= 1; --srcChars, ++src)
    {
        unsigned char ch = *src;
        if ((ch >= 0x80) || (table[ch] > maxValue))
            break;
    }

    /* check any padding */
    for (; 1 <= srcChars && srcChars <= maxPadding; --srcChars, ++src)
    {
        unsigned char ch = *src;
        if ((ch >= 0x80) || (table[ch] != maxValue + 1))
            break;
    }

    /* if srcChars isn't zero then the encoded string isn't valid */
    if (srcChars != 0)
        return -2; /*ERROR - invalid baseXX character*/

    /* OK */
    return 0;
}

static int cyoBaseXXValidateW(const wchar_t* src, size_t srcChars, size_t inputBytes,
    size_t maxPadding, unsigned char maxValue, const unsigned char table[])
{
    /*
    * returns 0 if the source is a valid baseXX encoding
    */

    if (!src)
        return -1; /*ERROR - NULL pointer*/

    if (srcChars % inputBytes != 0)
        return -1; /*ERROR - extra characters*/

    /* check the bytes */
    for (; srcChars >= 1; --srcChars, ++src)
    {
        wchar_t ch = *src;
        if ((ch >= 0x80) || (table[ch] > maxValue))
            break;
    }

    /* check any padding */
    for (; 1 <= srcChars && srcChars <= maxPadding; --srcChars, ++src)
    {
        wchar_t ch = *src;
        if ((ch >= 0x80) || (table[ch] != maxValue + 1))
            break;
    }

    /* if srcChars isn't zero then the encoded string isn't valid */
    if (srcChars != 0)
        return -2; /*ERROR - invalid baseXX character*/

    /* OK */
    return 0;
}

static size_t cyoBaseXXDecodeGetLength(size_t srcChars, size_t inputBytes,
    size_t outputBytes)
{
    if (srcChars % inputBytes != 0)
        return 0; /*ERROR - extra characters*/

    /* OK */
    return (((srcChars + inputBytes - 1) / inputBytes) * outputBytes);
}

/****************************** Base16 Decoding ******************************/

/*
 * output 1 byte for every 2 input:
 *
 *               outputs: 1
 * inputs: 1 = ----1111 = 1111----
 *         2 = ----2222 = ----2222
 */

static const size_t BASE16_INPUT = 2;
static const size_t BASE16_OUTPUT = 1;
static const size_t BASE16_MAX_PADDING = 0;
static const unsigned char BASE16_MAX_VALUE = 15;
static const unsigned char BASE16_TABLE[ 0x80 ] = {
    /*00-07*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*08-0f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*10-17*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*18-1f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*20-27*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*28-2f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*30-37*/ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /*8 = '0'-'7'*/
    /*38-3f*/ 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*2 = '8'-'9'*/
    /*40-47*/ 0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF, /*6 = 'A'-'F'*/
    /*48-4f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*50-57*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*58-5f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*60-67*/ 0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF, /*6 = 'a'-'f' (same as 'A'-'F')*/
    /*68-6f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*70-77*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*78-7f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

int cyoBase16ValidateA(const char* src, size_t srcChars)
{
    return cyoBaseXXValidateA(src, srcChars, BASE16_INPUT,
        BASE16_MAX_PADDING, BASE16_MAX_VALUE, BASE16_TABLE);
}

int cyoBase16ValidateW(const wchar_t* src, size_t srcChars)
{
    return cyoBaseXXValidateW(src, srcChars, BASE16_INPUT,
        BASE16_MAX_PADDING, BASE16_MAX_VALUE, BASE16_TABLE);
}

size_t cyoBase16DecodeGetLength(size_t srcChars)
{
    return cyoBaseXXDecodeGetLength(srcChars, BASE16_INPUT, BASE16_OUTPUT);
}

size_t cyoBase16DecodeA(void* dest, const char* src, size_t srcChars)
{
    if (dest && src && (srcChars % BASE16_INPUT == 0))
    {
        const char* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = srcChars;
        size_t dwDestSize = 0;
        unsigned char in1, in2;

        while (dwSrcSize >= 1)
        {
            /* 2 inputs */
            in1 = *pSrc++;
            in2 = *pSrc++;
            dwSrcSize -= BASE16_INPUT;

            /* Validate ASCII */
            if (in1 >= 0x80 || in2 >= 0x80)
                return 0; /*ERROR - invalid base16 character*/

            /* Convert ASCII to base16 */
            in1 = BASE16_TABLE[in1];
            in2 = BASE16_TABLE[in2];

            /* Validate base16 */
            if (in1 > BASE16_MAX_VALUE || in2 > BASE16_MAX_VALUE)
                return 0; /*ERROR - invalid base16 character*/

            /* 1 output */
            *pDest++ = ((in1 << 4) | in2);
            dwDestSize += BASE16_OUTPUT;
        }

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or srcChars isn't a multiple of 2*/
}

size_t cyoBase16DecodeW(void* dest, const wchar_t* src, size_t srcChars)
{
    if (dest && src && (srcChars % BASE16_INPUT == 0))
    {
        const wchar_t* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = srcChars;
        size_t dwDestSize = 0;
        wchar_t in1, in2;

        while (dwSrcSize >= 1)
        {
            /* 2 inputs */
            in1 = *pSrc++;
            in2 = *pSrc++;
            dwSrcSize -= BASE16_INPUT;

            /* Validate ASCII */
            if (in1 >= 0x80 || in2 >= 0x80)
                return 0; /*ERROR - invalid base16 character*/

            /* Convert ASCII to base16 */
            in1 = BASE16_TABLE[in1];
            in2 = BASE16_TABLE[in2];

            /* Validate base16 */
            if (in1 > BASE16_MAX_VALUE || in2 > BASE16_MAX_VALUE)
                return 0; /*ERROR - invalid base16 character*/

            /* 1 output */
            *pDest++ = (unsigned char)((in1 << 4) | in2);
            dwDestSize += BASE16_OUTPUT;
        }

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or srcChars isn't a multiple of 2*/
}

size_t cyoBase16DecodeBlockA(void* dest, const char* src)
{
    if (dest && src)
    {
        const char* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        unsigned char in1, in2;

        /* 2 inputs */
        in1 = *pSrc++;
        in2 = *pSrc++;

        /* Validate ASCII */
        if (in1 >= 0x80 || in2 >= 0x80)
            return 0; /*ERROR - invalid base16 character*/

        /* Convert ASCII to base16 */
        in1 = BASE16_TABLE[in1];
        in2 = BASE16_TABLE[in2];

        /* Validate base16 */
        if (in1 > BASE16_MAX_VALUE || in2 > BASE16_MAX_VALUE)
            return 0; /*ERROR - invalid base16 character*/

        /* 1 output */
        *pDest++ = ((in1 << 4) | in2);

        return BASE16_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase16DecodeBlockW(void* dest, const wchar_t* src)
{
    if (dest && src)
    {
        const wchar_t* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        wchar_t in1, in2;

        /* 2 inputs */
        in1 = *pSrc++;
        in2 = *pSrc++;

        /* Validate ASCII */
        if (in1 >= 0x80 || in2 >= 0x80)
            return 0; /*ERROR - invalid base16 character*/

        /* Convert ASCII to base16 */
        in1 = BASE16_TABLE[in1];
        in2 = BASE16_TABLE[in2];

        /* Validate base16 */
        if (in1 > BASE16_MAX_VALUE || in2 > BASE16_MAX_VALUE)
            return 0; /*ERROR - invalid base16 character*/

        /* 1 output */
        *pDest++ = (unsigned char)((in1 << 4) | in2);

        return BASE16_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

/****************************** Base32 Decoding ******************************/

/*
 * output 5 bytes for every 8 input:
 *
 *               outputs: 1        2        3        4        5
 * inputs: 1 = ---11111 = 11111---
 *         2 = ---222XX = -----222 XX------
 *         3 = ---33333 =          --33333-
 *         4 = ---4XXXX =          -------4 XXXX----
 *         5 = ---5555X =                   ----5555 X-------
 *         6 = ---66666 =                            -66666--
 *         7 = ---77XXX =                            ------77 XXX-----
 *         8 = ---88888 =                                     ---88888
 */

static const size_t BASE32_INPUT = 8;
static const size_t BASE32_OUTPUT = 5;
static const size_t BASE32_MAX_PADDING = 6;
static const unsigned char BASE32_MAX_VALUE = 31;
static const unsigned char BASE32_TABLE[ 0x80 ] = {
    /*00-07*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*08-0f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*10-17*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*18-1f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*20-27*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*28-2f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*30-37*/ 0xFF, 0xFF, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /*6 = '2'-'7'*/
    /*38-3f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0xFF, 0xFF, /*1 = '='*/
    /*40-47*/ 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /*7 = 'A'-'G'*/
    /*48-4f*/ 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, /*8 = 'H'-'O'*/
    /*50-57*/ 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, /*8 = 'P'-'W'*/
    /*58-5f*/ 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*3 = 'X'-'Z'*/
    /*60-67*/ 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /*7 = 'a'-'g' (same as 'A'-'G')*/
    /*68-6f*/ 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, /*8 = 'h'-'o' (same as 'H'-'O')*/
    /*70-77*/ 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, /*8 = 'p'-'w' (same as 'P'-'W')*/
    /*78-7f*/ 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  /*3 = 'x'-'z' (same as 'X'-'Z')*/
};

int cyoBase32ValidateA(const char* src, size_t srcChars)
{
    return cyoBaseXXValidateA(src, srcChars, BASE32_INPUT,
        BASE32_MAX_PADDING, BASE32_MAX_VALUE, BASE32_TABLE);
}

int cyoBase32ValidateW(const wchar_t* src, size_t srcChars)
{
    return cyoBaseXXValidateW(src, srcChars, BASE32_INPUT,
        BASE32_MAX_PADDING, BASE32_MAX_VALUE, BASE32_TABLE);
}

size_t cyoBase32DecodeGetLength(size_t srcChars)
{
    return cyoBaseXXDecodeGetLength(srcChars, BASE32_INPUT, BASE32_OUTPUT);
}

size_t cyoBase32DecodeA(void* dest, const char* src, size_t srcChars)
{
    if (dest && src && (srcChars % BASE32_INPUT == 0))
    {
        const char* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = srcChars;
        size_t dwDestSize = 0;
        unsigned char in1, in2, in3, in4, in5, in6, in7, in8;

        while (dwSrcSize >= 1)
        {
            /* 8 inputs */
            in1 = *pSrc++;
            in2 = *pSrc++;
            in3 = *pSrc++;
            in4 = *pSrc++;
            in5 = *pSrc++;
            in6 = *pSrc++;
            in7 = *pSrc++;
            in8 = *pSrc++;
            dwSrcSize -= BASE32_INPUT;

            /* Validate ASCII */
            if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80
                || in5 >= 0x80 || in6 >= 0x80 || in7 >= 0x80 || in8 >= 0x80)
                return 0; /*ERROR - invalid base32 character*/
            if (in1 == '=' || in2 == '=')
                return 0; /*ERROR - invalid padding*/
            if (dwSrcSize == 0)
            {
                if (in3 == '=' && in4 != '=')
                    return 0; /*ERROR - invalid padding*/
                if (in4 == '=' && in5 != '=')
                    return 0; /*ERROR - invalid padding*/
                if (in5 == '=' && in6 != '=')
                    return 0; /*ERROR - invalid padding*/
                if (in6 == '=' && in7 != '=')
                    return 0; /*ERROR - invalid padding*/
                if (in7 == '=' && in8 != '=')
                    return 0; /*ERROR - invalid padding*/
            }
            else
            {
                if (in3 == '=' || in4 == '=' || in5 == '=' || in6 == '=' || in7 == '=' || in8 == '=')
                    return 0; /*ERROR - invalid padding*/
            }

            /* Convert ASCII to base32 */
            in1 = BASE32_TABLE[in1];
            in2 = BASE32_TABLE[in2];
            in3 = BASE32_TABLE[in3];
            in4 = BASE32_TABLE[in4];
            in5 = BASE32_TABLE[in5];
            in6 = BASE32_TABLE[in6];
            in7 = BASE32_TABLE[in7];
            in8 = BASE32_TABLE[in8];

            /* Validate base32 */
            if (in1 > BASE32_MAX_VALUE || in2 > BASE32_MAX_VALUE)
                return 0; /*ERROR - invalid base32 character*/
            /*the following can be padding*/
            if (in3 > BASE32_MAX_VALUE + 1 || in4 > BASE32_MAX_VALUE + 1
                || in5 > BASE32_MAX_VALUE + 1 || in6 > BASE32_MAX_VALUE + 1
                || in7 > BASE32_MAX_VALUE + 1 || in8 > BASE32_MAX_VALUE + 1)
                return 0; /*ERROR - invalid base32 character*/

            /* 5 outputs */
            *pDest++ = ((in1 & 0x1f) << 3) | ((in2 & 0x1c) >> 2);
            *pDest++ = ((in2 & 0x03) << 6) | ((in3 & 0x1f) << 1) | ((in4 & 0x10) >> 4);
            *pDest++ = ((in4 & 0x0f) << 4) | ((in5 & 0x1e) >> 1);
            *pDest++ = ((in5 & 0x01) << 7) | ((in6 & 0x1f) << 2) | ((in7 & 0x18) >> 3);
            *pDest++ = ((in7 & 0x07) << 5) | (in8 & 0x1f);
            dwDestSize += BASE32_OUTPUT;

            /* Padding */
            if (in8 == BASE32_MAX_VALUE + 1)
            {
                --dwDestSize;
                assert((in7 == BASE32_MAX_VALUE + 1 && in6 == BASE32_MAX_VALUE + 1)
                    || (in7 != BASE32_MAX_VALUE + 1));
                if (in6 == BASE32_MAX_VALUE + 1)
                {
                    --dwDestSize;
                    if (in5 == BASE32_MAX_VALUE + 1)
                    {
                        --dwDestSize;
                        assert((in4 == BASE32_MAX_VALUE + 1 && in3 == BASE32_MAX_VALUE + 1)
                            || (in4 != BASE32_MAX_VALUE + 1));
                        if (in3 == BASE32_MAX_VALUE + 1)
                        {
                            --dwDestSize;
                        }
                    }
                }
            }
        }

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or srcChars isn't a multiple of 8*/
}

size_t cyoBase32DecodeW(void* dest, const wchar_t* src, size_t srcChars)
{
    if (dest && src && (srcChars % BASE32_INPUT == 0))
    {
        const wchar_t* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = srcChars;
        size_t dwDestSize = 0;
        wchar_t in1, in2, in3, in4, in5, in6, in7, in8;

        while (dwSrcSize >= 1)
        {
            /* 8 inputs */
            in1 = *pSrc++;
            in2 = *pSrc++;
            in3 = *pSrc++;
            in4 = *pSrc++;
            in5 = *pSrc++;
            in6 = *pSrc++;
            in7 = *pSrc++;
            in8 = *pSrc++;
            dwSrcSize -= BASE32_INPUT;

            /* Validate ASCII */
            if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80
                || in5 >= 0x80 || in6 >= 0x80 || in7 >= 0x80 || in8 >= 0x80)
                return 0; /*ERROR - invalid base32 character*/
            if (in1 == '=' || in2 == '=')
                return 0; /*ERROR - invalid padding*/
            if (dwSrcSize == 0)
            {
                if (in3 == '=' && in4 != '=')
                    return 0; /*ERROR - invalid padding*/
                if (in4 == '=' && in5 != '=')
                    return 0; /*ERROR - invalid padding*/
                if (in5 == '=' && in6 != '=')
                    return 0; /*ERROR - invalid padding*/
                if (in6 == '=' && in7 != '=')
                    return 0; /*ERROR - invalid padding*/
                if (in7 == '=' && in8 != '=')
                    return 0; /*ERROR - invalid padding*/
            }
            else
            {
                if (in3 == '=' || in4 == '=' || in5 == '=' || in6 == '=' || in7 == '=' || in8 == '=')
                    return 0; /*ERROR - invalid padding*/
            }

            /* Convert ASCII to base32 */
            in1 = BASE32_TABLE[in1];
            in2 = BASE32_TABLE[in2];
            in3 = BASE32_TABLE[in3];
            in4 = BASE32_TABLE[in4];
            in5 = BASE32_TABLE[in5];
            in6 = BASE32_TABLE[in6];
            in7 = BASE32_TABLE[in7];
            in8 = BASE32_TABLE[in8];

            /* Validate base32 */
            if (in1 > BASE32_MAX_VALUE || in2 > BASE32_MAX_VALUE)
                return 0; /*ERROR - invalid base32 character*/
            /*the following can be padding*/
            if (in3 > BASE32_MAX_VALUE + 1 || in4 > BASE32_MAX_VALUE + 1
                || in5 > BASE32_MAX_VALUE + 1 || in6 > BASE32_MAX_VALUE + 1
                || in7 > BASE32_MAX_VALUE + 1 || in8 > BASE32_MAX_VALUE + 1)
                return 0; /*ERROR - invalid base32 character*/

            /* 5 outputs */
            *pDest++ = ((in1 & 0x1f) << 3) | ((in2 & 0x1c) >> 2);
            *pDest++ = ((in2 & 0x03) << 6) | ((in3 & 0x1f) << 1) | ((in4 & 0x10) >> 4);
            *pDest++ = ((in4 & 0x0f) << 4) | ((in5 & 0x1e) >> 1);
            *pDest++ = ((in5 & 0x01) << 7) | ((in6 & 0x1f) << 2) | ((in7 & 0x18) >> 3);
            *pDest++ = ((in7 & 0x07) << 5) | (in8 & 0x1f);
            dwDestSize += BASE32_OUTPUT;

            /* Padding */
            if (in8 == BASE32_MAX_VALUE + 1)
            {
                --dwDestSize;
                assert((in7 == BASE32_MAX_VALUE + 1 && in6 == BASE32_MAX_VALUE + 1)
                    || (in7 != BASE32_MAX_VALUE + 1));
                if (in6 == BASE32_MAX_VALUE + 1)
                {
                    --dwDestSize;
                    if (in5 == BASE32_MAX_VALUE + 1)
                    {
                        --dwDestSize;
                        assert((in4 == BASE32_MAX_VALUE + 1 && in3 == BASE32_MAX_VALUE + 1)
                            || (in4 != BASE32_MAX_VALUE + 1));
                        if (in3 == BASE32_MAX_VALUE + 1)
                        {
                            --dwDestSize;
                        }
                    }
                }
            }
        }

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or srcChars isn't a multiple of 8*/
}

size_t cyoBase32DecodeBlockA(void* dest, const char* src)
{
    if (dest && src)
    {
        const char* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        unsigned char in1, in2, in3, in4, in5, in6, in7, in8;

        /* 8 inputs */
        in1 = *pSrc++;
        in2 = *pSrc++;
        in3 = *pSrc++;
        in4 = *pSrc++;
        in5 = *pSrc++;
        in6 = *pSrc++;
        in7 = *pSrc++;
        in8 = *pSrc++;

        /* Validate ASCII */
        if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80
            || in5 >= 0x80 || in6 >= 0x80 || in7 >= 0x80 || in8 >= 0x80)
            return 0; /*ERROR - invalid base32 character*/

        /* Convert ASCII to base32 */
        in1 = BASE32_TABLE[in1];
        in2 = BASE32_TABLE[in2];
        in3 = BASE32_TABLE[in3];
        in4 = BASE32_TABLE[in4];
        in5 = BASE32_TABLE[in5];
        in6 = BASE32_TABLE[in6];
        in7 = BASE32_TABLE[in7];
        in8 = BASE32_TABLE[in8];

        /* Validate base32 */
        if (in1 > BASE32_MAX_VALUE || in2 > BASE32_MAX_VALUE)
            return 0; /*ERROR - invalid base32 character*/
        /*the following can be padding*/
        if (in3 > BASE32_MAX_VALUE + 1 || in4 > BASE32_MAX_VALUE + 1
            || in5 > BASE32_MAX_VALUE + 1 || in6 > BASE32_MAX_VALUE + 1
            || in7 > BASE32_MAX_VALUE + 1 || in8 > BASE32_MAX_VALUE + 1)
            return 0; /*ERROR - invalid base32 character*/

        /* 5 outputs */
        *pDest++ = ((in1 & 0x1f) << 3) | ((in2 & 0x1c) >> 2);
        *pDest++ = ((in2 & 0x03) << 6) | ((in3 & 0x1f) << 1) | ((in4 & 0x10) >> 4);
        *pDest++ = ((in4 & 0x0f) << 4) | ((in5 & 0x1e) >> 1);
        *pDest++ = ((in5 & 0x01) << 7) | ((in6 & 0x1f) << 2) | ((in7 & 0x18) >> 3);
        *pDest++ = ((in7 & 0x07) << 5) | (in8 & 0x1f);

        return BASE32_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase32DecodeBlockW(void* dest, const wchar_t* src)
{
    if (dest && src)
    {
        const wchar_t* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        wchar_t in1, in2, in3, in4, in5, in6, in7, in8;

        /* 8 inputs */
        in1 = *pSrc++;
        in2 = *pSrc++;
        in3 = *pSrc++;
        in4 = *pSrc++;
        in5 = *pSrc++;
        in6 = *pSrc++;
        in7 = *pSrc++;
        in8 = *pSrc++;

        /* Validate ASCII */
        if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80
            || in5 >= 0x80 || in6 >= 0x80 || in7 >= 0x80 || in8 >= 0x80)
            return 0; /*ERROR - invalid base32 character*/

        /* Convert ASCII to base32 */
        in1 = BASE32_TABLE[in1];
        in2 = BASE32_TABLE[in2];
        in3 = BASE32_TABLE[in3];
        in4 = BASE32_TABLE[in4];
        in5 = BASE32_TABLE[in5];
        in6 = BASE32_TABLE[in6];
        in7 = BASE32_TABLE[in7];
        in8 = BASE32_TABLE[in8];

        /* Validate base32 */
        if (in1 > BASE32_MAX_VALUE || in2 > BASE32_MAX_VALUE)
            return 0; /*ERROR - invalid base32 character*/
        /*the following can be padding*/
        if (in3 > BASE32_MAX_VALUE + 1 || in4 > BASE32_MAX_VALUE + 1
            || in5 > BASE32_MAX_VALUE + 1 || in6 > BASE32_MAX_VALUE + 1
            || in7 > BASE32_MAX_VALUE + 1 || in8 > BASE32_MAX_VALUE + 1)
            return 0; /*ERROR - invalid base32 character*/

        /* 5 outputs */
        *pDest++ = ((in1 & 0x1f) << 3) | ((in2 & 0x1c) >> 2);
        *pDest++ = ((in2 & 0x03) << 6) | ((in3 & 0x1f) << 1) | ((in4 & 0x10) >> 4);
        *pDest++ = ((in4 & 0x0f) << 4) | ((in5 & 0x1e) >> 1);
        *pDest++ = ((in5 & 0x01) << 7) | ((in6 & 0x1f) << 2) | ((in7 & 0x18) >> 3);
        *pDest++ = ((in7 & 0x07) << 5) | (in8 & 0x1f);

        return BASE32_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

/****************************** Base64 Decoding ******************************/

/*
 * output 3 bytes for every 4 input:
 *
 *               outputs: 1        2        3
 * inputs: 1 = --111111 = 111111--
 *         2 = --22XXXX = ------22 XXXX----
 *         3 = --3333XX =          ----3333 XX------
 *         4 = --444444 =                   --444444
 */

static const size_t BASE64_INPUT = 4;
static const size_t BASE64_OUTPUT = 3;
static const size_t BASE64_MAX_PADDING = 2;
static const unsigned char BASE64_MAX_VALUE = 63;
static const unsigned char BASE64_TABLE[ 0x80 ] = {
    /*00-07*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*08-0f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*10-17*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*18-1f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*20-27*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*28-2f*/ 0xFF, 0xFF, 0xFF, 0x3e, 0xFF, 0xFF, 0xFF, 0x3f, /*2 = '+' and '/'*/
    /*30-37*/ 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, /*8 = '0'-'7'*/
    /*38-3f*/ 0x3c, 0x3d, 0xFF, 0xFF, 0xFF, 0x40, 0xFF, 0xFF, /*2 = '8'-'9' and '='*/
    /*40-47*/ 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /*7 = 'A'-'G'*/
    /*48-4f*/ 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, /*8 = 'H'-'O'*/
    /*50-57*/ 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, /*8 = 'P'-'W'*/
    /*58-5f*/ 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*3 = 'X'-'Z'*/
    /*60-67*/ 0xFF, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, /*7 = 'a'-'g'*/
    /*68-6f*/ 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, /*8 = 'h'-'o'*/
    /*70-77*/ 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, /*8 = 'p'-'w'*/
    /*78-7f*/ 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  /*3 = 'x'-'z'*/
};

int cyoBase64ValidateA(const char* src, size_t srcChars)
{
    return cyoBaseXXValidateA(src, srcChars, BASE64_INPUT,
        BASE64_MAX_PADDING, BASE64_MAX_VALUE, BASE64_TABLE);
}

int cyoBase64ValidateW(const wchar_t* src, size_t srcChars)
{
    return cyoBaseXXValidateW(src, srcChars, BASE64_INPUT,
        BASE64_MAX_PADDING, BASE64_MAX_VALUE, BASE64_TABLE);
}

size_t cyoBase64DecodeGetLength(size_t srcChars)
{
    return cyoBaseXXDecodeGetLength(srcChars, BASE64_INPUT, BASE64_OUTPUT);
}

size_t cyoBase64DecodeA(void* dest, const char* src, size_t srcChars)
{
    if (dest && src && (srcChars % BASE64_INPUT == 0))
    {
        const char* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = srcChars;
        size_t dwDestSize = 0;
        unsigned char in1, in2, in3, in4;

        while (dwSrcSize >= 1)
        {
            /* 4 inputs */
            in1 = *pSrc++;
            in2 = *pSrc++;
            in3 = *pSrc++;
            in4 = *pSrc++;
            dwSrcSize -= BASE64_INPUT;

            /* Validate ASCII */
            if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80)
                return 0; /*ERROR - invalid base64 character*/
            if (in1 == '=' || in2 == '=')
                return 0; /*ERROR - invalid padding*/
            if (dwSrcSize == 0)
            {
                if (in3 == '=' && in4 != '=')
                    return 0; /*ERROR - invalid padding*/
            }
            else
            {
                if (in3 == '=' || in4 == '=')
                    return 0; /*ERROR - invalid padding*/
            }

            /* Convert ASCII to base64 */
            in1 = BASE64_TABLE[in1];
            in2 = BASE64_TABLE[in2];
            in3 = BASE64_TABLE[in3];
            in4 = BASE64_TABLE[in4];

            /* Validate base64 */
            if (in1 > BASE64_MAX_VALUE || in2 > BASE64_MAX_VALUE)
                return 0; /*ERROR - invalid base64 character*/
            /*the following can be padding*/
            if (in3 > BASE64_MAX_VALUE + 1 || in4 > BASE64_MAX_VALUE + 1)
                return 0; /*ERROR - invalid base64 character*/

            /* 3 outputs */
            *pDest++ = ((in1 & 0x3f) << 2) | ((in2 & 0x30) >> 4);
            *pDest++ = ((in2 & 0x0f) << 4) | ((in3 & 0x3c) >> 2);
            *pDest++ = ((in3 & 0x03) << 6) | (in4 & 0x3f);
            dwDestSize += BASE64_OUTPUT;

            /* Padding */
            if (in4 == BASE64_MAX_VALUE + 1)
            {
                --dwDestSize;
                if (in3 == BASE64_MAX_VALUE + 1)
                {
                    --dwDestSize;
                }
            }
        }

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or srcChars isn't a multiple of 4*/
}

size_t cyoBase64DecodeW(void* dest, const wchar_t* src, size_t srcChars)
{
    if (dest && src && (srcChars % BASE64_INPUT == 0))
    {
        const wchar_t* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = srcChars;
        size_t dwDestSize = 0;
        wchar_t in1, in2, in3, in4;

        while (dwSrcSize >= 1)
        {
            /* 4 inputs */
            in1 = *pSrc++;
            in2 = *pSrc++;
            in3 = *pSrc++;
            in4 = *pSrc++;
            dwSrcSize -= BASE64_INPUT;

            /* Validate ASCII */
            if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80)
                return 0; /*ERROR - invalid base64 character*/
            if (in1 == '=' || in2 == '=')
                return 0; /*ERROR - invalid padding*/
            if (dwSrcSize == 0)
            {
                if (in3 == '=' && in4 != '=')
                    return 0; /*ERROR - invalid padding*/
            }
            else
            {
                if (in3 == '=' || in4 == '=')
                    return 0; /*ERROR - invalid padding*/
            }

            /* Convert ASCII to base64 */
            in1 = BASE64_TABLE[in1];
            in2 = BASE64_TABLE[in2];
            in3 = BASE64_TABLE[in3];
            in4 = BASE64_TABLE[in4];

            /* Validate base64 */
            if (in1 > BASE64_MAX_VALUE || in2 > BASE64_MAX_VALUE)
                return 0; /*ERROR - invalid base64 character*/
            /*the following can be padding*/
            if (in3 > BASE64_MAX_VALUE + 1 || in4 > BASE64_MAX_VALUE + 1)
                return 0; /*ERROR - invalid base64 character*/

            /* 3 outputs */
            *pDest++ = ((in1 & 0x3f) << 2) | ((in2 & 0x30) >> 4);
            *pDest++ = ((in2 & 0x0f) << 4) | ((in3 & 0x3c) >> 2);
            *pDest++ = ((in3 & 0x03) << 6) | (in4 & 0x3f);
            dwDestSize += BASE64_OUTPUT;

            /* Padding */
            if (in4 == BASE64_MAX_VALUE + 1)
            {
                --dwDestSize;
                if (in3 == BASE64_MAX_VALUE + 1)
                {
                    --dwDestSize;
                }
            }
        }

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or srcChars isn't a multiple of 4*/
}

size_t cyoBase64DecodeBlockA(void* dest, const char* src)
{
    if (dest && src)
    {
        const char* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        unsigned char in1, in2, in3, in4;

        /* 4 inputs */
        in1 = *pSrc++;
        in2 = *pSrc++;
        in3 = *pSrc++;
        in4 = *pSrc++;

        /* Validate ASCII */
        if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80)
            return 0; /*ERROR - invalid base64 character*/

        /* Convert ASCII to base64 */
        in1 = BASE64_TABLE[in1];
        in2 = BASE64_TABLE[in2];
        in3 = BASE64_TABLE[in3];
        in4 = BASE64_TABLE[in4];

        /* Validate base64 */
        if (in1 > BASE64_MAX_VALUE || in2 > BASE64_MAX_VALUE)
            return 0; /*ERROR - invalid base64 character*/
        /*the following can be padding*/
        if (in3 > BASE64_MAX_VALUE + 1 || in4 > BASE64_MAX_VALUE + 1)
            return 0; /*ERROR - invalid base64 character*/

        /* 3 outputs */
        *pDest++ = ((in1 & 0x3f) << 2) | ((in2 & 0x30) >> 4);
        *pDest++ = ((in2 & 0x0f) << 4) | ((in3 & 0x3c) >> 2);
        *pDest++ = ((in3 & 0x03) << 6) | (in4 & 0x3f);

        return BASE64_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase64DecodeBlockW(void* dest, const wchar_t* src)
{
    if (dest && src)
    {
        const wchar_t* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        wchar_t in1, in2, in3, in4;

        /* 4 inputs */
        in1 = *pSrc++;
        in2 = *pSrc++;
        in3 = *pSrc++;
        in4 = *pSrc++;

        /* Validate ASCII */
        if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80)
            return 0; /*ERROR - invalid base64 character*/

        /* Convert ASCII to base64 */
        in1 = BASE64_TABLE[in1];
        in2 = BASE64_TABLE[in2];
        in3 = BASE64_TABLE[in3];
        in4 = BASE64_TABLE[in4];

        /* Validate base64 */
        if (in1 > BASE64_MAX_VALUE || in2 > BASE64_MAX_VALUE)
            return 0; /*ERROR - invalid base64 character*/
        /*the following can be padding*/
        if (in3 > BASE64_MAX_VALUE + 1 || in4 > BASE64_MAX_VALUE + 1)
            return 0; /*ERROR - invalid base64 character*/

        /* 3 outputs */
        *pDest++ = ((in1 & 0x3f) << 2) | ((in2 & 0x30) >> 4);
        *pDest++ = ((in2 & 0x0f) << 4) | ((in3 & 0x3c) >> 2);
        *pDest++ = ((in3 & 0x03) << 6) | (in4 & 0x3f);

        return BASE64_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

/****************************** Base85 Decoding ******************************/

/*
 * output 4 bytes for every 5 input
 */

static const size_t BASE85_INPUT = 5;
static const size_t BASE85_OUTPUT = 4;

#define FOLD_ZERO 1 /*output 'z' instead of '!!!!!'*/
//#define FOLD_SPACES 1 /*output 'y' instead of 4 spaces*/

int cyoBase85ValidateA(const char* src, size_t srcChars)
{
    const char* pSrc = src;
    size_t dwSrcSize = srcChars;
    unsigned char in1, in2, in3, in4, in5;

    while (dwSrcSize >= 1)
    {
#if FOLD_ZERO
        if (*pSrc == 'z')
        {
            --dwSrcSize;
            continue;
        }
#endif

        /* 5 inputs */
        assert(dwSrcSize >= 1);
        in1 = (*pSrc++ - 33);
        --dwSrcSize;
        in2 = in3 = in4 = in5 = 0;
        if (dwSrcSize >= 1)
        {
            in2 = (*pSrc++ - 33);
            --dwSrcSize;
        }
        if (dwSrcSize >= 1)
        {
            in3 = (*pSrc++ - 33);
            --dwSrcSize;
        }
        if (dwSrcSize >= 1)
        {
            in4 = (*pSrc++ - 33);
            --dwSrcSize;
        }
        if (dwSrcSize >= 1)
        {
            in5 = (*pSrc++ - 33);
            --dwSrcSize;
        }

        /* Validate */
        if (in1 >= 85 || in2 >= 85 || in3 >= 85 || in4 >= 85 || in5 >= 85)
            return -2; /*ERROR - invalid base85 character*/
    }

    /* OK */
    return 0;
}

int cyoBase85ValidateW(const wchar_t* src, size_t srcChars)
{
    const wchar_t* pSrc = src;
    size_t dwSrcSize = srcChars;
    wchar_t in1, in2, in3, in4, in5;

    while (dwSrcSize >= 1)
    {
#if FOLD_ZERO
        if (*pSrc == L'z')
        {
            --dwSrcSize;
            continue;
        }
#endif

        /* 5 inputs */
        assert(dwSrcSize >= 1);
        in1 = (*pSrc++ - 33);
        --dwSrcSize;
        in2 = in3 = in4 = in5 = 0;
        if (dwSrcSize >= 1)
        {
            in2 = (*pSrc++ - 33);
            --dwSrcSize;
        }
        if (dwSrcSize >= 1)
        {
            in3 = (*pSrc++ - 33);
            --dwSrcSize;
        }
        if (dwSrcSize >= 1)
        {
            in4 = (*pSrc++ - 33);
            --dwSrcSize;
        }
        if (dwSrcSize >= 1)
        {
            in5 = (*pSrc++ - 33);
            --dwSrcSize;
        }

        /* Validate */
        if (in1 >= 85 || in2 >= 85 || in3 >= 85 || in4 >= 85 || in5 >= 85)
            return -2; /*ERROR - invalid base85 character*/
    }

    /* OK */
    return 0;
}

size_t cyoBase85DecodeGetLength(size_t srcChars)
{
    return cyoBaseXXDecodeGetLength(srcChars, BASE85_INPUT, BASE85_OUTPUT);
}

size_t cyoBase85DecodeA(void* dest, const char* src, size_t srcChars)
{
    if (dest && src && (srcChars % BASE85_INPUT == 0))
    {
        const char* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = srcChars;
        size_t dwDestSize = 0;
        unsigned char in1, in2, in3, in4, in5;
        unsigned int out;

        while (dwSrcSize >= 1)
        {
#if FOLD_ZERO
            if (*pSrc == 'z')
            {
                ++pSrc;
                *pDest++ = 0;
                dwDestSize += BASE85_OUTPUT;
                continue;
            }
#endif

            /* 5 inputs */
            in1 = (*pSrc++ - 33);
            in2 = (*pSrc++ - 33);
            in3 = (*pSrc++ - 33);
            in4 = (*pSrc++ - 33);
            in5 = (*pSrc++ - 33);
            dwSrcSize -= BASE85_INPUT;

            /* Validate */
            if (in1 >= 85 || in2 >= 85 || in3 >= 85 || in4 >= 85 || in5 >= 85)
                return 0; /*ERROR - invalid base85 character*/

            /* Output */
            out = in1;
            out *= 85;
            out |= in2;
            out *= 85;
            out |= in3;
            out *= 85;
            out |= in4;
            out *= 85;
            out |= in5;
            *(unsigned int*)pDest = out;
            pDest += BASE85_OUTPUT;
            dwDestSize += BASE85_OUTPUT;
        }

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or srcChars isn't a multiple of 5*/
}

size_t cyoBase85DecodeW(void* dest, const wchar_t* src, size_t srcChars)
{
    if (dest && src && (srcChars % BASE85_INPUT == 0))
    {
        const wchar_t* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = srcChars;
        size_t dwDestSize = 0;
        wchar_t in1, in2, in3, in4, in5;
        unsigned int out;

        while (dwSrcSize >= 1)
        {
#if FOLD_ZERO
            if (*pSrc == L'z')
            {
                ++pSrc;
                *pDest++ = 0;
                dwDestSize += BASE85_OUTPUT;
                continue;
            }
#endif

            /* 5 inputs */
            in1 = (*pSrc++ - 33);
            in2 = (*pSrc++ - 33);
            in3 = (*pSrc++ - 33);
            in4 = (*pSrc++ - 33);
            in5 = (*pSrc++ - 33);
            dwSrcSize -= BASE85_INPUT;

            /* Validate */
            if (in1 >= 85 || in2 >= 85 || in3 >= 85 || in4 >= 85 || in5 >= 85)
                return 0; /*ERROR - invalid base85 character*/

            /* Output */
            out = in1;
            out *= 85;
            out |= in2;
            out *= 85;
            out |= in3;
            out *= 85;
            out |= in4;
            out *= 85;
            out |= in5;
            *(unsigned int*)pDest = out;
            pDest += BASE85_OUTPUT;
            dwDestSize += BASE85_OUTPUT;
        }

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or srcChars isn't a multiple of 5*/
}

size_t cyoBase85DecodeBlockA(void* dest, const char* src)
{
    if (dest && src)
    {
        const char* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        unsigned char in1, in2, in3, in4, in5;
        unsigned int out;

#if FOLD_ZERO
        if (*pSrc == 'z')
        {
            *pDest = 0;
            return BASE85_OUTPUT;
        }
#endif

        /* 5 inputs */
        in1 = (*pSrc++ - 33);
        in2 = (*pSrc++ - 33);
        in3 = (*pSrc++ - 33);
        in4 = (*pSrc++ - 33);
        in5 = (*pSrc++ - 33);

        /* Validate */
        if (in1 >= 85 || in2 >= 85 || in3 >= 85 || in4 >= 85 || in5 >= 85)
            return 0; /*ERROR - invalid base85 character*/

        /* Output */
        out = in1;
        out *= 85;
        out |= in2;
        out *= 85;
        out |= in3;
        out *= 85;
        out |= in4;
        out *= 85;
        out |= in5;
        *(unsigned int*)pDest = out;
        return BASE85_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}

size_t cyoBase85DecodeBlockW(void* dest, const wchar_t* src)
{
    if (dest && src)
    {
        const wchar_t* pSrc = src;
        unsigned char* pDest = (unsigned char*)dest;
        wchar_t in1, in2, in3, in4, in5;
        unsigned int out;

#if FOLD_ZERO
        if (*pSrc == L'z')
        {
            *pDest = 0;
            return BASE85_OUTPUT;
        }
#endif

        /* 5 inputs */
        in1 = (*pSrc++ - 33);
        in2 = (*pSrc++ - 33);
        in3 = (*pSrc++ - 33);
        in4 = (*pSrc++ - 33);
        in5 = (*pSrc++ - 33);

        /* Validate */
        if (in1 >= 85 || in2 >= 85 || in3 >= 85 || in4 >= 85 || in5 >= 85)
            return 0; /*ERROR - invalid base85 character*/

        /* Output */
        out = in1;
        out *= 85;
        out |= in2;
        out *= 85;
        out |= in3;
        out *= 85;
        out |= in4;
        out *= 85;
        out |= in5;
        *(unsigned int*)pDest = out;
        return BASE85_OUTPUT;
    }
    else
        return 0; /*ERROR - null pointer*/
}
