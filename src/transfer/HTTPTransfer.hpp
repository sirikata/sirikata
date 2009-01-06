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
#include "TransferManager.hpp"

namespace Iridium {
/** CacheLayer.hpp -- Class dealing with HTTP downloads. */
namespace Transfer {


class HTTPRequest {
	typedef boost::function3<void, HTTPRequest*,
			const DenseDataPtr &, bool> CallbackFunc;
	URI mURI;
	Range mRange;
	CallbackFunc mCallback;
public:

	inline const URI &getURI() const {return mURI;}
	inline const Range &getRange() const {return mRange;}

	static void nullCallback(HTTPRequest*, const DenseDataPtr &, bool);

	HTTPRequest(const URI &uri, const Range &range)
		: mURI(uri), mRange(range), mCallback(&nullCallback) {
	}

	void setCallback(const CallbackFunc &cb) {
		mCallback = cb;
	}

	void go();
};

//typedef boost::shared_ptr<HTTPRequest> HTTPRequestPtr;

/** Not really a cache layer, but implements the interface.*/
class NetworkTransfer : TransferLayer {
	void httpCallback(
			TransferCallback callback,
			HTTPRequest* httpreq,
			const DenseDataPtr &recvData,
			bool permanentError) {
		if (recvData) {
			// Now go back through the chain!
			TransferLayer::populateParentCaches(httpreq->getURI().fingerprint(), recvData);
			callback(recvData);
		} else {
			TransferLayer::getData(httpreq->getURI(), httpreq->getRange(), callback);
		}
	}

public:
	virtual bool getData(const URI &downloadURI,
			const Range &requestedRange,
			const TransferCallback &callback) {
		HTTPRequest* thisRequest = new HTTPRequest(downloadURI, requestedRange);
		thisRequest->setCallback(boost::bind(&NetworkTransfer::httpCallback, this, callback));
		// should call callback when it finishes.
		thisRequest->go();
		return true;
	}
};

}
}

#endif /* HTTPTRANSFER_HPP_ */
