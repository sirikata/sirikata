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

	ServiceManager<DownloadHandler> * mDownloadServ;
	ServiceManager<NameUploadHandler> * mNameUploadServ;
	ServiceManager<UploadHandler> * mUploadServ;

	volatile bool mCleanup;
	AtomicValue<int> mPendingCleanup;
	boost::condition_variable mCleanupCV;

	typedef std::tr1::unordered_multimap<Fingerprint, Range, Fingerprint::Hasher> DownloadRangeMap;
	typedef std::tr1::unordered_set<std::string> UploadMap;
	DownloadRangeMap mActiveTransfers;
	UploadMap mActiveUploads;

	boost::mutex mMutex;
	boost::mutex mUploadMutex;

	void downloadFinished(const RemoteFileId &remoteid, const Range &range, const SparseData *downloadedData) {
		bool found = true;
		{
			boost::unique_lock<boost::mutex> l(mMutex);
			DownloadRangeMap::const_iterator iter =
				mActiveTransfers.find(remoteid.fingerprint());
			while (iter != mActiveTransfers.end() && (*iter).first == remoteid.fingerprint()) {
				if (downloadedData ?
						downloadedData->contains((*iter).second) :
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
	        doDownloadByHash(listener, range, remoteid, false);
	}
    Task::SubscriptionId doDownloadByHash(const EventListener &listener, const Range &range, const RemoteFileId *remoteid, bool requestID) {
		Task::SubscriptionId ret = Task::SubscriptionIdClass::null();
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
				return ret;
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
			if (requestID) {
			     ret = mEventSystem->subscribeId(DownloadEvent::getIdPair(*remoteid), listener);
			} else {
			     mEventSystem->subscribe(DownloadEvent::getIdPair(*remoteid), listener);
			}
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
		return ret;
	}

	Task::EventResponse uploadDataFinishedDoName(const URI &name,
			const RemoteFileId &hash,
			const EventListener &listener,
			Task::EventPtr ev) {
		UploadEventPtr upev (std::tr1::static_pointer_cast<UploadEvent>(ev));
		if (upev->success()) {
			uploadName(name, hash, listener);
		} else {
			listener(Task::EventPtr(new UploadEvent(upev->getStatus(), name, UploadEventId)));
		}
		return Task::EventResponse::del();
	}

	void doUploadName(
			const URI &name,
			const RemoteFileId &hash,
			bool success,
			bool successThisRound,
			ServiceIterator *services) {
		success = success && successThisRound;

		std::tr1::shared_ptr<NameUploadHandler> nameHandler;
		URI uploadURI;
		ServiceParams params;
		if (mNameUploadServ->getNextProtocol(services,
				successThisRound?ServiceIterator::SUCCESS:ServiceIterator::GENERAL_ERROR,
				name,uploadURI,params,nameHandler)) {

			nameHandler->uploadName(NULL, params, uploadURI, hash,
					std::tr1::bind(&EventTransferManager::doUploadName, this,
							name, hash, success, _1, services));
		} else {
			Status stat = success ? SUCCESS : FAIL_NAMEUPLOAD;
			Task::EventPtr ev (new UploadEvent(stat, name, UploadEventId));
            mNameLookup->addToCache(name, hash);
			mEventSystem->fire(ev);
		}
	}

	void doUploadData(
			const RemoteFileId &hash,
			const DenseDataPtr &toUpload,
			bool success,
			bool successThisRound,
			ServiceIterator *services) {
		success = success && successThisRound;

		std::tr1::shared_ptr<UploadHandler> dataHandler;
		URI uploadURI;
		ServiceParams params;
		if (mUploadServ->getNextProtocol(services,
				successThisRound?ServiceIterator::SUCCESS:ServiceIterator::GENERAL_ERROR,
				hash.uri(),uploadURI,params,dataHandler)) {

			dataHandler->upload(NULL, params, uploadURI, toUpload,
					std::tr1::bind(&EventTransferManager::doUploadData, this,
							hash, toUpload, success, _1, services));
		} else {
		// FIXME: Report FAIL_UPLOAD if services list is empty, or no protocols are supported.
		// Distinguish from case that all services were uploaded to?? Does success even matter?
/*
			if (!services || services->empty()) {
				success = false;
			}
*/
			Status stat = success ? SUCCESS : FAIL_UPLOAD;
			Task::EventPtr ev (new UploadEvent(stat, hash.uri(), UploadDataEventId));
			{
				boost::unique_lock<boost::mutex> l(mMutex);
				UploadMap::const_iterator iter = mActiveUploads.find(hash.uri().toString());
				assert (iter != mActiveUploads.end());
				mActiveUploads.erase(iter);
			}
			mFirstTransferLayer->addToCache(hash.fingerprint(), toUpload);
			mEventSystem->fire(ev);
		}
	}
	void doUploadByHash(const RemoteFileId &hash,
			const DenseDataPtr &toUpload,
			const EventListener &listener) {
		mUploadServ->lookupService(
			hash.uri().context(),
			std::tr1::bind(&EventTransferManager::doUploadData, this,
			               hash, toUpload, true, true, _1));
	}
	void uploadIfNotExists(const RemoteFileId &hash,
			const DenseDataPtr &toUpload,
			const EventListener &listener,
			bool existsThisRound,
			bool existsInAll,
			bool firstRound,
			ServiceIterator *services) {

		existsInAll = existsInAll && existsThisRound;

		std::tr1::shared_ptr<DownloadHandler> dataHandler;
		URI dloadURI;
		ServiceParams params;

		// Always pass success to these
		if (mDownloadServ->getNextProtocol(services,
				ServiceIterator::SUCCESS,
				hash.uri(),dloadURI,params,dataHandler)) {

			dataHandler->exists(NULL, dloadURI, std::tr1::bind(
				&EventTransferManager::uploadIfNotExists, this, hash, toUpload, listener, _1, existsInAll, false, services));
		} else {
			if (existsInAll && !firstRound) {
				Task::EventPtr ev (new UploadEvent(SUCCESS, hash.uri(), UploadDataEventId));
				{
					boost::unique_lock<boost::mutex> l(mMutex);
					UploadMap::const_iterator iter = mActiveUploads.find(hash.uri().toString());
					assert (iter != mActiveUploads.end());
					mActiveUploads.erase(iter);
				}
				mEventSystem->fire(ev);
			} else {
				doUploadByHash(hash, toUpload, listener);
			}
		}
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
				ServiceManager<DownloadHandler> * downloadReg,
				ServiceManager<NameUploadHandler> * uploadNameReg,
				ServiceManager<UploadHandler> * uploadDataReg)
			: mFirstTransferLayer(download),
			  mNameLookup(nameLookup),
			  mEventSystem(eventSystem),
			  mDownloadServ(downloadReg),
			  mNameUploadServ(uploadNameReg),
			  mUploadServ(uploadDataReg),
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
		mNameLookup->lookupHash(name, std::tr1::bind(&EventTransferManager::downloadNameLookupSuccess, this, listener, range, _2));
	}

	virtual SubscriptionId downloadByHash(const RemoteFileId &name, const EventListener &listener, const Range &range) {
		// This is the same as if the download() function got a cached name lookup response.
		++mPendingCleanup;
		return doDownloadByHash(listener, range, &name, true);
	}

	virtual void downloadName(const URI &nameURI,
			const std::tr1::function<void(const URI &nameURI,const RemoteFileId *fingerprint)> &listener) {
		mNameLookup->lookupHash(nameURI, listener);
	}

	virtual void upload(const URI &name,
			const RemoteFileId &hash,
			const DenseDataPtr &toUpload,
			const EventListener &listener,
			bool forceIfExists) {
		if (!mNameUploadServ || !mUploadServ) {
			listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, name, UploadEventId)));
		}
		/*
		if (!exists(hash.uri())) {
			uploadByHash(hash, toUpload, listener);
		}
		uploadName(name, hash, listener);
		*/
		uploadByHash(hash, toUpload,
				std::tr1::bind(&EventTransferManager::uploadDataFinishedDoName,
						this, name, hash, listener, _1), forceIfExists);
	}

	virtual void uploadName(const URI &name,
			const RemoteFileId &hash,
			const EventListener &listener) {
		if (!mNameUploadServ) {
			listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, name, UploadEventId)));
		}
		mEventSystem->subscribe(UploadEvent::getIdPair(name, UploadEventId), listener);

		mNameUploadServ->lookupService(
			name.context(),
			std::tr1::bind(&EventTransferManager::doUploadName, this,
					name, hash, true, true, _1));
	}

	virtual void uploadByHash(const RemoteFileId &hash,
			const DenseDataPtr &toUpload,
			const EventListener &listener,
			bool forceIfExists) {
		if (!mUploadServ) {
			listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, hash.uri(), UploadDataEventId)));
		}
		{
			boost::unique_lock<boost::mutex> l(mMutex);
			mEventSystem->subscribe(UploadEvent::getIdPair(hash.uri(), UploadDataEventId), listener);
			UploadMap::const_iterator iter = mActiveUploads.find(hash.uri().toString());
			if (iter == mActiveUploads.end()) {
				if (!forceIfExists) {
					mDownloadServ->lookupService(
						hash.uri().context(),
						std::tr1::bind(&EventTransferManager::uploadIfNotExists, this,
						               hash, toUpload, listener, true, true, true, _1));
				} else {
					doUploadByHash(hash, toUpload, listener);
				}
				mActiveUploads.insert(hash.uri().toString());
			}
		}
	}

};

}
}

#endif /* SIRIKATA_EventTransferManager_HPP__ */
