/*  Sirikata Transfer -- Content Distribution Network
 *  TransferTest.hpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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
/*  Created on: Jan 18, 2010 */

#include <cxxtest/TestSuite.h>

#include "util/Thread.hpp"
#include "task/EventManager.hpp"
#include "task/WorkQueue.hpp"

#include "transfer/URI.hpp"
#include "transfer/CachedServiceLookup.hpp"

#include "transfer/ServiceManager.hpp"
#include "transfer/NameLookupManager.hpp"
#include "transfer/HTTPDownloadHandler.hpp"

#include "transfer/NetworkCacheLayer.hpp"

#include "transfer/TransferPool.hpp"
#include "transfer/RemoteFileMetadata.hpp"
#include "transfer/TransferMediator.hpp"

using namespace Sirikata;

boost::condition_variable done;
boost::mutex mut;
int numClis = 0;

class SampleClient {

	Transfer::TransferMediator * mTransferMediator;
	std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;
	const std::string mClientID;
	std::vector<Transfer::URI> mURIList;

public:

	SampleClient(Transfer::TransferMediator * transferMediator, const std::string & clientID, std::vector<Transfer::URI> uriList) :
		mTransferMediator(transferMediator), mClientID(clientID), mURIList(uriList) {
		boost::unique_lock<boost::mutex> lock(mut);
		numClis++;
	}

	void run() {
		using std::tr1::placeholders::_1;

		//Register with the transfer mediator!
		mTransferPool = mTransferMediator->registerClient(mClientID, std::tr1::bind(&SampleClient::transferFinished, this, _1));

		for(int i=0; i<10; i++) {
			for(std::vector<Transfer::URI>::iterator it = mURIList.begin(); it != mURIList.end(); it++) {
				//SILOG(transfer, debug, mClientID << " adding " << it->toString());
				float pri = rand()/(float(RAND_MAX)+1);
				std::tr1::shared_ptr<Transfer::TransferRequest> req(new Transfer::MetadataRequest(*it, pri));
				mTransferPool->addRequest(req);
			}

			//sleep between 500ms and 1000ms
			boost::this_thread::sleep(boost::posix_time::milliseconds(rand() % 500 + 500));
		}

		boost::unique_lock<boost::mutex> lock(mut);
		numClis--;
		if(numClis <= 0) {
			done.notify_all();
		}
	}

	Task::EventResponse transferFinished(Task::EventPtr evbase) {
		//Transfer::DownloadEventPtr ev = std::tr1::dynamic_pointer_cast<Transfer::DownloadEvent> (evbase);

		//TS_ASSERT_EQUALS(ev->getStatus(), Transfer::TransferManager::SUCCESS);

		//notifyOne();

		return Task::EventResponse::del();
	}

};

class TransferTest : public CxxTest::TestSuite {

	//for ease of use
	typedef Transfer::URIContext URIContext;

	//Event-based / Thread stuff
	Task::WorkQueue *mWorkQueue;
	Task::GenEventManager *mEventSystem;
	Thread *mEventProcessThread;

	//Used to map from meerkat:// names to their mhash
	Transfer::ServiceLookup *mNameService;
	//Used to map from mhash:// to where to actually download
	Transfer::ServiceLookup *mDownloadService;

	//Name lookup handlers
	Transfer::ProtocolRegistry<Transfer::NameLookupHandler> *mNameLookupReg;
	Transfer::ServiceManager<Transfer::NameLookupHandler> *mNameLookupMgr;
	Transfer::NameLookupManager *mNameLookup;

	//Handlers for actual downloads
	Transfer::ProtocolRegistry<Transfer::DownloadHandler> *mDownloadReg;
	Transfer::ServiceManager<Transfer::DownloadHandler> *mDownloadMgr;
	Transfer::CacheLayer *mNetworkCache;

	//Set to true when event manager should shut down
	volatile bool mDestroyEventManager;

	//Mediates transfers between subsystems (graphics, physics, etc)
	Transfer::TransferMediator *mTransferMediator;

	SampleClient * mSampleClient1;
	SampleClient * mSampleClient2;
	SampleClient * mSampleClient3;

	Thread * mMediatorThread;
	Thread * mClientThread1;
	Thread * mClientThread2;
	Thread * mClientThread3;
	ThreadSafeQueue<int> mTestQueue;

public:

	void setUp() {
		mDestroyEventManager = false;

		//Initialize main worker thread for event-based stuff
		mWorkQueue = new Task::ThreadSafeWorkQueue;
		mEventSystem = new Task::GenEventManager(mWorkQueue);
		mEventProcessThread = new Thread(std::tr1::bind(
			&TransferTest::sleep_processEventQueue, this));


		//Makes it so we can look up files with meerkat:// URIs and find out their mhash value from /dns/names/global
		mNameService = new Transfer::CachedServiceLookup;
		Transfer::ListOfServices *services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http","graphics.stanford.edu","","~danielrh/dns/names/global"),
				Transfer::ServiceParams()));
		mNameService->addToCache(URIContext("meerkat","","",""), Transfer::ListOfServicesPtr(services));

		//Makes it so we can find the content of files in /files/global using their mhash value
		mDownloadService = new Transfer::CachedServiceLookup;
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http","graphics.stanford.edu","","~danielrh/uploadsystem/files/global"),
				Transfer::ServiceParams()));
		mDownloadService->addToCache(URIContext("mhash","","",""), Transfer::ListOfServicesPtr(services));

		//We need to register a name lookup handler. HTTPDownloadHandler implements a name lookup via HTTP so register it here
		mNameLookupReg = new Transfer::ProtocolRegistry<Transfer::NameLookupHandler>;
		std::tr1::shared_ptr<Transfer::HTTPDownloadHandler> httpHandler(new Transfer::HTTPDownloadHandler);
		mNameLookupReg->setHandler("http", httpHandler);
		mNameLookupMgr = new Transfer::ServiceManager<Transfer::NameLookupHandler>(mNameService, mNameLookupReg);
		mNameLookup = new Transfer::NameLookupManager(mNameLookupMgr);

		//Register a download handler with the protocol registry. Note that NetworkCacheLayer is not really a cache
		mDownloadReg = new Transfer::ProtocolRegistry<Transfer::DownloadHandler>;
		mDownloadReg->setHandler("http", httpHandler);
		mDownloadMgr = new Transfer::ServiceManager<Transfer::DownloadHandler>(mDownloadService, mDownloadReg);
		mNetworkCache = new Transfer::NetworkCacheLayer(NULL, mDownloadMgr);

		//Create a transfer mediator to use for client transfer requests
		mTransferMediator = new Transfer::TransferMediator(mNetworkCache, mNameLookup, mEventSystem);

		mMediatorThread = new Thread(std::tr1::bind(&Transfer::TransferMediator::mediatorThread, mTransferMediator));

		//5 urls
		std::vector<Transfer::URI> list1;
		list1.push_back(Transfer::URI("meerkat:///arcade.mesh"));
		list1.push_back(Transfer::URI("meerkat:///arcade.os"));
		list1.push_back(Transfer::URI("meerkat:///blackclear.png"));
		list1.push_back(Transfer::URI("meerkat:///blackcube_bk.png"));
		list1.push_back(Transfer::URI("meerkat:///blackcube_dn.png"));

		//4 new urls, 1 overlap from list1
		std::vector<Transfer::URI> list2;
		list2.push_back(Transfer::URI("meerkat:///arcade.mesh"));
		list2.push_back(Transfer::URI("meerkat:///blackcube_fr.png"));
		list2.push_back(Transfer::URI("meerkat:///blackcube_lf.png"));
		list2.push_back(Transfer::URI("meerkat:///blackcube_rt.png"));
		list2.push_back(Transfer::URI("meerkat:///blackcube_up.png"));

		//3 new urls, 1 overlap from list1, 1 overlap from list2
		std::vector<Transfer::URI> list3;
		list3.push_back(Transfer::URI("meerkat:///arcade.os"));
		list3.push_back(Transfer::URI("meerkat:///blackcube_fr.png"));
		list3.push_back(Transfer::URI("meerkat:///Sea.material"));
		list3.push_back(Transfer::URI("meerkat:///OldTV.material"));
		list3.push_back(Transfer::URI("meerkat:///OldMovie.material"));

		mSampleClient1 = new SampleClient(mTransferMediator, "sample1", list1);
		mSampleClient2 = new SampleClient(mTransferMediator, "sample2", list2);
		mSampleClient3 = new SampleClient(mTransferMediator, "sample3", list3);

		mClientThread1 = new Thread(std::tr1::bind(&SampleClient::run, mSampleClient1));
		mClientThread2 = new Thread(std::tr1::bind(&SampleClient::run, mSampleClient2));
		mClientThread3 = new Thread(std::tr1::bind(&SampleClient::run, mSampleClient3));
	}

	void tearDown() {
		delete mNetworkCache;
		delete mDownloadMgr;
		delete mDownloadReg;

		delete mNameLookup;
		delete mNameLookupReg;
		delete mNameLookupMgr;

		delete mNameService;
		delete mDownloadService;

		mDestroyEventManager = true;
		mWorkQueue->enqueue(NULL);
		mEventProcessThread->join();

		delete mEventProcessThread;
		delete mEventSystem;
		delete mWorkQueue;

		mClientThread1->join();
		mClientThread2->join();
		mClientThread3->join();
		mMediatorThread->join();
	}

	void sleep_processEventQueue() {
		while (!mDestroyEventManager) {
			mWorkQueue->dequeueBlocking();
		}
	}

	void testSomething() {
		srand ( time(NULL) );
		boost::unique_lock<boost::mutex> lock(mut);
		done.wait(lock);
		mTransferMediator->cleanup();
	}

};
