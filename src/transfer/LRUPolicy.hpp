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

class LRUPolicy : CachePolicy {
	typedef LRUList::iterator* LRUData;

	typedef std::pair<Fingerprint, size_t> LRUElement;
	typedef std::list<LRUElement> LRUList;

	size_t mTotalSize;
	ssize_t mFreeSpace;

	LRUList mLeastUsed;
	// std::list<Fingerprint>::const_iterator

public:
	LRUPolicy(size_t allocatedSpace)
		: mTotalSize(allocatedSpace),
		mFreeSpace((ssize_t)allocatedSpace) {
	}

	virtual void use(const Fingerprint &id, Data &data) {
		// "All iterators remain valid"
		mLeastUsed.splice(mLeastUsed.end(), mLeastUsed, *(LRUData)data);
	}

	virtual void useAndUpdate(const Fingerprint &id, Data &data, size_t size) {
		use(id, data); // No optimizations to be made here.

		LRUList::iterator &lruiter = *(LRUData)data;

		mFreeSpace += ((*lruiter).second - size); // update the difference
		(*lruiter).second = size;

	}

	virtual void destroy(const Fingerprint &id, const Data &data) {
		LRUData *lrudata = (LRUData)data;

		mFreeSpace += (**lrudata).second; // return the space

		mLeastUsed.erase(*lrudata);
		delete lrudata;
	}

	virtual Data create(const Fingerprint &id, size_t size) {
		mFreeSpace -= size; // remove the space

		mLeastUsed.push_back(LRUData(id, size));
		LRUList::iterator newIter = mLeastUsed.end();
		--newIter; // I wish push_back returned an iterator

		return (Data)(new LRUList::iterator(newIter));
	}

	virtual bool allocateSpace(
			size_t requiredSpace,
			typename CacheMap<CacheInfo>::write_iterator &writer)
	{
		if ((double)requiredSpace > (double)mTotalSize * 0.75) {
			return false;
		}
		while (mFreeSpace < requiredSpace && !mLeastUsed.empty()) {
			LRUList::iterator lruiter = mLeastUsed.begin();
			requiredSpace -= (*lruiter).second;

			writer.find((*lruiter).first);
			writer.erase(); // calls destroy();
		}
		return (mFreeSpace > requiredSpace);
	}
};

}
}

#endif /* IRIDIUM_LRUPolicy_HPP__ */
