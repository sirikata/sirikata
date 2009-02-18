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
#include "transfer/NetworkTransfer.hpp"
#include "transfer/HTTPRequest.hpp"
#include "transfer/ProtocolRegistry.hpp"
#include "transfer/HTTPDownloadHandler.hpp"
#include "transfer/CachedServiceLookup.hpp"
#include "transfer/NameLookupManager.hpp"
#include "transfer/CachedNameLookupManager.hpp"

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
using namespace Sirikata;
//using std::tr1::shared_ptr;

class NameLookupTest : public CxxTest::TestSuite
{
	typedef Transfer::Fingerprint Fingerprint;
	typedef Transfer::URI URI;
	typedef Transfer::RemoteFileId RemoteFileId;
	typedef Transfer::URIContext URIContext;

	Transfer::ServiceLookup *mService;
	Transfer::NameLookupManager *mNameLookups;
	Transfer::CachedNameLookupManager *mCachedNameLookups;
	Transfer::ProtocolRegistry<Transfer::DownloadHandler> *mDownloadReg;
	Transfer::ProtocolRegistry<Transfer::NameLookupHandler> *mNameLookupReg;
	Transfer::CacheLayer *mTransferLayer;

	volatile int finishedTest;

	boost::mutex wakeMutex;
	boost::condition_variable wakeCV;

public:
	void setUp() {
		mService = new Transfer::CachedServiceLookup();

		Transfer::ListOfServices *services;
		services = new Transfer::ListOfServices;
		services->push_back(URIContext("testoftheemergencybroadcastsystem","blah","","")); // check that other URIs will be tried
		services->push_back(URIContext("http","localhost","","nonexistantdirectory"));
		services->push_back(URIContext("http","graphics.stanford.edu","","~danielrh/dns/names/global"));
		services->push_back(URIContext("http","localhost","","nonexistantdirectory1"));
		mService->addToCache(URIContext("meerkat","","",""), Transfer::ListOfServicesPtr(services));

		services = new Transfer::ListOfServices;
		services->push_back(URIContext("testoftheemergencybroadcastsystem2","blah","","")); // check that other URIs will be tried
		services->push_back(URIContext("http", "localhost","","nonexistantdirectory2"));
		services->push_back(URIContext("http","graphics.stanford.edu","","~danielrh/uploadsystem/files/global"));
		services->push_back(URIContext("http", "localhost","","nonexistantdirectory3"));
		mService->addToCache(URIContext("mhash","","",""), Transfer::ListOfServicesPtr(services));

		mNameLookupReg = new Transfer::ProtocolRegistry<Transfer::NameLookupHandler>;
		boost::shared_ptr<Transfer::HTTPDownloadHandler> httpHandler(new Transfer::HTTPDownloadHandler);
		mNameLookupReg->setHandler("http", httpHandler);
		mNameLookups = new Transfer::NameLookupManager(mService, mNameLookupReg);
		mCachedNameLookups = new Transfer::CachedNameLookupManager(mService, mNameLookupReg);

		mDownloadReg = new Transfer::ProtocolRegistry<Transfer::DownloadHandler>;
		mDownloadReg->setHandler("http", httpHandler);
		mTransferLayer = new Transfer::NetworkTransfer(NULL, mService, mDownloadReg);

		finishedTest = 0;
	}
	void tearDown() {
		delete mTransferLayer;
		delete mDownloadReg;
		delete mNameLookups;
		delete mCachedNameLookups;
		delete mNameLookupReg;
		delete mService;

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

	void simpleLookupCB(const Fingerprint &expected, const RemoteFileId *rfid) {
		TS_ASSERT(rfid != NULL);
		if (rfid) {
			TS_ASSERT_EQUALS(expected, rfid->fingerprint());
			std::cout << rfid->uri() << " (" << rfid->fingerprint() << ")" << std::endl;
		}

		notifyOne();
	}
	void testNameLookup() {
		//  http://graphics.stanford.edu/~danielrh/dns/names/global/ASCII.material;
		mNameLookups->lookupHash(URI(URIContext(), "meerkat:/ASCII.material"),
				std::tr1::bind(&NameLookupTest::simpleLookupCB, this,
						Fingerprint::convertFromHex("6934b2638e79135f87d36a3d4f4b53f41f349132172f3248c7c7091b59497551"), _1));

		waitFor(1);
	}
	void verifyCB(const Fingerprint &expectedHash, const Transfer::SparseData *sparseData) {
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
				fullData += std::string(length, '\0');
			}
			pos += length;
		}
		Fingerprint actualHash (Fingerprint::computeDigest(fullData));
		TS_ASSERT_EQUALS(expectedHash, actualHash);

		notifyOne(); // don't forget to do this.
	}
	void doTransferAndVerifyCB(const RemoteFileId *rfid) {
		TS_ASSERT(rfid != NULL);
		if (rfid) {
			mTransferLayer->getData(*rfid, Transfer::Range(true),
				std::tr1::bind(&NameLookupTest::verifyCB, this, rfid->fingerprint(), _1));
		}
	}
	void testDownload() {
        //  http://graphics.stanford.edu/~danielrh/dns/names/global/ASCII.material;
		mNameLookups->lookupHash(URI(URIContext(), "meerkat:/ASCII.material"),
				std::tr1::bind(&NameLookupTest::doTransferAndVerifyCB, this, _1));

		waitFor(1);
	}
};
