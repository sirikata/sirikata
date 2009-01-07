/*     Iridium Utilities -- Iridium Cryptography Utilities
 *  Sha256.hpp
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
 *  * Neither the name of Iridium nor the names of its contributors may
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
#include "Array.hpp"

#include <string>
#include <cstdlib>
#include <cstring>

namespace Iridium {
class SHA256 {
public:
    enum {static_size=32,hex_size=64};
private:
    Array<unsigned char,static_size> mData;
public:
    unsigned int size()const{
        return static_size;
    }
    /**
     * Allocates a std::string of size 64
     * \returns the std::string filled with mData converted to hex
     */
    std::string convertToHexString()const;
    /**
     * \returns an array filled with mData converted to hex
     */
    Array<char,hex_size> convertToHex()const;
    /**
     * \returns raw SHA256sum
     */
    const Array<unsigned char, static_size> &rawData()const {
        return mData;
    }

	bool operator==(const SHA256& other) const {
        return mData==other.mData;
    }
	bool operator!=(const SHA256& other) const {
        return !(mData==other.mData);
    }
	bool operator<(const SHA256& other) const{
        return mData<other.mData;
    }
    /**
     * Looks at the first 64 characters and pairs them into 32 bytes
     * Creates a shasum with these 32 raw bytes
     * \param digest must be 64 elements of hex digest
     * \throws std::invalid_argument if nonhex character encountered
     */
    static SHA256 convertFromHex(const char*digest);
    /**
     * Looks at the first 64 characters and pairs them into 32 bytes
     * \param digest must be a string of length 64
     * \throws std::invalid_argument if nonhex character encountered
     */
    static SHA256 convertFromHex(const std::string&digest);
    /**
     * Creates a shasum with these 32 raw bytes
     * \param digest holds the array of bytes
     */
    static SHA256 convertFromBinary(const Array<unsigned char,static_size>&digest) {
        return convertFromBinary(digest.data());
    }
    /**
     * Looks at the first 32 bytes
     * Creates a shasum with these 32 raw bytes
     * \param digest must be exactly 32 bytes long
     */
    static SHA256 convertFromBinary(const void*digest){
        SHA256 retval;
        memcpy(retval.mData.data(),digest,static_size);
        return retval;
    }
    /**
     * Computes SHA256 digest from data.
     * \param data contains data to be hashed
     * \param length is the length of the data to be hashed
     * \returns SHASum digest
     */
    static SHA256 computeDigest(const void *data, size_t length);
    /**
     * Computes SHA256 digest from data.
     * \param data contains data to be hashed
     * \returns SHASum digest
     */
    static SHA256 computeDigest(const std::string&data);
    /**
     * Fills the SHA256 with array of entirely 0's.
     */
    const SHA256&nil(){
        static SHA256 nil;
        static char empty_array[static_size]={0};
        static void* result=std::memcpy(nil.mData.data(),empty_array,static_size);
        return nil;
    }
    /**
     * Computes the digest of an empty file and returns that digest
     */
    const SHA256&emptyDigest() {
        static SHA256 empty=computeDigest(NULL,0);
        return empty;
    }

};
}
