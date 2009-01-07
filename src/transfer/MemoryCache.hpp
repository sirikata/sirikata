/*     Iridium Transfer -- Content Transfer management system
 *  MemoryCache.hpp
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
/*  Created on: Jan 1, 2009 */

#ifndef IRIDIUM_MemoryCache_HPP__
#define IRIDIUM_MemoryCache_HPP__

#include <map>

#include "CacheLayer.hpp"

namespace Iridium {
/** MemoryCache.hpp -- MemoryCache -- the first layer of transfer cache. */
namespace Transfer {

/// MemoryCache is usually the first layer in the cache--simple map from FileId to SparseData.
class MemoryCache : public CacheLayer {
public:
	typedef SparseData CacheData;

private:
	typedef CacheMap MemoryMap;
	MemoryMap mData;

protected:
	virtual void populateCache(const Fingerprint &fileId, const DenseDataPtr &respondData) {
		{
			MemoryMap::write_iterator writer(mData);
			if (mData.alloc(respondData->mRange.mLength, writer)) {
				if (writer.insert(fileId, new SparseData(), respondData->mRange.mLength)) {
					SparseData *sparse = writer;
					(*sparse).addValidData(respondData);
					writer.use();
				} else {
					SparseData *sparse = writer;
					(*sparse).addValidData(respondData);
					writer.update((*sparse).getSpaceUsed());
				}

			}
		}
		CacheLayer::populateParentCaches(fileId, respondData);
	}

	virtual void destroyCacheEntry(const Fingerprint &fileId,  void *cacheLayerData) {
		CacheData *toDelete = (CacheData*)cacheLayerData;
		delete toDelete;
	}

public:
	MemoryCache(CachePolicy *policy, TransferManager *cacheMgr, CacheLayer *respondTo, CacheLayer *tryNext)
			: CacheLayer(cacheMgr, respondTo, tryNext),
			mData(this, policy) {
	}

	virtual bool getData(const URI &uri, const Range &requestedRange,
			const TransferCallback&callback) {
		bool haveData = false;
		SparseData foundData;
		{
			MemoryMap::read_iterator iter(mData);
			if (iter.find(uri.fingerprint())) {
				const SparseData *sparseData = iter;
				if (requestedRange.isContainedBy(*sparseData)) {
					haveData = true;
					foundData = *sparseData;
				}
			}
		}
		if (haveData) {
			for (SparseData::iterator iter = foundData.ptrbegin();
					iter != foundData.ptrend();
					++iter) {
				CacheLayer::populateParentCaches(uri.fingerprint(), (*iter));
			}
			callback(&foundData);
			return false;
		} else {
			return CacheLayer::getData(uri, requestedRange, callback);
		}
	}
};

}
}

#endif /* IRIDIUM_MemoryCache_HPP__ */

