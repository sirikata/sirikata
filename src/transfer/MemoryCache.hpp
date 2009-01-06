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
class MemoryCache : CacheLayer {
public:
	typedef SparseDataPtr CacheData;

private:
	typedef CacheMap<DiskInfo> MemoryMap;
	MemoryMap mData;

protected:
	virtual void populateCache(const Fingerprint &fileId, const DenseDataPtr &respondData) {
		{
			MemoryMap::write_iterator writer;
			if (mCachePolicy->allocateSpace(writer, respondData.length())) {
				if (writer.insert(fileId, SparseDataPtr())) {
					writer.use();
					*writer = respondData;
				} else {
					size_t length = (*writer)->add(respondData);
					writer.update(fileId, length);
				}

			}
		}
		TransferLayer::populateParentCaches(fileId, respondData);
	}

public:
	MemoryCache(CachePolicy<CacheData> *policy, TransferManager *cacheMgr, CacheLayer *respondTo, TransferLayer *tryNext)
			: CacheLayer(cacheMgr, respondTo, tryNext),
			mFiles(policy) {
	}

	virtual bool getData(const URI &uri, const Range &requestedRange,
			const TransferCallback&callback) {
		SparseDataPtr foundData;
		{
			DataMap::read_iterator iter(mData);
			if (iter.find(uri.fingerprint())) {
				const SparseDataPtr &sparseData = (*iter).second;
				if (sparseData->containsRange(range)) {
					foundData = sparseData;
				}
			}
		}
		if (foundData) {
			TransferLayer::populateParentCaches(uri.fingerprint(), foundData);
			callback(foundData);
			return false;
		} else {
			return TransferLayer::getData(uri, range, callback);
		}
	}
};

}
}

#endif /* IRIDIUM_MemoryCache_HPP__ */

