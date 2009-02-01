/*  Sirikata Transfer -- Content Transfer management system
 *  NetworkTransfer.hpp
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

#ifndef SIRIKATA_HTTPTransfer_HPP__
#define SIRIKATA_HTTPTransfer_HPP__

#include "CacheLayer.hpp"
#include "HTTPRequest.hpp"

namespace Sirikata {
/** CacheLayer.hpp -- Class dealing with HTTP downloads. */
namespace Transfer {


/** Not really a cache layer, but implements the interface.*/
class NetworkTransfer : public CacheLayer {
	volatile bool cleanup;
	std::list<HTTPRequest> activeTransfers;
	boost::mutex transferLock; // for abort.

	void httpCallback(
			std::list<HTTPRequest>::iterator iter,
			TransferCallback callback,
			HTTPRequest* httpreq,
			const DenseDataPtr &recvData,
			bool success) {
		if (recvData && success) {
			// Now go back through the chain!
			CacheLayer::populateParentCaches(httpreq->getURI().fingerprint(), recvData);
			SparseData data;
			data.addValidData(recvData);
			callback(&data);
		} else {
			CacheLayer::getData(httpreq->getURI(), httpreq->getRange(), callback);
		}
		if (!cleanup) {
			boost::unique_lock<boost::mutex> transfer_lock(transferLock);
			activeTransfers.erase(iter);
		}
	}

public:
	NetworkTransfer(CacheLayer *next)
			:CacheLayer(next) {
		cleanup = false;
	}

	virtual ~NetworkTransfer() {
		boost::unique_lock<boost::mutex> transfer_lock(transferLock);
		cleanup = true;
		for (std::list<HTTPRequest>::iterator iter = activeTransfers.begin();
				iter != activeTransfers.end();
				++iter) {
			(*iter).abort();
		}
		activeTransfers.clear();
	}

	virtual bool getData(const URI &downloadURI,
			const Range &requestedRange,
			const TransferCallback &callback) {
		HTTPRequest *thisRequest;
		std::list<HTTPRequest>::iterator transferIter;
		{
			boost::unique_lock<boost::mutex> access_activeTransfers(transferLock);
			activeTransfers.push_back(HTTPRequest(downloadURI, requestedRange));
			transferIter = activeTransfers.end();
			--transferIter;
			thisRequest = &(*transferIter);
		}
		thisRequest->setCallback(std::tr1::bind(&NetworkTransfer::httpCallback, this, transferIter, callback, _1, _2, _3));
		// should call callback when it finishes.
		thisRequest->go();
		return true;
	}
};

}
}

#endif /* HTTPTRANSFER_HPP_ */
