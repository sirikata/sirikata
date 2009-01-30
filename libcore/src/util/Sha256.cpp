/*  Sirikata Utilities -- Sirikata Cryptography Utilities
 *  Sha256.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "Sha256.hpp"
#include "internal_sha2.hpp"
#include <stdexcept>
#include <iostream>
namespace Sirikata {
static unsigned char numToHex(unsigned int num) {
    if (num<10)
        return num+'0';
    if (num<16)
        return (num-10)+'a';
    return 'x';
}
static unsigned int numFromHex(unsigned char charValue) {
    if (charValue>='0'&&charValue<='9')
        return charValue-'0';
    if (charValue>='a'&&charValue<='f')
        return 10 + (charValue-'a');
    if (charValue>='A'&&charValue<='F')
        return 10 + (charValue-'A');
    static char error_message[]="Charater \'X\' not a hexidecimal number";
     error_message[10]=charValue;
    //not thread safe--but you'll get at least one of the error messages
    throw std::invalid_argument(error_message);
    return 0;
}
std::string SHA256::convertToHexString() const {
    std::string retval;
    retval.resize(hex_size);
    for (unsigned int i=0;i<static_size;++i) {
        retval[i*2]=numToHex(mData[i]/16);
        retval[i*2+1]=numToHex(mData[i]%16);
    }
    return retval;
}

Array<char,SHA256::hex_size> SHA256::convertToHex() const {
    Array<char,hex_size> retval;
    for (unsigned int i=0;i<static_size;++i) {
        retval[i*2]=numToHex(mData[i]/16);
        retval[i*2+1]=numToHex(mData[i]%16);
    }
    return retval;
}
SHA256 SHA256::convertFromHex(const char*data){
    SHA256 retval;
    for (unsigned int i=0;i<static_size;++i) {
        retval.mData[i]=numFromHex(data[i*2])*16+numFromHex(data[i*2+1]);
    }
    return retval;
}
SHA256 SHA256::convertFromHex(const std::string&data){
    if (data.length()!=64)
        throw std::invalid_argument("input string was the wrong length for a SHA256 Digest");
    return convertFromHex(data.data());
}
SHA256 SHA256::computeDigest(const void*data, size_t length) {
    SHA256 retval;
    Sirikata::Util::Internal::SHA256_CTX context;
    Sirikata::Util::Internal::SHA256_Init(&context);
    Sirikata::Util::Internal::SHA256_Update(&context, (const unsigned char*)data, length);
    Sirikata::Util::Internal::SHA256_Final(retval.mData.data(), &context);
    return retval;
}
SHA256 SHA256::computeDigest(const std::string&data) {
    SHA256 retval;
    Sirikata::Util::Internal::SHA256_CTX context;
    Sirikata::Util::Internal::SHA256_Init(&context);
    Sirikata::Util::Internal::SHA256_Update(&context, (const unsigned char*)data.data(), data.length());
    Sirikata::Util::Internal::SHA256_Final(retval.mData.data(), &context);
    return retval;
}
}
