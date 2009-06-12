/*  Sirikata Transfer -- Content Transfer management system
 *  CachePolicy.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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
/*  Created on: Jan 5, 2009 */

#ifndef SIRIKATA_CachePolicy_HPP__
#define SIRIKATA_CachePolicy_HPP__

#include "URI.hpp"

namespace Sirikata {
namespace Transfer {

static const cache_usize_type tebibyte = 0x10000000000LL, terabyte = 1000000000000LL;
static const cache_usize_type gibibyte = 0x40000000,      gigabyte = 1000000000;
static const cache_usize_type mebibyte = 0x100000,        megabyte = 1000000;
static const cache_usize_type kibibyte = 0x400,           kilobyte = 1000;
static const cache_usize_type byte     = 1;

/// Critical to the functioning of CacheLayer--makes decisions which pieces of data to keep and which to throw out.
class CachePolicy {

protected:
	cache_usize_type mTotalSize;
	float mMaxSizePct;
	cache_ssize_type mFreeSpace;

	inline void updateSpace(cache_usize_type oldsize, cache_usize_type newsize) {
		std::ostringstream oss;
		oss << "[CachePolicy] ";
		if (oldsize) {
			if (newsize) {
				oss << "Resizing cache data from " << oldsize << " to " << newsize;
			} else {
				oss << "Deallocating cache data of " << oldsize;
			}
		} else {
			oss << "Allocating cache data of " << newsize;
		}
		mFreeSpace += oldsize;
		mFreeSpace -= newsize;
		oss << "; free space is now " << mFreeSpace << ".";
		SILOG(transfer,debug,oss.str());
	}

public:

	struct Data {
	};

	CachePolicy(cache_usize_type allocatedSpace, float maxSizePct)
			: mTotalSize(allocatedSpace),
			mMaxSizePct(maxSizePct),
			mFreeSpace((cache_ssize_type)allocatedSpace) {
		}

	/// Virtual destructor since children will allocate class members.
	virtual ~CachePolicy() {
	}

	/**
	 *  Marks the entry as used
	 *  @param id    The FileId corresponding to the data.
	 *  @param data  The opaque data corresponding to this policy.
	 */
	virtual void use(const Fingerprint &id, Data* data, cache_usize_type size) = 0;

	/**
	 *  Marks the entry as used, and update the space usage
	 *  @param id    The FileId corresponding to the data.
	 *  @param data  The opaque data corresponding to this policy.
	 *  @param size  The amount of space actually used for this element.
	 */
	virtual void useAndUpdate(const Fingerprint &id, Data* data, cache_usize_type oldsize, cache_usize_type newsize) = 0;

	/**
	 *  Deletes the opaque data (and anything other corresponding info)
	 *
	 *  @param id    The FileId corresponding to the data.
	 *  @param data  The opaque data corresponding to this policy.
	 */
	virtual void destroy(const Fingerprint &id, Data* data, cache_usize_type size) = 0;

	/**
	 *  Allocates opaque data (and anything other corresponding info)
	 *  At this point, the allocation must not fail.  It is up to the
	 *  CacheLayer to respect the decision from allocateSpace().
	 *
	 *  @param id    The FileId corresponding to the data.
	 *  @param size  The amount of space allocated (should be the same
	 *               as was earlier passed to allocateSpace).
	 *  @returns     The corresponding opaque data.
	 */
	virtual Data* create(const Fingerprint &id, cache_usize_type size) = 0;

	/**
	 *  Allocates a certain number of bytes for use in a new cache entry.
	 *
	 *  @param requiredSpace  the amount of space this single entry needs
	 *  @param iter           A locked CacheMap::write_iterator
	 *                        (to use to delete large entries)
	 *  @returns              whether the space could be allocated
	 */
	virtual bool cachable(cache_usize_type requiredSpace) {
		if (((cache_ssize_type)requiredSpace) < 0) {
			// overflows a ssize_t.
			return false;
		}
		if ((double)requiredSpace >= (double)mTotalSize * mMaxSizePct) {
			SILOG(transfer,debug,"[CachePolicy] Rejecting allocation for " << requiredSpace << " bytes of " << mFreeSpace << " free");
			return false;
		}
		SILOG(transfer,debug,"[CachePolicy] Need to allocate " << requiredSpace << " bytes of " << mFreeSpace << " free");
		return true;
	}

	virtual bool nextItem(cache_usize_type requiredSpace, Fingerprint &myprint) = 0;
};


}
}

#include "CacheLayer.hpp"

#endif /* SIRIKATA_CachePolicy_HPP__ */
