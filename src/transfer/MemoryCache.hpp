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

class MemoryCache : CacheLayer {

	typedef std::map<FileId, std::pair<SparseDataPtr, CacheInfoPtr> > DataMap;
	DataMap mData;

protected:
	virtual void populateCache(const FileId &fileId, const DenseData &respondData) {
		DataMap::const_iterator iter = mData.find(fileId);
		if (iter == mData.end()) {
			iter = mData.insert(fileId, new SparseData()).second;
		}
		if (mCachePolicy->allocateSpace(respondData.length())) {
			mCachePolicy->insertAndUse(fileId, respondData.length());
			TransferLayer::populateParentCaches(fileId, respondData);
		}
	}

public:
	virtual bool getData(const FileId &fileId, const Range &requestedRange,
			const TransferCallback&callback) {
		DataMap::const_iterator iter = mData.find(fileId);
		if (iter != mData.end()) {
			const SparseDataPtr &sparseData = (*iter).second;
			if (sparseData->containsRange(range)) {
				callback(sparseData);
				return false;
			}
		}
		return TransferLayer::getData(fileId, range, callback);
	}
};

}
}

#endif /* IRIDIUM_MemoryCache_HPP__ */

