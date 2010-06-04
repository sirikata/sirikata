/*  Sirikata Tests -- Sirikata Test Suite
 *  NameLookupTest.hpp
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
/*  Created on: Feb 7, 2009 */


#include <cxxtest/TestSuite.h>
#include <sirikata/core/transfer/NetworkCacheLayer.hpp>
#include <sirikata/core/transfer/HTTPRequest.hpp>
#include <sirikata/core/transfer/ProtocolRegistry.hpp>
#include <sirikata/core/transfer/HTTPDownloadHandler.hpp>
#include <sirikata/core/transfer/CachedServiceLookup.hpp>
#include <sirikata/core/transfer/ServiceManager.hpp>
#include <sirikata/core/transfer/NameLookupManager.hpp>
#include <sirikata/core/transfer/CachedNameLookupManager.hpp>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
using namespace Sirikata;


class NameLookupTest : public CxxTest::TestSuite
{
	typedef Transfer::Fingerprint Fingerprint;
	typedef Transfer::URI URI;
	typedef Transfer::RemoteFileId RemoteFileId;
	typedef Transfer::URIContext URIContext;

	Transfer::ServiceLookup *mNameService;
	Transfer::ServiceLookup *mDownloadService;
	Transfer::NameLookupManager *mNameLookups;
	Transfer::CachedNameLookupManager *mCachedNameLookups;
	Transfer::ProtocolRegistry<Transfer::DownloadHandler> *mDownloadReg;
	Transfer::ProtocolRegistry<Transfer::NameLookupHandler> *mNameLookupReg;
	Transfer::ServiceManager<Transfer::DownloadHandler> *mDownloadMgr;
	Transfer::ServiceManager<Transfer::NameLookupHandler> *mNameLookupMgr;
	Transfer::CacheLayer *mTransferLayer;

	volatile int finishedTest;

	boost::mutex wakeMutex;
	boost::condition_variable wakeCV;

public:
	void setUp() {
		Transfer::ListOfServices *services;

		mNameService = new Transfer::CachedServiceLookup();
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("testoftheemergencybroadcastsystem","blah","",""),
				Transfer::ServiceParams())); // check that other URIs will be tried
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http","localhost","","nonexistantdirectory"),
				Transfer::ServiceParams()));
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http","graphics.stanford.edu","","~danielrh/dns/names/global"),
				Transfer::ServiceParams()));
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http","localhost","","nonexistantdirectory1"),
				Transfer::ServiceParams()));
		mNameService->addToCache(URIContext("meerkat","","",""), Transfer::ListOfServicesPtr(services));

		mDownloadService = new Transfer::CachedServiceLookup();
		services = new Transfer::ListOfServices;
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("testoftheemergencybroadcastsystem2","blah","",""),
				Transfer::ServiceParams())); // check that other URIs will be tried
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http", "localhost","","nonexistantdirectory2"),
				Transfer::ServiceParams()));
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http","graphics.stanford.edu","","~danielrh/uploadsystem/files/global"),
				Transfer::ServiceParams()));
		services->push_back(Transfer::ListOfServices::value_type(
				URIContext("http", "localhost","","nonexistantdirectory3"),
				Transfer::ServiceParams()));
		mDownloadService->addToCache(URIContext("mhash","","",""), Transfer::ListOfServicesPtr(services));

		mNameLookupReg = new Transfer::ProtocolRegistry<Transfer::NameLookupHandler>;
		std::tr1::shared_ptr<Transfer::HTTPDownloadHandler> httpHandler(new Transfer::HTTPDownloadHandler);
		mNameLookupReg->setHandler("http", httpHandler);
		mNameLookupMgr = new Transfer::ServiceManager<Transfer::NameLookupHandler>(mNameService,mNameLookupReg);
		mNameLookups = new Transfer::NameLookupManager(mNameLookupMgr);
		mCachedNameLookups = new Transfer::CachedNameLookupManager(mNameLookupMgr);

		mDownloadReg = new Transfer::ProtocolRegistry<Transfer::DownloadHandler>;
		mDownloadReg->setHandler("http", httpHandler);
		mDownloadMgr = new Transfer::ServiceManager<Transfer::DownloadHandler>(mDownloadService,mDownloadReg);
		mTransferLayer = new Transfer::NetworkCacheLayer(NULL, mDownloadMgr);

		finishedTest = 0;
	}
	void tearDown() {
		delete mTransferLayer;
		delete mDownloadMgr;
		delete mDownloadReg;
		delete mNameLookups;
		delete mCachedNameLookups;
		delete mNameLookupMgr;
		delete mNameLookupReg;
		delete mNameService;
		delete mDownloadService;

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

	void simpleLookupCB(const Fingerprint &expected, const URI&uri, const RemoteFileId *rfid) {
		TS_ASSERT(rfid != NULL);
		if (rfid) {
			TS_ASSERT_EQUALS(expected, rfid->fingerprint());
			std::cout << rfid->uri() << " (" << rfid->fingerprint() << ")" << std::endl;
		}

		notifyOne();
	}
	void testNameLookup() {
		using std::tr1::placeholders::_1;
		using std::tr1::placeholders::_2;
		//  http://graphics.stanford.edu/~danielrh/dns/names/global/ASCII.material;
		mNameLookups->lookupHash(URI(URIContext(), "meerkat:/ASCII.material"),
				std::tr1::bind(&NameLookupTest::simpleLookupCB, this,
                               Fingerprint::convertFromHex("6934b2638e79135f87d36a3d4f4b53f41f349132172f3248c7c7091b59497551"), _1,_2));

		waitFor(1);
	}
	void verifyCB(const Fingerprint &expectedHash, const Transfer::SparseData *sparseData) {
		if (!sparseData) {
			TS_FAIL("Failed to download " + expectedHash.convertToHexString() + " from CacheLayer");
			notifyOne();
			return;
		}
		std::string fullData;
		Transfer::cache_usize_type pos = 0, length;
		while (true) {
			const unsigned char *data = sparseData->dataAt(pos, length);
			if (!data && !length) {
				break;
			}
			if (data) {
				fullData += std::string(data, data+length);
			} else {
				fullData += std::string((size_t)length, '\0');
			}
			pos += length;
		}
		Fingerprint actualHash (Fingerprint::computeDigest(fullData));
		TS_ASSERT_EQUALS(expectedHash, actualHash);

		notifyOne(); // don't forget to do this.
	}
	void doTransferAndVerifyCB(const URI&, const RemoteFileId *rfid) {
		using std::tr1::placeholders::_1;
		if (rfid) {
			mTransferLayer->getData(*rfid, Transfer::Range(true),
                                    std::tr1::bind(&NameLookupTest::verifyCB, this, rfid->fingerprint(), _1));
		} else {
			TS_FAIL("Name Lookup Failed!");
			notifyOne();
		}
	}
	void testDownload() {
		using std::tr1::placeholders::_1;
		using std::tr1::placeholders::_2;

        //  http://graphics.stanford.edu/~danielrh/dns/names/global/ASCII.material;
		mNameLookups->lookupHash(URI(URIContext(), "meerkat:/ASCII.material"),
                                 std::tr1::bind(&NameLookupTest::doTransferAndVerifyCB, this, _1,_2));

		waitFor(1);
	}
};
