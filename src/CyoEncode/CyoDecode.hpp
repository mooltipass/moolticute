/*
 * CyoDecode.hpp - part of the CyoEncode library
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

#ifndef __CYODECODE_HPP
#define __CYODECODE_HPP

#include "CyoDecode.h"

namespace CyoDecode
{
    typedef unsigned char byte_t;

    class Base16
    {
    public:
        static int Validate(const char* src, size_t srcChars) {
            return cyoBase16ValidateA(src, srcChars);
        }
        static int Validate(const wchar_t* src, size_t srcChars) {
            return cyoBase16ValidateW(src, srcChars);
        }
        static size_t GetLength(size_t srcChars) {
            return cyoBase16DecodeGetLength(srcChars);
        }
        static size_t Decode(byte_t* dest, const char* src, size_t srcChars) {
            return cyoBase16DecodeA(dest, src, srcChars);
        }
        static size_t Decode(byte_t* dest, const wchar_t* src, size_t srcChars) {
            return cyoBase16DecodeW(dest, src, srcChars);
        }
        static size_t DecodeBlock(byte_t& dest, const char src[2]) {
            return cyoBase16DecodeBlockA(&dest, src);
        }
        static size_t DecodeBlock(byte_t& dest, const wchar_t src[2]) {
            return cyoBase16DecodeBlockW(&dest, src);
        }
    };

    class Base32
    {
    public:
        static int Validate(const char* src, size_t srcChars) {
            return cyoBase32ValidateA(src, srcChars);
        }
        static int Validate(const wchar_t* src, size_t srcChars) {
            return cyoBase32ValidateW(src, srcChars);
        }
        static size_t GetLength(size_t srcChars) {
            return cyoBase32DecodeGetLength(srcChars);
        }
        static size_t Decode(byte_t* dest, const char* src, size_t srcChars) {
            return cyoBase32DecodeA(dest, src, srcChars);
        }
        static size_t Decode(byte_t* dest, const wchar_t* src, size_t srcChars) {
            return cyoBase32DecodeW(dest, src, srcChars);
        }
        static size_t DecodeBlock(byte_t dest[5], const char src[8]) {
            return cyoBase32DecodeBlockA(dest, src);
        }
        static size_t DecodeBlock(byte_t dest[5], const wchar_t src[8]) {
            return cyoBase32DecodeBlockW(dest, src);
        }
    };

    class Base64
    {
    public:
        static int Validate(const char* src, size_t srcChars) {
            return cyoBase64ValidateA(src, srcChars);
        }
        static int Validate(const wchar_t* src, size_t srcChars) {
            return cyoBase64ValidateW(src, srcChars);
        }
        static size_t GetLength(size_t srcChars) {
            return cyoBase64DecodeGetLength(srcChars);
        }
        static size_t Decode(byte_t* dest, const char* src, size_t srcChars) {
            return cyoBase64DecodeA(dest, src, srcChars);
        }
        static size_t Decode(byte_t* dest, const wchar_t* src, size_t srcChars) {
            return cyoBase64DecodeW(dest, src, srcChars);
        }
        static size_t DecodeBlock(byte_t dest[3], const char src[4]) {
            return cyoBase64DecodeBlockA(dest, src);
        }
        static size_t DecodeBlock(byte_t dest[3], const wchar_t src[4]) {
            return cyoBase64DecodeBlockW(dest, src);
        }
    };

    class Base85
    {
    public:
        static int Validate(const char* src, size_t srcChars) {
            return cyoBase85ValidateA(src, srcChars);
        }
        static int Validate(const wchar_t* src, size_t srcChars) {
            return cyoBase85ValidateW(src, srcChars);
        }
        static size_t GetLength(size_t srcChars) {
            return cyoBase85DecodeGetLength(srcChars);
        }
        static size_t Decode(byte_t* dest, const char* src, size_t srcChars) {
            return cyoBase85DecodeA(dest, src, srcChars);
        }
        static size_t Decode(byte_t* dest, const wchar_t* src, size_t srcChars) {
            return cyoBase85DecodeW(dest, src, srcChars);
        }
        static size_t DecodeBlock(byte_t dest[3], const char src[4]) {
            return cyoBase85DecodeBlockA(dest, src);
        }
        static size_t DecodeBlock(byte_t dest[3], const wchar_t src[4]) {
            return cyoBase85DecodeBlockW(dest, src);
        }
    };
}

#endif //__CYODECODE_HPP
