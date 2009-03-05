/*  Sirikata Utilities -- Sirikata Cryptography Utilities
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

#ifndef _SIRIKATA_SHA256_HPP_
#define _SIRIKATA_SHA256_HPP_

#include <string>
#include <cstdlib>
#include <cstring>
#include <exception>

#include "Array.hpp"

namespace Sirikata {

class SHA256 {
	friend class SHA256Context;
public:
    enum {static_size=32,hex_size=64};
private:
    Array<unsigned char,static_size> mData;
public:
	/** Take the first sizeof(size_t) bytes of the SHA256
	 * as a valid hash for an unordered_map.
	 *
	 * Note: does not pay attention to endianness.
	 */
	struct Hasher {
		size_t operator() (const SHA256 &hash) const{
			return *((size_t*)(hash.rawData().data()));
		}
	};

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
    static const SHA256&nil(){
        static SHA256 nil;
        static char empty_array[static_size]={0};
        static void* result=std::memcpy(nil.mData.data(),empty_array,static_size);
        return nil;
    }
    /**
     * Computes the digest of an empty file and returns that digest
     */
    static const SHA256&emptyDigest() {
        static SHA256 empty=computeDigest(NULL,0);
        return empty;
    }

    friend inline std::ostream &operator <<(std::ostream &os, const SHA256&shasum) {
    	return os << shasum.convertToHexString();
    }

};

/** Class to allow creating a shasum from sparse data and other types. */
class SHA256Context {
	/// Do not allow copying because no function exists to copy SHA256_CTX
	SHA256Context(const SHA256Context &other);
	SHA256Context & operator=(const SHA256Context &other);

	/// Opaque reference to SHA256_CTX.
	void *mCtx;

	/** Stores the result after the context has been freed.
	 * Allows return by reference.
	 */
	SHA256 mRetval;
public:
	/// Constructs a new context with no data.
	SHA256Context();
	/// Destroys context only if not already cast to SHA256.
	~SHA256Context();

	/// Insert data.
	void update(const void *data, size_t length);
	/// Insert std::string data; same as update(data(),length())
	inline void update(const std::string &data) {
		update(data.data(), data.length());
	}
	/// Insert zero data -- special case to make SparseData easier to handle.
	void updateZeros(size_t length);

	/// Returns a SHA256 sum from the updated data. Cannot update after this.
	const SHA256& get();
};

}

#endif //_SIRIKATA_SHA256_HPP_
