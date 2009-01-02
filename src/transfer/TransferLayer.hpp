/*     Iridium Transfer -- Content Transfer management system
 *  TransferLayer.hpp
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
/*  Created on: Dec 30, 2008 */

#ifndef IRIDIUM_TransferLayer_HPP__
#define IRIDIUM_TransferLayer_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <string>
#include <algorithm>
#include <map>
#include <list>
#include <vector>

#include "TransferData.hpp"

namespace Iridium {
/** TransferLayer.hpp -- TransferLayer and Range */
namespace Transfer {

/**
 * Cache callback function passed into CacheLayer::getData.
 *
 * @param SparseDataPtr   contains all data in the file this cache knows about.
 *                    SparseDataPtr is guaranteed to contain the requested block,
 *                    but it may be seprated into adjacent DenseData pieces.
 */
typedef boost::function1<void, SparseDataPtr> TransferCallback;

struct URI {
	std::string hash;
	std::string preferredServer;
};

typedef URI FileId;

class TransferManager;
class CacheLayer;

/** Base class for cache layer--will try a next cache and respond with the data to
 * any previous cache layers so they can store that data as well. */
class TransferLayer {
protected:
	TransferManager *mTransferManager;
private:
	CacheLayer *mRespondTo;
	TransferLayer *mNext;

protected:
	/** Goes up the heirararchy of cache layers filling in data.
	 * Note that you must *NOT* call the callback until you have
	 * populated the cache.
	 *
	 * @TODO: Handle two requests for the same chunk at the same time.
	 * */
	void populateParentCaches(const FileId &fileId, const DenseData &data);
public:

	virtual ~TransferLayer() {
	}

	/**
	 * Constructor needs to know what cache layer to try next, and what to return to.
	 */
	TransferLayer(TransferManager *cacheman, CacheLayer *respondTo, TransferLayer *tryNext)
		: mTransferManager(cacheman), mRespondTo(respondTo), mNext(tryNext) {
	}

	/**
	 * Query this cache layer.  If successful, call callback with the data and also
	 * call populateCache in order to populate the previous cache levels.
	 *
	 * @param fileId         A unique identifier corresponding to the file (probably a hash).
	 * @param requestedRange A Range object specifying a single range that you need.
	 * @param callback       To be called with the data if successful, or NULL if failed.
	 * @return          false, if the callback happened synchronously (i.e. in memory cache)
	 */
	virtual bool getData(const FileId &fileId, const Range &requestedRange,
			const TransferCallback&callback) {
		if (mNext) {
			return mNext->getData(fileId, requestedRange, callback);
		} else {
			callback(SparseDataPtr());
			return false;
		}
	}

};

}
}

#endif /* IRIDIUM_TransferLayer_HPP__ */
