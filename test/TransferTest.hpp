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

class TransferManagerTestSuite : public CxxTest::TestSuite
{
	Transfer::CacheLayer *mMemoryCache;
	Transfer::CacheLayer *mDiskCache;
	Transfer::CacheLayer *mHTTPTransfer;
	Transfer::TransferManager *mTransfer;

	boost::mutex wakeMutex;
	volatile bool finishedTest;
	boost::condition_variable wakeCV;
public:
	TransferManagerTestSuite () {
	}

	virtual void setUp() {
		mHTTPTransfer = new Transfer::NetworkTransfer(NULL);
		mDiskCache = new Transfer::DiskCache(new Transfer::LRUPolicy(32000), "diskCache/", mHTTPTransfer);
		mMemoryCache = new Transfer::MemoryCache(new Transfer::LRUPolicy(32000000), mDiskCache);
		mTransfer = new Transfer::TransferManager(mMemoryCache);
	}

	virtual void tearDown() {
		delete mTransfer;
		delete mMemoryCache;
		delete mDiskCache;
		delete mHTTPTransfer;
	}

	static TransferManagerTestSuite * createSuite( void ) {
		return new TransferManagerTestSuite();
	}
	static void destroySuite(TransferManagerTestSuite * k) {
		delete k;
	}

	void callbackExampleCom(volatile bool *finishedTest, const Transfer::URI &uri, const Transfer::SparseData *myData) {
		TS_ASSERT(myData != NULL);
		if (myData) {
			myData->debugPrint(std::cout);
			const Transfer::DenseData &densedata = (*myData->begin());
			TS_ASSERT_EQUALS (densedata.length(), uri.length());
			std::cout << SHA256::computeDigest(densedata.data(), densedata.length()) <<std::endl;
			std::cout << uri.fingerprint() <<std::endl;
		} else {
			std::cout << "fail!" << std::endl;
		}
		std::cout << "Finished displaying!" << std::endl;
		{
			boost::unique_lock<boost::mutex> wakeup(wakeMutex);
			*finishedTest = true;
			wakeCV.notify_one();
		}
		std::cout << "Finished callback" << std::endl;
	}

	void testExampleCom( void ) {
		const char *expected = "55ca2e1659205d752e4285ce927dcda19b039ca793011610aaee3e5ab250ff80";
		Transfer::URI exampleComUri (SHA256::convertFromHex(expected), 438, "http://example.com/");
		std::cout << "shasum should be " <<  expected << std::endl;
		volatile bool finishedTest = false;

		mTransfer->download(exampleComUri,
				boost::bind(&TransferManagerTestSuite::callbackExampleCom, this, &finishedTest, exampleComUri, _1),
				Transfer::Range(0, exampleComUri.length()));

		{
			boost::unique_lock<boost::mutex> wakeup(wakeMutex);
			while (!finishedTest) {
				wakeCV.wait(wakeup);
			}
		}
		std::cout << "Finished example.com test" << std::endl;
	}
	void testExampleCom2( void ) {
		// test disk cache.
		testExampleCom();

		// test memory cache.
		rename("diskCache/55ca2e1659205d752e4285ce927dcda19b039ca793011610aaee3e5ab250ff80","t");
		// ensure it is not using the disk cache.
		testExampleCom();
		rename("t","diskCache/55ca2e1659205d752e4285ce927dcda19b039ca793011610aaee3e5ab250ff80");
	}

};

using namespace Iridium;

