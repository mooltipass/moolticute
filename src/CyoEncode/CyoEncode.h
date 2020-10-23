/*
 * CyoEncode.h - part of the CyoEncode library
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

#ifndef __CYOENCODE_H
#define __CYOENCODE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Base16 Encoding
 */
size_t cyoBase16EncodeGetLength(size_t srcBytes);
size_t cyoBase16EncodeA(char* dest, const void* src, size_t srcBytes);
size_t cyoBase16EncodeW(wchar_t* dest, const void* src, size_t srcBytes);
size_t cyoBase16EncodeBlockA(char* dest, const void* src); /*encodes 1 input bytes, outputs 2 chars*/
size_t cyoBase16EncodeBlockW(wchar_t* dest, const void* src); /*encodes 1 input bytes, outputs 2 chars*/
#define cyoBase16Encode cyoBase16EncodeA
#define cyoBase16EncodeBlock cyoBase16EncodeBlockA

/*
 * Base32 Encoding
 */
size_t cyoBase32EncodeGetLength(size_t srcBytes);
size_t cyoBase32EncodeA(char* dest, const void* src, size_t srcBytes);
size_t cyoBase32EncodeW(wchar_t* dest, const void* src, size_t srcBytes);
size_t cyoBase32EncodeBlockA(char* dest, const void* src); /*encodes 5 input bytes, outputs 8 chars*/
size_t cyoBase32EncodeBlockW(wchar_t* dest, const void* src); /*encodes 5 input bytes, outputs 8 chars*/
#define cyoBase32Encode cyoBase32EncodeA
#define cyoBase32EncodeBlock cyoBase32EncodeBlockA

/*
 * Base64 Encoding
 */
size_t cyoBase64EncodeGetLength(size_t srcBytes);
size_t cyoBase64EncodeA(char* dest, const void* src, size_t srcBytes);
size_t cyoBase64EncodeW(wchar_t* dest, const void* src, size_t srcBytes);
size_t cyoBase64EncodeBlockA(char* dest, const void* src); /*encodes 3 input bytes, outputs 4 chars*/
size_t cyoBase64EncodeBlockW(wchar_t* dest, const void* src); /*encodes 3 input bytes, outputs 4 chars*/
#define cyoBase64Encode cyoBase64EncodeA
#define cyoBase64EncodeBlock cyoBase64EncodeBlockA

/*
 * Base85 Encoding
 */
size_t cyoBase85EncodeGetLength(size_t srcBytes);
size_t cyoBase85EncodeA(char* dest, const void* src, size_t srcBytes);
size_t cyoBase85EncodeW(wchar_t* dest, const void* src, size_t srcBytes);
size_t cyoBase85EncodeBlockA(char* dest, const void* src); /*encodes 4 input bytes, outputs 5 chars*/
size_t cyoBase85EncodeBlockW(wchar_t* dest, const void* src); /*encodes 4 input bytes, outputs 5 chars*/
#define cyoBase85Encode cyoBase85EncodeA
#define cyoBase85EncodeBlock cyoBase85EncodeBlockA

#ifdef __cplusplus
}
#endif

#endif /*__CYOENCODE_H*/
