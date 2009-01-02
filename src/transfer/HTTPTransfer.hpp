/*     Iridium Transfer -- Content Transfer management system
 *  HTTPTransfer.hpp
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
/*  Created on: Dec 31, 2008 */

#ifndef IRIDIUM_HTTPTransfer_HPP__
#define IRIDIUM_HTTPTransfer_HPP__

#include <vector>
#include "TransferLayer.hpp"
#include "CacheLayer.hpp"
#include "CacheManager.hpp"

#include "task/Time.hpp"
#include "task/TimerQueue.hpp"

namespace Iridium {
/** CacheLayer.hpp -- Class dealing with HTTP downloads. */
namespace Transfer {

#define RETRY_TIME 1.0

class HTTPRequest {
	boost::function2<void, const SparseDataPtr &, bool> callback;
	URI myURI;
	Range myRange;
public:
	HTTPRequest(const URI &uri, const Range &range)
		: myURI(uri), myRange(range) {
	}

	void setCallback(const boost::function2<void, const DenseData &, bool> &cb);

	bool go();
};
typedef boost::shared_ptr<HTTPRequest> HTTPRequestPtr;

/** Not really a cache layer, but implements the interface.*/
class HTTPTransfer : TransferLayer {
	void httpCallback(const FileId &fileId, int triesLeft, const Range &requestedRange,
			const boost::function1<void, SparseDataPtr> &callback,
			DenseData *recvData, bool permanentError) {
		if (recvData) {
			// Now go back through the chain!
			{boost::scoped_lock forCache(mTransferManager->mLock);
				TransferLayer::populateParentCaches(fileId, requestedRange, callback);
			}
			callback(recvData);
		} else {
			if (permanentError) {
				boost::scoped_lock forData(mTransferManager->mLock);
				TransferLayer::getData(fileId, requestedRange, callback);
			} else {
				Task::timer_queue.schedule(Task::AbsTime::now() + RETRY_TIME,
					boost::bind(getData, this, fileId, requestedRange, callback, triesLeft-1));
			}
		}
	}

public:
	virtual bool getData(const FileId &fileId, const Range &requestedRange,
				boost::function1<void, SparseDataPtr> callback, int numTries=3) {
		HTTPRequestPtr thisRequest (new HTTPRequest(URI(fileId), requestedRange));
		thisRequest->setCallback(boost::bind(&HTTPTransfer::httpCallback, this, fileId, numTries));
		// should call callback when it finishes.
		return thisRequest->go(); // (if returns false, already called callback).
	}
};

}
}

#endif /* HTTPTRANSFER_HPP_ */
