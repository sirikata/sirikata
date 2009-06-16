/*  Sirikata Transfer -- Content Transfer management system
 *  CacheLayer.hpp
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
/*  Created on: Dec 31, 2008 */

#ifndef SIRIKATA_CacheLayer_HPP__
#define SIRIKATA_CacheLayer_HPP__

#include "TransferData.hpp"
#include "URI.hpp"
#include "CachePolicy.hpp"

namespace Sirikata {
/** CacheLayer.hpp -- CacheLayer superclass */
namespace Transfer {

/**
 * Cache callback function passed into CacheLayer::getData.
 *
 * @param SparseDataPtr   contains all data in the file this cache knows about.
 *                    SparseDataPtr is guaranteed to contain the requested block,
 *                    but it may be seprated into adjacent DenseData pieces.
 */
typedef std::tr1::function<void(const SparseData*)> TransferCallback;

/** Base class for cache layer--will try a next cache and respond with the data to
 * any previous cache layers so they can store that data as well. */
class CacheLayer : Noncopyable {
private:
	CacheLayer *mRespondTo;
	CacheLayer *mNext;

	friend class CacheMap;

	inline void setResponder(CacheLayer *other) {
		mRespondTo = other;
	}

protected:
	/** Goes up the heirararchy of cache layers filling in data.
	 * Note that you must *NOT* call the callback until you have
	 * populated the cache.
	 *
	 * @TODO: Handle two requests for the same chunk at the same time.
	 * */
	inline void populateParentCaches(const Fingerprint &fileId, const DenseDataPtr &data) {
		if (mRespondTo) {
			mRespondTo->populateCache(fileId, data);
		}
	}

	struct CacheEntry {
	};

	/** Called after a cache entry has been destroyed--
	 * Does not remove the item from other caches in the chain.
	 *
	 * Should be called only from CacheMap.
	 */
	virtual void destroyCacheEntry(const Fingerprint &fileId, CacheEntry *cacheLayerData, cache_usize_type releaseSize) {
	}

	/**
	 * Goes up the heirararchy of cache layers filling in data.
	 *
	 * @param fileId  the Fingerprint to store this data in CacheMap.
	 * @param data    Data to be stored in this CacheLayer.
	 * */
	virtual void populateCache(const Fingerprint &fileId, const DenseDataPtr &data) {
		populateParentCaches(fileId, data);
	}

public:

	virtual ~CacheLayer() {
		if (mNext) {
			mNext->setResponder(NULL);
		}
	}

	/**
	 * Constructor needs to know what cache layer to try next, and what to return to.
	 */
	CacheLayer(CacheLayer *tryNext)
			: mRespondTo(NULL), mNext(tryNext) {
		if (tryNext) {
			tryNext->setResponder(this);
		}
	}

	/* UNSAFE (for testing only)!
	   Ensure you set this back to NULL before deallocating either CacheLayer.
	*/
	void setRespondTo(CacheLayer *newRespond) {
		mRespondTo = newRespond;
	}

	/** If you want to change the ordering of the cache layers.
	 * This may be safe, but it's probably not smart to use except when
	 * constructing or destroying a CacheLayer.
	 */
	void setNext(CacheLayer *newNext) {
		if (mNext) {
			mNext->setResponder(NULL);
		}
		mNext = newNext;
		if (mNext) {
			mNext->setResponder(this);
		}
	}

	CacheLayer *getNext() const {
		return mNext;
	}

	CacheLayer *getResponder() const {
		return mRespondTo;
	}

	void addToCache(const Fingerprint &fileId, const DenseDataPtr &data) {
		if (mNext) {
			mNext->addToCache(fileId, data);
		} else {
			populateCache(fileId, data);
		}
	}

	/**
	 * Purges this hash from all subsequent caches. In general, it is not
	 * useful to do this manually, since the CachePolicy handles freeing
	 * extra data, and it is usually assumed that the cache is correct.
	 *
	 * Validity checking should probably be added elsewhere--though it is
	 * not really feasable for incomplete downloads if we used a range.
	 */
	virtual void purgeFromCache(const Fingerprint &fileId) {
		if (mNext) {
			mNext->purgeFromCache(fileId);
		}
	}

	/**
	 * Query this cache layer.  If successful, call callback with the data and also
	 * call populateCache in order to populate the previous cache levels.
	 *
	 * @param uri         A unique identifier corresponding to the file (contains a hash).
	 * @param requestedRange A Range object specifying a single range that you need.
	 * @param callback       To be called with the data if successful, or NULL if failed.
	 * @return          false, if the callback happened synchronously (i.e. in memory cache)
	 */
	virtual void getData(const RemoteFileId &fid, const Range &requestedRange,
			const TransferCallback&callback) {
		if (mNext) {
			mNext->getData(fid, requestedRange, callback);
		} else {
			// need some way to signal error
			callback(NULL);
		}
	}

};

}
}

#endif /* SIRIKATA_CacheLayer_HPP__ */
