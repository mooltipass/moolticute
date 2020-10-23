/*
 * CyoDecode.h - part of the CyoEncode library
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

#ifndef __CYODECODE_H
#define __CYODECODE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Base16 Decoding
 */
int cyoBase16ValidateA(const char* src, size_t srcChars);
int cyoBase16ValidateW(const wchar_t* src, size_t srcChars);
size_t cyoBase16DecodeGetLength(size_t srcChars);
size_t cyoBase16DecodeA(void* dest, const char* src, size_t srcChars);
size_t cyoBase16DecodeW(void* dest, const wchar_t* src, size_t srcChars);
size_t cyoBase16DecodeBlockA(void* dest, const char* src); /*decodes 2 input chars, outputs 1 byte*/
size_t cyoBase16DecodeBlockW(void* dest, const wchar_t* src); /*decodes 2 input chars, outputs 1 byte*/
#define cyoBase16Validate cyoBase16ValidateA
#define cyoBase16Decode cyoBase16DecodeA
#define cyoBase16DecodeBlock cyoBase16DecodeBlockA

/*
 * Base32 Decoding
 */
int cyoBase32ValidateA(const char* src, size_t srcChars);
int cyoBase32ValidateW(const wchar_t* src, size_t srcChars);
size_t cyoBase32DecodeGetLength(size_t srcChars);
size_t cyoBase32DecodeA(void* dest, const char* src, size_t srcChars);
size_t cyoBase32DecodeW(void* dest, const wchar_t* src, size_t srcChars);
size_t cyoBase32DecodeBlockA(void* dest, const char* src); /*decodes 8 input chars, outputs 5 bytes*/
size_t cyoBase32DecodeBlockW(void* dest, const wchar_t* src); /*decodes 8 input chars, outputs 5 bytes*/
#define cyoBase32Validate cyoBase32ValidateA
#define cyoBase32Decode cyoBase32DecodeA
#define cyoBase32DecodeBlock cyoBase32DecodeBlockA

/*
 * Base64 Decoding
 */
int cyoBase64ValidateA(const char* src, size_t srcChars);
int cyoBase64ValidateW(const wchar_t* src, size_t srcChars);
size_t cyoBase64DecodeGetLength(size_t srcChars);
size_t cyoBase64DecodeA(void* dest, const char* src, size_t srcChars);
size_t cyoBase64DecodeW(void* dest, const wchar_t* src, size_t srcChars);
size_t cyoBase64DecodeBlockA(void* dest, const char* src); /*decodes 4 input chars, outputs 3 bytes*/
size_t cyoBase64DecodeBlockW(void* dest, const wchar_t* src); /*decodes 4 input chars, outputs 3 bytes*/
#define cyoBase64Validate cyoBase64ValidateA
#define cyoBase64Decode cyoBase64DecodeA
#define cyoBase64DecodeBlock cyoBase64DecodeBlockA

/*
 * Base85 Decoding
 */
int cyoBase85ValidateA(const char* src, size_t srcChars);
int cyoBase85ValidateW(const wchar_t* src, size_t srcChars);
size_t cyoBase85DecodeGetLength(size_t srcChars);
size_t cyoBase85DecodeA(void* dest, const char* src, size_t srcChars);
size_t cyoBase85DecodeW(void* dest, const wchar_t* src, size_t srcChars);
size_t cyoBase85DecodeBlockA(void* dest, const char* src); /*decodes 5 input chars, outputs 4 bytes*/
size_t cyoBase85DecodeBlockW(void* dest, const wchar_t* src); /*decodes 5 input chars, outputs 4 bytes*/
#define cyoBase85Validate cyoBase85ValidateA
#define cyoBase85Decode cyoBase85DecodeA
#define cyoBase85DecodeBlock cyoBase85DecodeBlockA

#ifdef __cplusplus
}
#endif

#endif /*__CYODECODE_H*/
