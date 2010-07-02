/*  Sirikata Transfer -- Content Transfer management system
 *  RemoteFileMetadata.hpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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
/*  Created on: Jan 18th, 2010 */

#ifndef SIRIKATA_RemoteFileMetadata_HPP__
#define SIRIKATA_RemoteFileMetadata_HPP__

#include <sirikata/core/util/Sha256.hpp>
#include <sirikata/core/transfer/Range.hpp>
#include <sirikata/core/transfer/URI.hpp>

namespace Sirikata {
namespace Transfer {

typedef uint64 size_type;

typedef SHA256 Fingerprint;

/*
 * Storage for a single chunk of a file - has start, end, length, hash
 * Uses Range and Fingerprint
 */
class Chunk {
private:
    Fingerprint mHash;
    Range mRange;
public:
    Chunk(const Fingerprint &fingerprint, const Range &range) :
        mHash(fingerprint), mRange(range) {

    }

    inline const Fingerprint getHash() const {
        return mHash;
    }

    inline const Range getRange() const {
        return mRange;
    }

    inline bool operator==(const Chunk &other) const {
        return (mHash == other.mHash && mRange == other.mRange);
    }

};

typedef std::list<Chunk> ChunkList;
typedef std::map<std::string, std::string> FileHeaders;

/** Container for all of the metadata associated with a remote file.
 * Stores URI, Fingerprint, file size, chunk list
 */
class RemoteFileMetadata {
	Fingerprint mHash;
	URI mURI;
	size_type mSize;
	ChunkList mChunkList;
	FileHeaders mHeaders;

public:

	RemoteFileMetadata(const Fingerprint &fingerprint, const URI &uri, size_type size, const ChunkList &chunklist, const FileHeaders &headers)
		: mHash(fingerprint), mURI(uri), mSize(size), mChunkList(chunklist), mHeaders(headers) {

	}

	inline size_type getSize() const {
	    return mSize;
	}

	inline std::string toString() const {
		return mURI.toString();
	}

	inline const Fingerprint &getFingerprint() const {
		return mHash;
	}

	inline const URI &getURI() const {
		return mURI;
	}

	inline const ChunkList &getChunkList() const {
		return mChunkList;
	}

	inline const FileHeaders &getHeaders() const {
		return mHeaders;
	}

	inline bool operator==(const RemoteFileMetadata &other) const {
		return getURI() == other.getURI();
	}
	inline bool operator!=(const RemoteFileMetadata &other) const {
		return getURI() != other.getURI();
	}
	inline bool operator<(const RemoteFileMetadata &other) const {
		return getURI() < other.getURI();
	}

};

}
}

#endif
