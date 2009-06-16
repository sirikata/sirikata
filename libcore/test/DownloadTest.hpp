/*  Sirikata Transfer -- Content Distribution Network
 *  DownloadTest.hpp
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
/*  Created on: Feb 17, 2009 */

#include <cxxtest/TestSuite.h>
#include "transfer/EventTransferManager.hpp"
#include "task/EventManager.hpp"
#include "task/WorkQueue.hpp"
#include "transfer/CachedServiceLookup.hpp"
#include "transfer/ServiceManager.hpp"
#include "transfer/CachedNameLookupManager.hpp"
#include "transfer/NetworkCacheLayer.hpp"
#include "transfer/MemoryCacheLayer.hpp"
#include "transfer/HTTPDownloadHandler.hpp"
#include "transfer/URI.hpp"
#include "transfer/LRUPolicy.hpp"

using namespace Sirikata;
class DownloadTest : public CxxTest::TestSuite {
	typedef Transfer::TransferManager TransferManager;
	typedef Transfer::NameLookupManager NameLookupManager;
	typedef Transfer::CacheLayer CacheLayer;
	typedef Transfer::URI URI;
	typedef Transfer::Range Range;
	typedef Transfer::URIContext URIContext;

	TransferManager *mTransferManager;
	TransferManager *mCachedTransferManager;
	CacheLayer *mMemoryCache;
	Transfer::CachePolicy *mMemoryCachePolicy;
	CacheLayer *mCachedNetworkCache;
	// No disk cache because it doesn't need to be persistent.
	CacheLayer *mNetworkCache;
	NameLookupManager *mNameLookup;
	NameLookupManager *mCachedNameLookup;
	Transfer::ProtocolRegistry<Transfer::DownloadHandler> *mDownloadReg;
	Transfer::ProtocolRegistry<Transfer::NameLookupHandler> *mNameLookupReg;
	Transfer::ServiceLookup *mNameService;
	Transfer::ServiceLookup *mDownloadService;
	Transfer::ServiceManager<Transfer::DownloadHandler> *mDownloadMgr;
	Transfer::ServiceManager<Transfer::NameLookupHandler> *mNameLookupMgr;

	Task::WorkQueue *mWorkQueue;
	Task::GenEventManager *mEventSystem;

	boost::thread *mEventProcessThread;

	int finishedTest;
	boost::mutex wakeMutex;
	boost::condition_variable wakeCV;

	volatile bool mDestroyEventManager;

public:

	static Task::EventResponse printTransfer(Task::EventPtr evptr) {
		Transfer::DownloadEventPtr ev (std::tr1::dynamic_pointer_cast<Transfer::DownloadEvent>(evptr));

 		if (ev->success()) {
            if (SILOGP(transfer,debug)) {
                std::stringstream rangeListStream;
                Range::printRangeList(rangeListStream, (Transfer::DenseDataList&)(ev->data()));
                SILOG(transfer,debug,"Transfer " << "finished" <<
                      " (" << ((int)ev->getStatus()) << "): " << ev->uri() << rangeListStream);
            }
		} else {
            if (SILOGP(transfer,error)) {
                std::stringstream rangeListStream;
                Range::printRangeList(rangeListStream, (Transfer::DenseDataList&)(ev->data()));
                SILOG(transfer,error,"Transfer " << "failed" <<
                      " (" << (int)(ev->getStatus()) << "): " << ev->uri() << rangeListStream);
            }
		}
		return Task::EventResponse::nop();
	}

	void sleep_processEventQueue() {
		while (!mDestroyEventManager) {
			mWorkQueue->dequeueBlocking();
		}
	}

	void setUp() {
		// Make one of everything! Woohoo!

		mDestroyEventManager = false;

		mWorkQueue = new Task::ThreadSafeWorkQueue;
		mEventSystem = new Task::GenEventManager(mWorkQueue);
		mEventProcessThread = new boost::thread(std::tr1::bind(
			&DownloadTest::sleep_processEventQueue, this));

		mNameService = new Transfer::CachedServiceLookup;

		Transfer::ListOfServices *services;
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http","graphics.stanford.edu","","~danielrh/dns/names/global"),
				Transfer::ServiceParams()));
		mNameService->addToCache(URIContext("meerkat","","",""), Transfer::ListOfServicesPtr(services));

		mDownloadService = new Transfer::CachedServiceLookup;
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http","graphics.stanford.edu","","~danielrh/uploadsystem/files/global"),
				Transfer::ServiceParams()));
		mDownloadService->addToCache(URIContext("mhash","","",""), Transfer::ListOfServicesPtr(services));

		mNameLookupReg = new Transfer::ProtocolRegistry<Transfer::NameLookupHandler>;
		std::tr1::shared_ptr<Transfer::HTTPDownloadHandler> httpHandler(new Transfer::HTTPDownloadHandler);
		mNameLookupReg->setHandler("http", httpHandler);
		mNameLookupMgr = new Transfer::ServiceManager<Transfer::NameLookupHandler>(mNameService, mNameLookupReg);
		mNameLookup = new Transfer::NameLookupManager(mNameLookupMgr);

		mDownloadReg = new Transfer::ProtocolRegistry<Transfer::DownloadHandler>;
		mDownloadReg->setHandler("http", httpHandler);
		mDownloadMgr = new Transfer::ServiceManager<Transfer::DownloadHandler>(mDownloadService, mDownloadReg);
		mNetworkCache = new Transfer::NetworkCacheLayer(NULL, mDownloadMgr);

		mTransferManager = new Transfer::EventTransferManager(mNetworkCache, mNameLookup, mEventSystem,NULL,NULL,NULL);

		// Uses the same event system, so don't combine the cached and non-cached ones into a single test.
		mCachedNetworkCache = new Transfer::NetworkCacheLayer(NULL, mDownloadMgr);
		mMemoryCachePolicy = new Transfer::LRUPolicy(1000000);
		mMemoryCache = new Transfer::MemoryCacheLayer(mMemoryCachePolicy, mCachedNetworkCache);
		mCachedNameLookup = new Transfer::CachedNameLookupManager(mNameLookupMgr);
		mCachedTransferManager = new Transfer::EventTransferManager(mMemoryCache, mCachedNameLookup, mEventSystem,NULL,NULL,NULL);

		mEventSystem->subscribe(Transfer::DownloadEventId, &printTransfer, Task::EARLY);

		finishedTest = 0;
	}
	void tearDown() {
		// Deletion order:
		mTransferManager->cleanup();
		mCachedTransferManager->cleanup();
		// First delete name lookups
		delete mNameLookup;
		delete mCachedNameLookup;
		delete mNameLookupMgr;
		delete mNameLookupReg;
		delete mNameService;

		// First delete data transfer -- this will cause all
		// delete non-cached CacheLayer
		delete mNetworkCache;
			// also the cached CacheLayer
		delete mMemoryCache;
		delete mMemoryCachePolicy;
		delete mCachedNetworkCache; // CacheLayer's deleted from first to last.
		delete mDownloadMgr; // Download service manager must be deleted after the NetworkCacheLayer.
		delete mDownloadReg;
		delete mDownloadService;
		delete mTransferManager;
		delete mCachedTransferManager;

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

	Task::EventResponse downloadFinished(Task::EventPtr evbase) {
		Transfer::DownloadEventPtr ev = std::tr1::dynamic_pointer_cast<Transfer::DownloadEvent> (evbase);

		TS_ASSERT_EQUALS(ev->getStatus(), Transfer::TransferManager::SUCCESS);

		notifyOne();

		return Task::EventResponse::del();
	}

	void testSimpleFileDownload() {
		using std::tr1::placeholders::_1;
		mTransferManager->download(URI("meerkat:///arcade.mesh"),
			std::tr1::bind(&DownloadTest::downloadFinished, this, _1), Range(true));
		mTransferManager->download(URI("meerkat:///arcade.mesh"),
			std::tr1::bind(&DownloadTest::downloadFinished, this, _1), Range(true));
		waitFor(2);
	}

	Task::EventResponse downloadCheckRange(const Range &toCheck, Task::EventPtr evbase) {
		Transfer::DownloadEventPtr ev = std::tr1::dynamic_pointer_cast<Transfer::DownloadEvent> (evbase);
		SILOG(transfer,debug,toCheck);
		if (ev->data().empty()) {
			std::stringstream msg;
			msg << "Got empty data for " << toCheck << "!";
			TS_FAIL(msg.str());
			notifyOne();
			return Task::EventResponse::del();
		} else if (ev->data().contains(toCheck)) {
			TS_ASSERT_EQUALS(ev->getStatus(), Transfer::TransferManager::SUCCESS);
			notifyOne();
			return Task::EventResponse::del();
		} else {
			return Task::EventResponse::nop();
		}
	}

	void testCombiningRangedFileDownload() {
        using std::tr1::placeholders::_1;
		mTransferManager->download(URI("meerkat:///arcade.mesh"),
			std::tr1::bind(&DownloadTest::downloadCheckRange, this, Range(0,3, Transfer::BOUNDS), _1), Range(0,3, Transfer::BOUNDS));
		mTransferManager->download(URI("meerkat:///arcade.mesh"),
			std::tr1::bind(&DownloadTest::downloadCheckRange, this, Range(true), _1), Range(true));
		mTransferManager->download(URI("meerkat:///arcade.mesh"),
			std::tr1::bind(&DownloadTest::downloadCheckRange, this, Range(2, true), _1), Range(2, true));
		waitFor(3);
	}
};
