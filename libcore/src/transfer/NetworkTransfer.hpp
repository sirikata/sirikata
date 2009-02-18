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
#include "ServiceLookup.hpp"
#include "ProtocolRegistry.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace Sirikata {
/** CacheLayer.hpp -- Class dealing with HTTP downloads. */
namespace Transfer {


/** Not really a cache layer, but implements the interface.*/
class NetworkTransfer : public CacheLayer {
	struct RequestInfo {
		DownloadHandler::TransferDataPtr httpreq;
		TransferCallback callback;
		RemoteFileId fileId;
		Range range;
		ListOfServicesPtr services;

		RequestInfo(const RemoteFileId &fileId, const Range &range, TransferCallback cb)
			: callback(cb), fileId(fileId), range(range) {
		}
	};

	volatile bool cleanup;
	std::list<RequestInfo> mActiveTransfers;
	ServiceLookup *mServicesLookup;
	ProtocolRegistry<DownloadHandler> *mProtoReg;
	boost::mutex mActiveTransferLock; ///< for abort.
	boost::condition_variable mCleanupCV;

	void httpCallback(
			std::list<RequestInfo>::iterator iter,
			unsigned int whichService,
			DenseDataPtr recvData,
			bool success) {
		const RequestInfo &info = *iter;
		if (recvData && success) {
			// Now go back through the chain!
			CacheLayer::populateParentCaches(info.fileId.fingerprint(), recvData);
			SparseData data;
			data.addValidData(recvData);
			info.callback(&data);

			boost::unique_lock<boost::mutex> transfer_lock(mActiveTransferLock);
			mActiveTransfers.erase(iter);
			mCleanupCV.notify_one();
		} else {
			// don't want to make new queries in this case.
			doFetch(iter, whichService+1);
		}
	}

	void doFetch(
			std::list<RequestInfo>::iterator iter,
			unsigned int whichService) {
		/* FIXME: this does not acquire a lock--which will cause a problem
		 * if this class is destructed while we are executing doFetch.
		 */
		RequestInfo &info = *iter;
		if (cleanup || whichService >= info.services->size()) {
			std::cerr << "None of the " << whichService << " download URIContexts registered for " <<
					info.fileId.uri() << " were successful." << std::endl;
			CacheLayer::getData(info.fileId, info.range, info.callback);
			boost::unique_lock<boost::mutex> transfer_lock(mActiveTransferLock);
			mActiveTransfers.erase(iter);
			mCleanupCV.notify_one();
			return;
		}
		URI lookupUri ((*info.services)[whichService], info.fileId.uri().filename());
		std::tr1::shared_ptr<DownloadHandler> handler = mProtoReg->lookup(lookupUri.proto());
		if (handler) {
			// info IS GETTING FREED BEFORE download RETURNS TO SET info.httpreq!!!!!!!!!
			info.httpreq = DownloadHandler::TransferDataPtr();
			handler->download(&info.httpreq, lookupUri, info.range,
					std::tr1::bind(&NetworkTransfer::httpCallback, this, iter, whichService, _1, _2));
			// info may be deleted by now (not so unlikely as it sounds -- it happens if you connect to localhost)
		} else {
			doFetch(iter, whichService+1);
		}
	}

	void gotServices(std::list<RequestInfo>::iterator iter, const ListOfServicesPtr &services) {
		(*iter).services = services;
		doFetch(iter, 0);
	}

public:
	NetworkTransfer(CacheLayer *next, ServiceLookup *services, ProtocolRegistry<DownloadHandler> *protoReg)
			:CacheLayer(next), mServicesLookup(services), mProtoReg(protoReg) {
		cleanup = false;
	}

	virtual ~NetworkTransfer() {
		cleanup = true; // Prevents doFetch (callback for NameLookup) from starting a new download.
		std::list<DownloadHandler::TransferDataPtr> pendingDelete;
		{
			boost::unique_lock<boost::mutex> transfer_lock(mActiveTransferLock);
			for (std::list<RequestInfo>::iterator iter = mActiveTransfers.begin();
					iter != mActiveTransfers.end();
					++iter) {
				if ((*iter).httpreq) {
					pendingDelete.push_back((*iter).httpreq);
				}
			}
		}
		std::list<DownloadHandler::TransferDataPtr>::iterator iter;
		for (iter = pendingDelete.begin();
				iter != pendingDelete.end();
				++iter) {
			(*iter)->abort();
		}
		{
			boost::unique_lock<boost::mutex> transfer_lock(mActiveTransferLock);
			while (!mActiveTransfers.empty()) {
				mCleanupCV.wait(transfer_lock);
			}
		}
	}

	virtual void getData(const RemoteFileId &downloadFileId,
			const Range &requestedRange,
			const TransferCallback &callback) {

		RequestInfo info(downloadFileId, requestedRange, callback);
		std::list<RequestInfo>::iterator infoIter;
		{
			boost::unique_lock<boost::mutex> transfer_lock(mActiveTransferLock);
			infoIter = mActiveTransfers.insert(mActiveTransfers.end(), info);
		}

		if (mServicesLookup == NULL) {
			ListOfServicesPtr services(new ListOfServices);
			services->push_back(downloadFileId.uri().context());
			gotServices(infoIter, services);
		} else {
			mServicesLookup->lookupService(downloadFileId.uri().context(),
					std::tr1::bind(&NetworkTransfer::gotServices, this, infoIter, _1));
		}
	}
};

}
}

#endif /* HTTPTRANSFER_HPP_ */
