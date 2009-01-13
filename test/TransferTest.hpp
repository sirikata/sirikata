#include <cxxtest/TestSuite.h>
#include "transfer/DiskCache.hpp"
#include "transfer/MemoryCache.hpp"
#include "transfer/NetworkTransfer.hpp"
#include "transfer/TransferManager.hpp"
#include "transfer/TransferData.hpp"
#include "transfer/LRUPolicy.hpp"

#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace Iridium;

/*  Created on: Jan 10, 2009 */

#define EXAMPLE_HASH "55ca2e1659205d752e4285ce927dcda19b039ca793011610aaee3e5ab250ff80"

class TransferManagerTestSuite : public CxxTest::TestSuite
{
	Transfer::CacheLayer *mMemoryCache;
	Transfer::CacheLayer *mDiskCache;
	Transfer::CacheLayer *mHTTPTransfer;
	Transfer::TransferManager *mTransfer;
	volatile int finishedTest;

	boost::mutex wakeMutex;
	boost::condition_variable wakeCV;
public:
	TransferManagerTestSuite () {
	}

	virtual void setUp() {
		finishedTest = 0;
		mHTTPTransfer = new Transfer::NetworkTransfer(NULL);
		mDiskCache = new Transfer::DiskCache(new Transfer::LRUPolicy(32000), "diskCache/", mHTTPTransfer);
		mMemoryCache = new Transfer::MemoryCache(new Transfer::LRUPolicy(3200), mDiskCache);
		mTransfer = new Transfer::TransferManager(mMemoryCache);
	}

	virtual void tearDown() {
		delete mTransfer;
		delete mMemoryCache;
		delete mDiskCache;
		delete mHTTPTransfer;
		finishedTest = 0;
	}

	static TransferManagerTestSuite * createSuite( void ) {
		return new TransferManagerTestSuite();
	}
	static void destroySuite(TransferManagerTestSuite * k) {
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

	void callbackExampleCom(const Transfer::URI &uri, const Transfer::SparseData *myData) {
		TS_ASSERT(myData != NULL);
		if (myData) {
			myData->debugPrint(std::cout);
			const Transfer::DenseData &densedata = (*myData->begin());
			TS_ASSERT_EQUALS (SHA256::computeDigest(densedata.data(), densedata.length()), uri.fingerprint());
		} else {
			std::cout << "fail!" << std::endl;
		}
		std::cout << "Finished displaying!" << std::endl;
		notifyOne();
		std::cout << "Finished callback" << std::endl;
	}

	bool doExampleComTest( void ) {
		Transfer::URI exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), "http://example.com/");

		bool async = mTransfer->download(exampleComUri,
				boost::bind(&TransferManagerTestSuite::callbackExampleCom, this, exampleComUri, _1),
				Transfer::Range(true));

		waitFor(1);

		std::cout << "Finished example.com test" << std::endl;
		return async;
	}

	void testExampleCom( void ) {
		doExampleComTest();
	}
	void testExampleCom2( void ) {
		const bool asynchronous = true;
		// test disk cache.
		std::cout << "Testing disk cache..."<<std::endl;
		TS_ASSERT(doExampleComTest() == asynchronous);

		// test memory cache.
		rename("diskCache/" EXAMPLE_HASH,"t");
		// ensure it is not using the disk cache.
		std::cout << "Testing memory cache..."<<std::endl;
		TS_ASSERT(doExampleComTest() != asynchronous);
		rename("t","diskCache/" EXAMPLE_HASH);
	}

	void simpleCallback(const Transfer::SparseData *myData) {
		TS_ASSERT(myData!=NULL);
		if (myData) {
			myData->debugPrint(std::cout);
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
		Transfer::URI testUri (SHA256::computeDigest("01234"), "http://www.google.com/");
		Transfer::URI testUri2 (SHA256::computeDigest("56789"), "http://www.google.com/intl/en_ALL/images/logo.gif");
		Transfer::URI exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), "http://example.com/");

		Transfer::TransferCallback simpleCB = boost::bind(&TransferManagerTestSuite::simpleCallback, this, _1);
		Transfer::TransferCallback checkNullCB = boost::bind(&TransferManagerTestSuite::checkNullCallback, this, _1);

		mTransfer->purgeFromCache(testUri.fingerprint());
		mTransfer->purgeFromCache(testUri2.fingerprint());
		mTransfer->download(testUri, checkNullCB,
				Transfer::Range(true));
		mTransfer->download(testUri2, checkNullCB,
				Transfer::Range(true));

		// example.com should be in disk cache--make sure it waits for the request.
		mTransfer->download(exampleComUri, simpleCB,
				Transfer::Range(true));

		// do not wait--we want to clean up these requests.
	}

	void _testRange( void ) {
		Transfer::TransferCallback simpleCB = boost::bind(&TransferManagerTestSuite::simpleCallback, this, _1);

		Transfer::URI exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), "http://example.com/");
		mTransfer->purgeFromCache(exampleComUri.fingerprint());
		mTransfer->download(exampleComUri, simpleCB,
				Transfer::Range(2, 8, Transfer::BOUNDS));
		mTransfer->download(exampleComUri, simpleCB,
				Transfer::Range(8, 14, Transfer::BOUNDS));
		mTransfer->download(exampleComUri, simpleCB,
				Transfer::Range(6, 14, Transfer::BOUNDS));

		waitFor(3);

		bool wasAsync = mTransfer->download(exampleComUri, simpleCB,
				Transfer::Range(6, 8, Transfer::BOUNDS));

		// This request should now be cached.
		TS_ASSERT(wasAsync == false);
		waitFor(4);

		wasAsync = mTransfer->download(exampleComUri, simpleCB,
				Transfer::Range(2, 14, Transfer::BOUNDS));
		TS_ASSERT(wasAsync == false);
		waitFor(5);

		// downloads from 2 to the end.
		wasAsync = mTransfer->download(exampleComUri,
				boost::bind(&TransferManagerTestSuite::checkOneDenseDataCallback, this, _1),
				Transfer::Range(2, true));
		TS_ASSERT(wasAsync == true);
		waitFor(6);

		wasAsync = mTransfer->download(exampleComUri,
				boost::bind(&TransferManagerTestSuite::checkOneDenseDataCallback, this, _1),
				Transfer::Range(true));
		TS_ASSERT(wasAsync == true);
		waitFor(7);

		wasAsync = mTransfer->download(exampleComUri, simpleCB,
				Transfer::Range(2, 14, Transfer::BOUNDS));
		TS_ASSERT(wasAsync == false);
		waitFor(8);

		wasAsync = mTransfer->download(exampleComUri, simpleCB,
				Transfer::Range(1, true, Transfer::BOUNDS));
		TS_ASSERT(wasAsync == false);
		waitFor(9);
	}

};

using namespace Iridium;

