/*  Sirikata Transfer -- Content Distribution Network
 *  UploadTest.hpp
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
/*  Created on: Mar 3, 2009 */

#include <cxxtest/TestSuite.h>

#include "transfer/EventTransferManager.hpp"
#include "task/EventManager.hpp"
#include "task/WorkQueue.hpp"
#include "transfer/CachedServiceLookup.hpp"
#include "transfer/NetworkCacheLayer.hpp"
#include "transfer/HTTPDownloadHandler.hpp"
#include "transfer/HTTPFormUploadHandler.hpp"
#include "transfer/URI.hpp"
#include "transfer/LRUPolicy.hpp"

using namespace Sirikata;

#define SERVER "http://localhost"
// #define SERVER "http://graphics.stanford.edu/~danielrh"
class UploadTest : public CxxTest::TestSuite {
	typedef Transfer::TransferManager TransferManager;
	typedef Transfer::NameLookupManager NameLookupManager;
	typedef Transfer::CacheLayer CacheLayer;
	typedef Transfer::URI URI;
	typedef Transfer::Range Range;
	typedef Transfer::URIContext URIContext;

	Task::WorkQueue *mWorkQueue;
	TransferManager *mTransferManager;
	CacheLayer *mNetworkCache;
	NameLookupManager *mNameLookup;

	Transfer::ServiceManager<Transfer::DownloadHandler> *mDownloadServMgr;
	Transfer::ServiceManager<Transfer::NameLookupHandler> *mDownNameServMgr;
	Transfer::ServiceManager<Transfer::UploadHandler> *mUploadServMgr;
	Transfer::ServiceManager<Transfer::NameUploadHandler> *mUpNameServMgr;

	Task::GenEventManager *mEventSystem;

	boost::thread *mEventProcessThread;

	int finishedTest;
	boost::mutex wakeMutex;
	boost::condition_variable wakeCV;
	volatile bool mDestroyEventManager;

	static Task::EventResponse printTransfer(Task::EventPtr evbase) {
		Transfer::UploadEventPtr ev = std::tr1::dynamic_pointer_cast<Transfer::UploadEvent> (evbase);
		std::ostringstream msg;
		msg << "Upload " << (ev->success()?"succeeded":"failed") <<
			" (" << ((int)ev->getStatus()) << ")" << ": " << ev->uri();
		if (ev->success()) {
			SILOG(transfer,debug,msg);
		} else {
			SILOG(transfer,error,msg);
		}

		return Task::EventResponse::nop();
	}

	void sleep_processEventQueue() {
		while (!mDestroyEventManager) {
			mWorkQueue->dequeueBlocking();
		}
	}

	void setUp() {
		Transfer::ListOfServices *services;
		Transfer::ServiceParams params;
		Transfer::ProtocolRegistry<Transfer::DownloadHandler> *mDownloadReg;
		Transfer::ProtocolRegistry<Transfer::NameLookupHandler> *mDownNameReg;
		Transfer::ProtocolRegistry<Transfer::UploadHandler> *mUploadReg;
		Transfer::ProtocolRegistry<Transfer::NameUploadHandler> *mUpNameReg;
		Transfer::ServiceLookup *mDownNameService;
		Transfer::ServiceLookup *mDownloadService;
		Transfer::ServiceLookup *mUploadService;
		Transfer::ServiceLookup *mUpNameService;

		std::tr1::shared_ptr<Transfer::HTTPDownloadHandler> httpDownHandler(new Transfer::HTTPDownloadHandler);
		std::tr1::shared_ptr<Transfer::HTTPFormUploadHandler> httpUpHandler(new Transfer::HTTPFormUploadHandler);

		mDestroyEventManager = false;
		mWorkQueue = new Task::ThreadSafeWorkQueue;
		mEventSystem = new Task::GenEventManager(mWorkQueue);
		mEventProcessThread = new boost::thread(std::tr1::bind(
			&UploadTest::sleep_processEventQueue, this));

		mDownNameService = new Transfer::CachedServiceLookup;
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext(SERVER "/dns/names/global"),
				Transfer::ServiceParams()));
		mDownNameService->addToCache(URIContext("meerkat:"), Transfer::ListOfServicesPtr(services));
		mDownNameReg = new Transfer::ProtocolRegistry<Transfer::NameLookupHandler>;
		mDownNameServMgr = new Transfer::ServiceManager<Transfer::NameLookupHandler>(mDownNameService,mDownNameReg);
		mDownNameReg->setHandler("http", httpDownHandler);

		mDownloadService = new Transfer::CachedServiceLookup;
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext(SERVER "/uploadsystem/files/global"),
				Transfer::ServiceParams()));
		mDownloadService->addToCache(URIContext("mhash:"), Transfer::ListOfServicesPtr(services));
		mDownloadReg = new Transfer::ProtocolRegistry<Transfer::DownloadHandler>;
		mDownloadServMgr = new Transfer::ServiceManager<Transfer::DownloadHandler>(mDownloadService,mDownloadReg);
		mDownloadReg->setHandler("http", httpDownHandler);

		//params.set("field:user", "");
		//params.set("field:password", "authentication0");
		//params.set("value:author", "test");
		params.set("value:password", "test");
		//params.set("value:authentication0", "test");

		mUploadService = new Transfer::CachedServiceLookup;
		params.set("field:file", "uploadFile0");
		params.unset("field:filename");
		params.set("value:uploadNeed", "1");
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext(SERVER "/uploadsystem/index.php"),
				params));
		mUploadService->addToCache(URIContext("mhash:"), Transfer::ListOfServicesPtr(services));
		mUploadReg = new Transfer::ProtocolRegistry<Transfer::UploadHandler>;
		mUploadServMgr = new Transfer::ServiceManager<Transfer::UploadHandler>(mUploadService,mUploadReg);
		mUploadReg->setHandler("http", httpUpHandler);

		mUpNameService = new Transfer::CachedServiceLookup;
		params.set("field:file", "MHashFile0");
		params.set("field:filename", "highlevel0");
		params.unset("value:uploadNeed");
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext(SERVER "/dns/index.php"),
				params));
		mUpNameService->addToCache(URIContext("meerkat:"), Transfer::ListOfServicesPtr(services));
		mUpNameReg = new Transfer::ProtocolRegistry<Transfer::NameUploadHandler>;
		mUpNameServMgr = new Transfer::ServiceManager<Transfer::NameUploadHandler>(mUpNameService,mUpNameReg);
		mUpNameReg->setHandler("http", httpUpHandler);

		mNetworkCache = new Transfer::NetworkCacheLayer(NULL, mDownloadServMgr);
		mTransferManager = new Transfer::EventTransferManager(mNetworkCache, mNameLookup, mEventSystem, mDownloadServMgr, mUpNameServMgr, mUploadServMgr);

		mEventSystem->subscribe(Transfer::DownloadEventId, &printTransfer, Task::EARLY);

		finishedTest = 0;
	}

	void tearDown() {
		// Deletion order:
		mTransferManager->cleanup();
		// First delete name lookups
		delete mTransferManager;
		delete mNameLookup;
		delete mNetworkCache;

		delete mDownloadServMgr->getProtocolRegistry();
		delete mDownloadServMgr->getServiceLookup();
		delete mDownloadServMgr;
		delete mUploadServMgr->getProtocolRegistry();
		delete mUploadServMgr->getServiceLookup();
		delete mUploadServMgr;
		delete mUpNameServMgr->getProtocolRegistry();
		delete mUpNameServMgr->getServiceLookup();
		delete mUpNameServMgr;
		delete mDownNameServMgr->getProtocolRegistry();
		delete mDownNameServMgr->getServiceLookup();
		delete mDownNameServMgr;

		mDestroyEventManager = true;
		mWorkQueue->enqueue(NULL);
		mEventProcessThread->join();
		delete mEventProcessThread;
		delete mEventSystem;
		delete mWorkQueue;

		finishedTest = 0;
	}

	void waitFor(int numTests) {
		boost::unique_lock<boost::mutex> wakeup(wakeMutex);
		while (finishedTest < numTests) {
			wakeCV.wait(wakeup);
		}
	}
	void notifyOne() {
		boost::unique_lock<boost::mutex> wakeup(wakeMutex);
		finishedTest++;
		wakeCV.notify_one();
	}

	Task::EventResponse uploadFinished(Task::EventPtr evbase) {
		Transfer::UploadEventPtr ev = std::tr1::dynamic_pointer_cast<Transfer::UploadEvent> (evbase);

		TS_ASSERT_EQUALS(ev->getStatus(), Transfer::TransferManager::SUCCESS);

		notifyOne();

		return Task::EventResponse::del();
	}

public:
	void testSimpleUpload() {
		Transfer::SparseData sd;
		Transfer::MutableDenseDataPtr first(new Transfer::DenseData(Range(0,8,Transfer::LENGTH)));
		std::memcpy(first->writableData(), "12345678", first->length());
		Transfer::MutableDenseDataPtr second(new Transfer::DenseData(Range(6,9,Transfer::LENGTH)));
		std::memcpy(second->writableData(), "78abcdefg", second->length());
		sd.addValidData(second);
		sd.addValidData(first);
		Transfer::Fingerprint fp = sd.computeFingerprint();
		using std::tr1::placeholders::_1;
		mTransferManager->upload(URI("meerkat:///test"), URIContext("mhash:"), sd.flatten(),
				std::tr1::bind(&UploadTest::uploadFinished, this, _1), true);
		//mTransferManager->uploadName("meerkat:///test",Transfer::RemoteFileId(fp, URIContext("mhash:")),
		//		std::tr1::bind(&UploadTest::uploadFinished, this, _1));

		waitFor(1);
	}

};
