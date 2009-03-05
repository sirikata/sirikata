/*  Sirikata Transfer -- Content Distribution Network
 *  EventTransferManager.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
/*  Created on: Feb 21, 2009 */

#ifndef SIRIKATA_EventTransferManager_HPP__
#define SIRIKATA_EventTransferManager_HPP__

#include "task/EventManager.hpp"
#include "CacheLayer.hpp"
#include "NameLookupManager.hpp"
#include "TransferManager.hpp"
#include "util/AtomicTypes.hpp"
#include "UploadHandler.hpp"

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace Sirikata {
namespace Transfer {


class EventTransferManager : public TransferManager {

	CacheLayer *mFirstTransferLayer;
	NameLookupManager *mNameLookup;
	Task::GenEventManager *mEventSystem;

	ProtocolRegistry<NameUploadHandler> * mNameUploadReg;
	ProtocolRegistry<UploadHandler> * mUploadReg;

	volatile bool mCleanup;
	AtomicValue<int> mPendingCleanup;
	boost::condition_variable mCleanupCV;

	typedef std::tr1::unordered_multimap<Fingerprint, Range, Fingerprint::Hasher> DownloadRangeMap;
	DownloadRangeMap mActiveTransfers;

	boost::mutex mMutex;

	void downloadFinished(const RemoteFileId &remoteid, const Range &range, const SparseData *downloadedData) {
		bool found = true;
		{
			boost::unique_lock<boost::mutex> l(mMutex);
			DownloadRangeMap::const_iterator iter =
				mActiveTransfers.find(remoteid.fingerprint());
			while (iter != mActiveTransfers.end() && (*iter).first == remoteid.fingerprint()) {
				if (downloadedData ?
						((*iter).second.isContainedBy(*downloadedData)) :
						((*iter).second == range)) {
					// Return value is the iterator immediately following q prior to the erasure.
					iter = mActiveTransfers.erase(iter);
					found = true;
				} else {
					++iter;
				}
			}
		}

		if (found) {
			Status stat;
			if (downloadedData) {
				stat = SUCCESS;
			} else {
				stat = FAIL_DOWNLOAD;
			}
			mEventSystem->fire(DownloadEventPtr(new DownloadEvent(stat, remoteid, downloadedData)));
		} else {
			SILOGNOCR(transfer,error,"Finished download for " << remoteid.uri() << " but event has already fired...");
		}
	}

	void downloadNameLookupSuccess(const EventListener &listener, const Range &range, const RemoteFileId *remoteid) {
		if (!remoteid) {
			listener(DownloadEventPtr(new DownloadEvent(FAIL_NAMELOOKUP, RemoteFileId(), NULL)));
		} else {
			boost::unique_lock<boost::mutex> l(mMutex);

			if (mCleanup) {
				listener(DownloadEventPtr(new DownloadEvent(FAIL_SHUTDOWN, *remoteid, NULL)));
				if (--mPendingCleanup == 0) {
					if (mCleanup) {
						mCleanupCV.notify_one(); // We are the last one to finish.
					}
				}
				return;
			}

			DownloadRangeMap::const_iterator iter =
				mActiveTransfers.find(remoteid->fingerprint());
			bool found = false;
			while (iter != mActiveTransfers.end() && (*iter).first == remoteid->fingerprint()) {
				if (range.isContainedBy((*iter).second)) {
					SILOG(transfer,debug,"ISContained " << range << " " << (*iter).second);
					found = true;
					break;
				}
				++iter;
			}
			SILOG(transfer,debug,"Getting " << range << " (found = " << found << ")");
			mEventSystem->subscribe(DownloadEvent::getIdPair(*remoteid), listener);
			if (!found) {
				mActiveTransfers.insert(
					DownloadRangeMap::value_type(remoteid->fingerprint(), range));
				CacheLayer * theCacheLayer = mFirstTransferLayer;
				// release lock after subscribing to ensure that event does not fire until now.
				l.unlock();

				/* Don't want to own a lock here, but also need to make sure
				 * nobody deletes mFirstTransferLayer while we are in the call.
				 * For any asynchronous callbacks, CacheLayer will handle cleanup,
				 * but for synchronous callbacks it is our responsibility.
				 */

				// FIXME: mFirstTransferLayer may be destroyed if cleanup is called after previous check.
                //using std::tr1::placeholders::_1;
				theCacheLayer->getData(*remoteid, range,
					std::tr1::bind(&EventTransferManager::downloadFinished, this, *remoteid, range, _1));

			}

		}

		if (--mPendingCleanup == 0) {
			if (mCleanup) {
				mCleanupCV.notify_one(); // We are the last one to finish.
			}
		}
	}

	Task::EventResponse uploadDataFinishedDoName(const URI &name,
			const RemoteFileId &hash,
			const EventListener &listener,
			Task::EventPtr ev) {
		UploadEventPtr upev (std::tr1::static_pointer_cast<UploadEvent>(ev));
		if (upev->success()) {
			uploadName(name, hash, listener);
		} else {
			listener(ev);
		}
		return Task::EventResponse::del();
	}

	void doUploadName(
			const URI &name,
			const RemoteFileId &hash,
			unsigned int which,
			bool success,
			bool successThisRound,
			const ListOfServicesPtr &services) {
		success = success && successThisRound;
		if (!services || which >= services->size()) {
			if (!services || services->empty()) {
				success = false;
			}
			Status stat = success ? SUCCESS : FAIL_NAMEUPLOAD;
			Task::EventPtr ev (new UploadEvent(stat, name));
			mEventSystem->fire(ev);
			return;
		}

		std::tr1::shared_ptr<NameUploadHandler> nameHandler;
		std::string proto = mNameUploadReg->lookup((*services)[which].first.proto(), nameHandler);
		URI uploadURI ((*services)[which].first, name.filename());
		uploadURI.getContext().setProto(proto);
		nameHandler->uploadName(NULL, (*services)[which].second, uploadURI, hash,
				std::tr1::bind(&EventTransferManager::doUploadName, this,
						name, hash, which+1, success, _1, services));
	}

	void doUploadData(
			const RemoteFileId &hash,
			const SparseData &toUpload,
			unsigned int which,
			bool success,
			bool successThisRound,
			const ListOfServicesPtr &services) {
		success = success && successThisRound;
		if (!services || which >= services->size()) {
			if (!services || services->empty()) {
				success = false;
			}
			Status stat = success ? SUCCESS : FAIL_UPLOAD;
			Task::EventPtr ev (new UploadEvent(stat, hash.uri()));
			mEventSystem->fire(ev);
			return;
		}

		std::tr1::shared_ptr<UploadHandler> dataHandler;
		std::string proto = mUploadReg->lookup((*services)[which].first.proto(), dataHandler);
		URI uploadURI ((*services)[which].first, hash.uri().filename());
		uploadURI.getContext().setProto(proto);
		dataHandler->upload(NULL, (*services)[which].second, uploadURI, toUpload,
				std::tr1::bind(&EventTransferManager::doUploadData, this,
						hash, toUpload, which+1, success, _1, services));
	}
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

	EventTransferManager(CacheLayer *download,
				NameLookupManager *nameLookup,
				Task::GenEventManager *eventSystem,
				ProtocolRegistry<NameUploadHandler> * uploadNameReg,
				ProtocolRegistry<UploadHandler> * uploadDataReg)
			: mFirstTransferLayer(download),
			  mNameLookup(nameLookup),
			  mEventSystem(eventSystem),
			  mNameUploadReg(uploadNameReg),
			  mUploadReg(uploadDataReg),
			  mCleanup(false),
			  mPendingCleanup(0) {
	}

	virtual void cleanup() {
		{
			boost::unique_lock<boost::mutex> cleanuplock(mMutex);

			// mEventSystem is still used to send out last-minute notifications of aborted transfers.
			// But delete the rest:
			mCleanup = true;
			mNameLookup = NULL;
			mFirstTransferLayer = NULL;

			while (mPendingCleanup.read() != 0) {
				// Wait for any downloadNameLookupSuccess callbacks to return.
				mCleanupCV.wait(cleanuplock);
			}
		}

	}

	virtual ~EventTransferManager() {
		boost::unique_lock<boost::mutex> cleanuplock(mMutex);
		//mEventSystem->fire(DownloadEventPtr(new DownloadEvent(FAIL_SHUTDOWN, RemoteFileId(), NULL)));
	}

	virtual void purgeFromCache(const Fingerprint &fprint) {
		mFirstTransferLayer->purgeFromCache(fprint);
	}

	virtual void download(const URI &name, const EventListener &listener, const Range &range) {
		// TODO: Handle multiple name lookups at the same time to the same filename. Is this possible? worth doing?
		++mPendingCleanup;
		mNameLookup->lookupHash(name, std::tr1::bind(&EventTransferManager::downloadNameLookupSuccess, this, listener, range, _1));
	}

	virtual void downloadByHash(const RemoteFileId &name, const EventListener &listener, const Range &range) {
		// This is the same as if the download() function got a cached name lookup response.
		++mPendingCleanup;
		downloadNameLookupSuccess(listener, range, &name);
	}

	virtual void upload(const URI &name,
			const RemoteFileId &hash,
			const SparseData &toUpload,
			const EventListener &listener) {
		if (!mNameUploadReg || !mUploadReg) {
			listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, name)));
		}
		/*
		if (!exists(hash.uri())) {
			uploadByHash(hash, toUpload, listener);
		}
		uploadName(name, hash, listener);
		*/
		uploadByHash(hash, toUpload,
				std::tr1::bind(&EventTransferManager::uploadDataFinishedDoName,
						this, name, hash, listener, _1));
	}

	virtual void uploadName(const URI &name,
			const RemoteFileId &hash,
			const EventListener &listener) {
		if (!mNameUploadReg) {
			listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, name)));
		}
		mEventSystem->subscribe(UploadEvent::getIdPair(name), listener);

		mNameUploadReg->lookupService(
			name.context(),
			std::tr1::bind(&EventTransferManager::doUploadName, this,
					name, hash, 0, true, true, _1));
	}

	virtual void uploadByHash(const RemoteFileId &hash,
			const SparseData &toUpload,
			const EventListener &listener) {
		if (!mUploadReg) {
			listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, hash.uri())));
		}
		mEventSystem->subscribe(UploadEvent::getIdPair(hash.uri()), listener);

		mUploadReg->lookupService(
			hash.uri().context(),
			std::tr1::bind(&EventTransferManager::doUploadData, this,
					hash, toUpload, 0, true, true, _1));
	}

};

}
}

#endif /* SIRIKATA_EventTransferManager_HPP__ */
