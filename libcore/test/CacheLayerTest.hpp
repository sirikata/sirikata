/*  Sirikata Tests -- Sirikata Test Suite
 *  TransferTest.hpp
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

#include <cxxtest/TestSuite.h>
#include "transfer/DiskCacheLayer.hpp"
#include "transfer/MemoryCacheLayer.hpp"
#include "transfer/NetworkCacheLayer.hpp"
#include "transfer/TransferData.hpp"
#include "transfer/LRUPolicy.hpp"

#include "transfer/ProtocolRegistry.hpp"
#include "transfer/HTTPDownloadHandler.hpp"


using namespace Sirikata;

/*  Created on: Jan 10, 2009 */

#define EXAMPLE_HASH "55ca2e1659205d752e4285ce927dcda19b039ca793011610aaee3e5ab250ff80"

class CacheLayerTestSuite : public CxxTest::TestSuite
{
	//typedef Transfer::RemoteFileId RemoteFileId;
	typedef Transfer::URI URI;
	typedef Transfer::URIContext URIContext;
	typedef Transfer::CacheLayer CacheLayer;
	std::vector< CacheLayer*> mCacheLayers;
	std::vector< Transfer::CachePolicy*> mCachePolicy;
	volatile int finishedTest;

	Transfer::ProtocolRegistry<Transfer::DownloadHandler> *mProtoReg;

	boost::mutex wakeMutex;
	boost::condition_variable wakeCV;
public:
	CacheLayerTestSuite () {
	}

	virtual void setUp() {
		finishedTest = 0;
		mCacheLayers.clear();
		mProtoReg = new Transfer::ProtocolRegistry<Transfer::DownloadHandler>();
		std::tr1::shared_ptr<Transfer::HTTPDownloadHandler> httpHandler(new Transfer::HTTPDownloadHandler);
		mProtoReg->setHandler("http", httpHandler);
	}

	CacheLayer *createTransferLayer(CacheLayer *next=NULL) {
		// NULL service lookup--we are using a simple http URL.
		CacheLayer *layer = new Transfer::NetworkCacheLayer(next, mProtoReg);
		mCacheLayers.push_back(layer);
		return layer;
	}
	CacheLayer *createDiskCache(CacheLayer *next = NULL,
				int size=32000,
				std::string dir="diskCache") {
		Transfer::CachePolicy *policy = new Transfer::LRUPolicy(size);
		CacheLayer *layer = new Transfer::DiskCacheLayer(
							policy,
							dir,
							next);
		mCacheLayers.push_back(layer);
		mCachePolicy.push_back(policy);
		return layer;
	}
	CacheLayer *createMemoryCache(CacheLayer *next = NULL,
				int size=3200) {
		Transfer::CachePolicy *policy = new Transfer::LRUPolicy(size);
		CacheLayer *layer = new Transfer::MemoryCacheLayer(
							policy,
							next);
		mCacheLayers.push_back(layer);
		mCachePolicy.push_back(policy);
		return layer;
	}
	CacheLayer *createSimpleCache(bool memory, bool disk, bool http) {
		CacheLayer *firstCache = NULL;
		if (http) {
			firstCache = createTransferLayer(firstCache);
		}
		if (disk) {
			firstCache = createDiskCache(firstCache);
		}
		if (memory) {
			firstCache = createMemoryCache(firstCache);
		}
		return firstCache;
	}
	void tearDownCache() {
		for (std::vector<Transfer::CacheLayer*>::reverse_iterator iter =
					 mCacheLayers.rbegin(); iter != mCacheLayers.rend(); ++iter) {
			delete (*iter);
		}
		mCacheLayers.clear();
		for (std::vector<Transfer::CachePolicy*>::iterator iter =
					 mCachePolicy.begin(); iter != mCachePolicy.end(); ++iter) {
			delete (*iter);
		}
		mCachePolicy.clear();
	}

	virtual void tearDown() {
		tearDownCache();
		finishedTest = 0;
		delete mProtoReg;
	}

	static CacheLayerTestSuite * createSuite( void ) {
		return new CacheLayerTestSuite();
	}
	static void destroySuite(CacheLayerTestSuite * k) {
		delete k;
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

	void callbackExampleCom(const Transfer::RemoteFileId &uri, const Transfer::SparseData *myData) {
		TS_ASSERT(myData != NULL);
		if (myData) {
			myData->debugPrint(std::cout);
			const Transfer::DenseData &densedata = (*myData->begin());
			TS_ASSERT_EQUALS (SHA256::computeDigest(densedata.data(), (size_t)densedata.length()), uri.fingerprint());
		} else {
			std::cout << "fail!" << std::endl;
		}
		std::cout << "Finished displaying!" << std::endl;
		notifyOne();
		std::cout << "Finished callback" << std::endl;
	}

	void doExampleComTest( CacheLayer *transfer ) {
		Transfer::RemoteFileId exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), URI(URIContext(), "http://example.com/"));
        using std::tr1::placeholders::_1;
		transfer->getData(exampleComUri,
				Transfer::Range(true),
				std::tr1::bind(&CacheLayerTestSuite::callbackExampleCom, this, exampleComUri, _1));

		waitFor(1);

		std::cout << "Finished localhost test" << std::endl;
	}

	void testDiskCache_exampleCom( void ) {
		CacheLayer *testCache = createSimpleCache(true, true, true);
		testCache->purgeFromCache(SHA256::convertFromHex(EXAMPLE_HASH));
		doExampleComTest(testCache);
		tearDownCache();
		// Ensure that it is now in the disk cache.
		doExampleComTest(createSimpleCache(false, true, false));
	}
	void testMemoryCache_exampleCom( void ) {
		CacheLayer *disk = createDiskCache();
		CacheLayer *memory = createMemoryCache(disk);
		// test disk cache.
		std::cout << "Testing disk cache..."<<std::endl;
		doExampleComTest(memory);

		// test memory cache.
		memory->setNext(NULL);
		// ensure it is not using the disk cache.
		std::cout << "Testing memory cache..."<<std::endl;
		doExampleComTest(memory);
	}

	void simpleCallback(const Transfer::SparseData *myData) {
		TS_ASSERT(myData!=NULL);
		if (myData) {
			//myData->debugPrint(std::cout);
		}
		notifyOne();
	}

	void checkNullCallback(const Transfer::SparseData *myData) {
		TS_ASSERT(myData==NULL);
		notifyOne();
	}

	void checkOneDenseDataCallback(const Transfer::SparseData *myData) {
		TS_ASSERT(myData!=NULL);
		if (myData) {
			Transfer::SparseData::const_iterator iter = myData->begin();
			TS_ASSERT(++iter == myData->end());
		}
		notifyOne();
	}

	void testCleanup( void ) {
		Transfer::RemoteFileId testUri (SHA256::computeDigest("01234"), URI(URIContext(), "http://www.google.com/"));
		Transfer::RemoteFileId testUri2 (SHA256::computeDigest("56789"), URI(URIContext(), "http://www.google.com/intl/en_ALL/images/logo.gif"));
		Transfer::RemoteFileId exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), URI(URIContext(), "http://example.com/"));
		using std::tr1::placeholders::_1;
		Transfer::TransferCallback simpleCB = std::tr1::bind(&CacheLayerTestSuite::simpleCallback, this, _1);
		Transfer::TransferCallback checkNullCB = std::tr1::bind(&CacheLayerTestSuite::checkNullCallback, this, _1);

		CacheLayer *transfer = createSimpleCache(true, true, true);

		transfer->purgeFromCache(testUri.fingerprint());
		transfer->purgeFromCache(testUri2.fingerprint());
		transfer->getData(testUri, Transfer::Range(true), checkNullCB);
		transfer->getData(testUri2, Transfer::Range(true), checkNullCB);

		// localhost should be in disk cache--make sure it waits for the request.
		// disk cache is required to finish all pending requests before cleaning up.
		transfer->getData(exampleComUri, Transfer::Range(true), simpleCB);

		// do not wait--we want to clean up these requests.
	}

	void testOverlappingRange( void ) {
        using std::tr1::placeholders::_1;
		Transfer::TransferCallback simpleCB = std::tr1::bind(&CacheLayerTestSuite::simpleCallback, this, _1);
		int numtests = 0;

		CacheLayer *http = createTransferLayer();
		CacheLayer *disk = createDiskCache(http);
		CacheLayer *memory = createMemoryCache(disk);

		Transfer::RemoteFileId exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), URI(URIContext(), "http://example.com/"));
		memory->purgeFromCache(exampleComUri.fingerprint());

		std::string diskFile = "diskCache/" + exampleComUri.fingerprint().convertToHexString() + ".part";
		// First test: GetData something which will overlap with the next two.
		printf("1\n");
		http->getData(exampleComUri,
				Transfer::Range(6, 10, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=1);

		// Now getData two pieces (both of these should kick out the first one)
		printf("2/3\n");
		http->getData(exampleComUri,
				Transfer::Range(2, 8, Transfer::BOUNDS),
				simpleCB);
		http->getData(exampleComUri,
				Transfer::Range(8, 14, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=2);

		// Now check that an overlapping range from before doesn't cause problems
		printf("4\n");
		http->getData(exampleComUri,
				Transfer::Range(6, 13, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=1);

		printf("5 -> THIS ONE IS FAILING\n");
		// Everything here should be cached
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(5, 8, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=1);

		printf("6 -> THIS ONE IS FAILING?\n");
		// And the whole range we just got.
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(2, 14, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=1);

		printf("7\n");
		// getDatas from 2 to the end. -- should not be cached
		// and should overwrite all previous ranges because it is bigger.
		memory->setNext(disk);
		memory->getData(exampleComUri,
				Transfer::Range(2, true),
				simpleCB);
		waitFor(numtests+=1);
		using std::tr1::placeholders::_1;
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(2, true),
				std::tr1::bind(&CacheLayerTestSuite::checkOneDenseDataCallback, this, _1));
		waitFor(numtests+=1);

		// Whole file trumps anything else.
		memory->setNext(disk);
		memory->getData(exampleComUri,
				Transfer::Range(true),
				simpleCB);
		waitFor(numtests+=1);

		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(2, true),
				std::tr1::bind(&CacheLayerTestSuite::checkOneDenseDataCallback, this, _1));
		waitFor(numtests+=1);

		// should be cached
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(2, 14, Transfer::BOUNDS),
				simpleCB);
		waitFor(numtests+=1);

		// should be 1--end should be cached as well.
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(1, 10, Transfer::BOUNDS, true),
				simpleCB);
		waitFor(numtests+=1);
	}

	void compareCallback(Transfer::DenseDataPtr compare, const Transfer::SparseData *myData) {
		TS_ASSERT(myData!=NULL);
		if (myData) {
			Transfer::Range::base_type offset = compare->startbyte();
			while (offset < compare->endbyte()) {
				Transfer::Range::length_type len;
				const unsigned char *gotData = myData->dataAt(offset, len);
				const unsigned char *compareData = compare->dataAt(offset);
				TS_ASSERT(gotData);
				if (!gotData) {
					break;
				}
				bool equal = memcmp(compareData, gotData, len+offset<compare->endbyte() ? (size_t)len : (size_t)(compare->endbyte()-offset))==0;
				TS_ASSERT(equal);
				if (!equal) {
					std::cout << std::endl << "WANT =======" << std::endl;
					std::cout << std::string(compareData, compareData+len);
					std::cout << std::endl << "GOT: =======" << std::endl;
					std::cout << std::string(gotData, gotData+len);
					std::cout << std::endl << "-----------" << std::endl;
				}
				offset += len;
				if (offset >= compare->endbyte()) {
					break;
				}
			}
		}
		notifyOne();
	}

	void testRange( void ) {
		Transfer::RemoteFileId exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), URI(URIContext(), "http://example.com/"));
		CacheLayer *http = createTransferLayer();
		CacheLayer *disk = createDiskCache(http);
		CacheLayer *memory = createMemoryCache(disk);
		using std::tr1::placeholders::_1;
		memory->purgeFromCache(exampleComUri.fingerprint());
		{
			Transfer::DenseDataPtr expect(new Transfer::DenseData(Transfer::Range(2, 6, Transfer::LENGTH)));
			memcpy(expect->writableData(), "TML>\r\n", (size_t)expect->length());
			memory->getData(exampleComUri,
					(Transfer::Range)*expect,
					std::tr1::bind(&CacheLayerTestSuite::compareCallback, this, expect, _1));
		}
		{
			Transfer::DenseDataPtr expect(new Transfer::DenseData(Transfer::Range(8, 6, Transfer::LENGTH)));
			memcpy(expect->writableData(), "<HEAD>", (size_t)expect->length());
			memory->getData(exampleComUri,
					(Transfer::Range)*expect,
					std::tr1::bind(&CacheLayerTestSuite::compareCallback, this, expect, _1));
		}
		waitFor(2);
		{
			Transfer::DenseDataPtr expect(new Transfer::DenseData(Transfer::Range(2, 12, Transfer::LENGTH)));
			memcpy(expect->writableData(), "TML>\r\n<HEAD>", (size_t)expect->length());
			memory->setNext(NULL);
			memory->getData(exampleComUri,
					(Transfer::Range)*expect,
					std::tr1::bind(&CacheLayerTestSuite::compareCallback, this, expect, _1));
		}
		waitFor(3);
	}

};

using namespace Sirikata;
