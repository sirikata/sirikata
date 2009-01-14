/*     Iridium Transfer -- Content Transfer management system
 *  LRUPolicy.hpp
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
/*  Created on: Jan 5, 2009 */

#ifndef IRIDIUM_LRUPolicy_HPP__
#define IRIDIUM_LRUPolicy_HPP__

#include <vector>
#include "CachePolicy.hpp"

namespace Iridium {
namespace Transfer {

/// Simple LRU policy--does not do any ordering by size.
class LRUPolicy : public CachePolicy {

	typedef Fingerprint LRUElement;
	typedef std::list<LRUElement> LRUList;

	struct LRUData : public Data {
		LRUList::iterator mIter;

		LRUData(const LRUList::iterator &copyIter)
			: mIter(copyIter) {
		}
	};

	LRUList mLeastUsed;
	// std::list<Fingerprint>::const_iterator

public:
	LRUPolicy(cache_usize_type allocatedSpace, float maxSizePct=0.5)
		: CachePolicy(allocatedSpace, maxSizePct) {
	}

	virtual void use(const Fingerprint &id, Data* data, cache_usize_type size) {
		LRUData *lrudata = static_cast<LRUData*>(data);

		// "All iterators remain valid"
		mLeastUsed.splice(mLeastUsed.end(), mLeastUsed, lrudata->mIter);
	}

	virtual void useAndUpdate(const Fingerprint &id, Data* data, cache_usize_type oldsize, cache_usize_type newsize) {
		use(id, data, newsize); // No optimizations to be made here.
		CachePolicy::updateSpace(oldsize, newsize);
	}

	virtual void destroy(const Fingerprint &id, Data* data, cache_usize_type size) {
		LRUData *lrudata = static_cast<LRUData*>(data);

		CachePolicy::updateSpace(size, 0);

		std::cout << "[LRUPolicy] Freeing " << id << " (" << size << " bytes); " << mFreeSpace << " free" << std::endl;
		mLeastUsed.erase(lrudata->mIter);
		delete lrudata;
	}

	virtual Data* create(const Fingerprint &id, cache_usize_type size) {
		CachePolicy::updateSpace(0, size);

		mLeastUsed.push_back(id);
		LRUList::iterator newIter = mLeastUsed.end();
		--newIter; // I wish push_back returned an iterator

		return new LRUData(newIter);
	}

	virtual bool nextItem(
			cache_usize_type requiredSpace,
			Fingerprint &myprint)
	{
		if (mFreeSpace < (cache_ssize_type)requiredSpace && !mLeastUsed.empty()) {

			myprint = mLeastUsed.front();
			return true;
		} else {
			return false;
		}
	}
};

}
}

#endif /* IRIDIUM_LRUPolicy_HPP__ */
