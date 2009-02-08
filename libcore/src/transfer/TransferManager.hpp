/*  Sirikata Transfer -- Content Transfer management system
 *  TransferManager.hpp
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

#ifndef SIRIKATA_TransferManager_HPP__
#define SIRIKATA_TransferManager_HPP__

#include "CacheLayer.hpp"

namespace Sirikata {
namespace Transfer {

/** Manages requests going into the cache.
 *
 * @see DownloadManager
 * @see TransferLayer
 * @see CacheLayer
 */
class TransferManager {
	CacheLayer *mFirstTransferLayer;

	/*
	struct RequestInfo {
		bool mPending; //pending overlapping requests.

		Range mReqRange;
		std::tr1::function<void(const Fingerprint&, const SparseData*)> mCallback;
	};
	typedef std::map<Fingerprint, std::list<RequestInfo> > RequestMap;
	RequestMap mActiveRequests;
	 */
	//boost::mutex mLock;
public:

	TransferManager(CacheLayer *firstLayer)
			: mFirstTransferLayer(firstLayer) {
	}

	/*
	void finishedRequest(const URI &name, const std::tr1::function<void(const URI&,
			const SparseData*, bool)>&callback) {
		// TODO: What to do when finished?
	}
	*/

	inline void purgeFromCache(const Fingerprint &fprint) {
		mFirstTransferLayer->purgeFromCache(fprint);
	}

	void download(const RemoteFileId &name,
                  const std::tr1::function<void(const SparseData*)>&callback,
				Range range) {
		// check for overlapping requests.
		// if overlapping, put in "pendingOn"
		// if not,
		//mFirstTransferLayer->getData(name, range, std::tr1::bind(&TransferManager::finishedRequest, this, name, callback));
		mFirstTransferLayer->getData(name, range, callback);
	}
};

}
}

#endif /* SIRIKATA_TransferManager_HPP__ */
