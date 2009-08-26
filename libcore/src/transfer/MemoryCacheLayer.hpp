/*  Sirikata Transfer -- Content Transfer management system
 *  MemoryCacheLayer.hpp
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
/*  Created on: Jan 1, 2009 */

#ifndef SIRIKATA_MemoryCacheLayer_HPP__
#define SIRIKATA_MemoryCacheLayer_HPP__

#include "CacheLayer.hpp"
#include "CacheMap.hpp"

namespace Sirikata {
/** MemoryCacheLayer.hpp -- MemoryCacheLayer -- the first layer of transfer cache. */
namespace Transfer {

/// MemoryCacheLayer is usually the first layer in the cache--simple map from FileId to SparseData.
class MemoryCacheLayer : public CacheLayer {
public:
	struct CacheData : public CacheEntry {
		SparseData mSparse;
	};

private:
	typedef CacheMap MemoryMap;
	MemoryMap mData;

protected:
	virtual void populateCache(const Fingerprint &fileId, const DenseDataPtr &respondData) {
		{
			MemoryMap::write_iterator writer(mData);
			if (mData.alloc(respondData->length(), writer)) {
				bool newentry = writer.insert(fileId, respondData->length());
				if (newentry) {
					SILOG(transfer,debug,fileId << " created " << *respondData);
					CacheData *cdata = new CacheData;
					*writer = cdata;
					cdata->mSparse.addValidData(respondData);
					writer.use();
				} else {
					CacheData *cdata = static_cast<CacheData*>(*writer);
					cdata->mSparse.addValidData(respondData);
                    if (SILOGP(transfer,debug)) {
                        SILOGNOCR(transfer,debug,fileId << " already exists: ");
                        std::stringstream rangeListStream;
                        Range::printRangeList(rangeListStream,
                            static_cast<DenseDataList&>(cdata->mSparse),
                            static_cast<Range>(*respondData));
                        SILOGNOCR(transfer,debug,rangeListStream.str());
                    }
					writer.update(cdata->mSparse.getSpaceUsed());
				}

			}
		}
		CacheLayer::populateParentCaches(fileId, respondData);
	}

	virtual void destroyCacheEntry(const Fingerprint &fileId, CacheEntry *cacheLayerData, cache_usize_type releaseSize) {
		CacheData *toDelete = static_cast<CacheData*>(cacheLayerData);
		delete toDelete;
	}

public:
	MemoryCacheLayer(CachePolicy *policy, CacheLayer *tryNext)
			: CacheLayer(tryNext),
			mData(NULL, policy) {
        mData.setOwner(this);//to avoid warning in visual studio
	}

	virtual void purgeFromCache(const Fingerprint &fileId) {
		CacheMap::write_iterator iter(mData);
		if (iter.find(fileId)) {
			iter.erase();
		}
		CacheLayer::purgeFromCache(fileId);
	}

	virtual void getData(const RemoteFileId &uri, const Range &requestedRange,
			const TransferCallback&callback) {
		bool haveData = false;
		SparseData foundData;
		{
			MemoryMap::read_iterator iter(mData);
			if (iter.find(uri.fingerprint())) {
				const SparseData &sparseData = static_cast<const CacheData*>(*iter)->mSparse;
                if (SILOGP(transfer,debug)) {
                    SILOGNOCR(transfer,debug,"Found " << uri.fingerprint() << "; ranges=");
                        std::stringstream rangeListStream;
                        Range::printRangeList(rangeListStream,
                            static_cast<const DenseDataList&>(sparseData),
                            requestedRange);
                    SILOG(transfer,debug,rangeListStream.str());
                }
				if (sparseData.contains(requestedRange)) {
					haveData = true;
					foundData = sparseData;
				}
			}
		}
		if (haveData) {
			for (DenseDataList::iterator iter = foundData.DenseDataList::begin();
					iter != foundData.DenseDataList::end();
					++iter) {
				CacheLayer::populateParentCaches(uri.fingerprint(), iter.getPtr());
			}
			callback(&foundData);
		} else {
			CacheLayer::getData(uri, requestedRange, callback);
		}
	}
};

}
}

#endif /* SIRIKATA_MemoryCache_HPP__ */
