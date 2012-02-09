/*  Sirikata
 *  StorageTestBase.hpp
 *
 *  Copyright (c) 2011, Stanford University
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

#ifndef __SIRIKATA_STORAGE_TEST_BASE_HPP__
#define __SIRIKATA_STORAGE_TEST_BASE_HPP__

#include <cxxtest/TestSuite.h>
#include <sirikata/oh/Storage.hpp>

class StorageTestBase
{
protected:
    typedef OH::Storage::ReadSet ReadSet;

    static const OH::Storage::Bucket _buckets[2];

    int _initialized;

    String _plugin;
    String _type;
    String _args;

    PluginManager _pmgr;
    OH::Storage* _storage;

    // Helpers for getting event loop setup/torn down
    Trace::Trace* _trace;
    ODPSST::ConnectionManager* _sstConnMgr;
    OHDPSST::ConnectionManager* _ohSSTConnMgr;
    Network::IOService* _ios;
    Network::IOStrand* _mainStrand;
    Network::IOWork* _work;
    ObjectHostContext* _ctx;

    // Processing happens in another thread, but the main test thread
    // needs to wait for the test to complete before proceeding. This
    // CV notifies the main thread as each callback finishes.
    boost::mutex _mutex;
    boost::condition_variable _cond;

public:
    StorageTestBase(String plugin, String type, String args)
     : _initialized(0),
       _plugin(plugin),
       _type(type),
       _args(args),
       _storage(NULL),
       _trace(NULL),
       _sstConnMgr(NULL),
       _ohSSTConnMgr(NULL),
       _mainStrand(NULL),
       _work(NULL),
       _ctx(NULL)
    {}

    void setUp() {
        if (!_initialized) {
            _initialized = 1;
            _pmgr.load(_plugin);
        }

        // Storage is tied to the main event loop, which requires quite a bit of setup
        ObjectHostID oh_id(1);
        _trace = new Trace::Trace("dummy.trace");
        _ios = new Network::IOService("StorageTest");
        _mainStrand = _ios->createStrand("StorageTest");
        _work = new Network::IOWork(*_ios, "StorageTest");
        Time start_time = Timer::now(); // Just for stats in ObjectHostContext.
        Duration duration = Duration::zero(); // Indicates to run forever.
        _sstConnMgr = new ODPSST::ConnectionManager();
        _ohSSTConnMgr = new OHDPSST::ConnectionManager();

        _ctx = new ObjectHostContext("test", oh_id, _sstConnMgr, _ohSSTConnMgr, _ios, _mainStrand, _trace, start_time, duration);

        _storage = OH::StorageFactory::getSingleton().getConstructor(_type)(_ctx, _args);

        _ctx->add(_ctx);
        _ctx->add(_storage);

        // Run the context, but we need to make sure it only lives in other
        // threads, otherwise we'd block up this one.  We include 4 threads to
        // exercise support for multiple threads.
        _ctx->run(4, Context::AllNew);
    }

    void tearDown() {
        delete _work;
        _work = NULL;

        _ctx->shutdown();

        _trace->prepareShutdown();

        delete _storage;
        _storage = NULL;

        delete _ctx;
        _ctx = NULL;

        _trace->shutdown();
        delete _trace;
        _trace = NULL;

        delete _sstConnMgr;
        _sstConnMgr = NULL;

        delete _ohSSTConnMgr;
        _ohSSTConnMgr = NULL;

        delete _mainStrand;
        _mainStrand = NULL;
        delete _ios;
        _ios = NULL;
    }

    void checkReadValuesImpl(bool expected_success, ReadSet expected, bool success, ReadSet* rs) {
        TS_ASSERT_EQUALS(expected_success, success);
        if (!success || !expected_success) return;

        if (!rs) {
            TS_ASSERT_EQUALS(expected.size(), 0);
            return;
        }

        TS_ASSERT_EQUALS(expected.size(), rs->size());
        for(ReadSet::iterator it = expected.begin(); it != expected.end(); it++) {
            String key = it->first; String value = it->second;
            TS_ASSERT(rs->find(key) != rs->end());
            TS_ASSERT_EQUALS((*rs)[key], value);
        }
    }

    void checkReadValues(bool expected_success, ReadSet expected, bool success, ReadSet* rs) {
        boost::unique_lock<boost::mutex> lock(_mutex);
        checkReadValuesImpl(expected_success, expected, success, rs);
        delete rs;
        _cond.notify_one();
    }

    void checkReadCountValueImpl(bool expected_success, int32 expected_count, bool success, int32 count) {
        TS_ASSERT_EQUALS(expected_success, success);
        if (!success || !expected_success) return;

        TS_ASSERT_EQUALS(expected_count, count);
        if (!count || !expected_count) return;
    }

    void checkCountValue(bool expected_success, int32 expected_count, bool success, int32 count) {
        boost::unique_lock<boost::mutex> lock(_mutex);
        checkReadCountValueImpl(expected_success, expected_count, success, count);
        _cond.notify_one();
    }

    void waitForTransaction() {
        boost::unique_lock<boost::mutex> lock(_mutex);
        _cond.wait(lock);
    }

    void testSetupTeardown() {
        TS_ASSERT(_storage);
    }

    void testSingleWrite() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->write(_buckets[0], "a", "abcde",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testSingleRead() {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        ReadSet rs;
        rs["a"] = "abcde";
        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs, _1, _2)
        );
        waitForTransaction();
    }

    void testSingleInvalidRead() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->read(_buckets[0], "key_does_not_exist",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testSingleCompare() {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->compare(_buckets[0], "a", "abcde",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testSingleInvalidCompare() {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->compare(_buckets[0], "a", "wrong_key_value",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testSingleErase() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->erase(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();

        // After erase, a read should fail
        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testMultiWrite() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "a", "abcde");
        _storage->write(_buckets[0], "f", "fghij");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testMultiRead() {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        ReadSet rs;
        rs["a"] = "abcde";
        rs["f"] = "fghij";
        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "a");
        _storage->read(_buckets[0], "f");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs, _1, _2)
        );
        waitForTransaction();
    }

    void testMultiInvalidRead() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "key_does_not_exist");
        _storage->read(_buckets[0], "another_key_does_not_exist");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testMultiSomeInvalidRead() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "a");
        _storage->read(_buckets[0], "another_key_does_not_exist");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testMultiErase() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->erase(_buckets[0], "a");
        _storage->erase(_buckets[0], "f");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "f",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();

    }

    void testAtomicWrite() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "a", "abcde");
        _storage->write(_buckets[0], "f", "fghij");
        _storage->read(_buckets[0], "x");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();

        ReadSet rs;
        rs["a"] = "abcde";
        rs["f"] = "fghij";

        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "a");
        _storage->read(_buckets[0], "f");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, rs, _1, _2)
        );
        waitForTransaction();

        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "a", "abcde");
        _storage->write(_buckets[0], "f", "fghij");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "a");
        _storage->read(_buckets[0], "f");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs, _1, _2)
        );
        waitForTransaction();
    }

    void testAtomicWriteErase() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        ReadSet rs1;
        rs1["a"] = "abcde";

        ReadSet rs2;
        rs2["k"] = "klmno";

        // To erase and ensure avoiding an error, first write to make
        // sure the value is there, then erase. Otherwise, erasing
        // could fail on the first run (empty db) and succeed
        // otherwise.
        _storage->write(_buckets[0], "k", "x",
             std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
         );
         waitForTransaction();
        _storage->erase(_buckets[0], "k",
             std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
         );
         waitForTransaction();

        _storage->beginTransaction(_buckets[0]);
        _storage->erase(_buckets[0], "a");
        _storage->erase(_buckets[0], "f");
        _storage->read(_buckets[0], "x");
        _storage->write(_buckets[0], "k", "klmno");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs1, _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "k",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, rs2, _1, _2)
        );
        waitForTransaction();

        _storage->beginTransaction(_buckets[0]);
        _storage->erase(_buckets[0], "a");
        _storage->erase(_buckets[0], "f");
        _storage->write(_buckets[0], "k", "klmno");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, rs1, _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "k",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs2, _1, _2)
        );
        waitForTransaction();
    }

    void testRangeRead() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "map:name:a", "abcde");
        _storage->write(_buckets[0], "map:name:f", "fghij");
        _storage->write(_buckets[0], "map:name:k", "klmno");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );

        ReadSet rs;
        rs["map:name:a"] = "abcde";
        rs["map:name:f"] = "fghij";
        rs["map:name:k"] = "klmno";

        _storage->rangeRead(_buckets[0],"map:name", "map:name@",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs, _1, _2)
        );
        waitForTransaction();
    }

    void testCount() {
    	// NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        int32 count = 3;
    	_storage->count(_buckets[0],"map:name", "map:name@",
    		std::tr1::bind(&StorageTestBase::checkCountValue, this, true, count, _1, _2)
    	);

    	waitForTransaction();
    }

    void testRangeErase() {
    	// NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

    	_storage->rangeErase(_buckets[0],"map:name", "map:name@",
    		std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
    	);
    	waitForTransaction();

        _storage->rangeRead(_buckets[0],"map:name", "map:name@",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }


    void testAllTransaction() {
        // Test that all operations work together in a transaction. Importantly,
        // this ensures that the expected read operations come back when you
        // group different types of operations.
        //
        // This was added because some operations weren't following the
        // transactional semantics properly, so it tries to test that operations
        // aren't performed independently of the rest of the transaction.

        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        // Setup some data
        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "foo", "bar");
        _storage->write(_buckets[0], "baz", "baz");
        _storage->write(_buckets[0], "map:all:a", "abcde");
        _storage->write(_buckets[0], "map:all:f", "fghij");
        _storage->write(_buckets[0], "map:all:k", "klmno");
        _storage->write(_buckets[0], "map:todelete:a", "xyzw");
        _storage->write(_buckets[0], "map:todelete:c", "xyzw");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();

        ReadSet rs;
        // These are values we'll read via rangeRead
        rs["map:all:a"] = "abcde";
        rs["map:all:f"] = "fghij";
        rs["map:all:k"] = "klmno";
        // And an individual key we'll read back.
        rs["foo"] = "bar";

        // We don't care about order here, just that if reads occur before the
        // commitTransaction, they won't properly be included in the results.
        _storage->beginTransaction(_buckets[0]);
        // Read existing data
        _storage->read(_buckets[0], "foo");
        // Range read existing data
        _storage->rangeRead(_buckets[0], "map:all", "map:all@");
        // Write some new data, verified below
        _storage->write(_buckets[0], "y", "z");
        // Delete individual key
        _storage->erase(_buckets[0], "baz");
        // Delete a range of data
        _storage->rangeErase(_buckets[0], "map:todelete", "map:todelete@");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs, _1, _2)
        );
        waitForTransaction();


        // Verify data written in above transaction
        ReadSet rs2;
        rs2["y"] = "z";
        _storage->read(_buckets[0], "y",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs2, _1, _2)
        );
        waitForTransaction();
    }


    void resetRollbackData() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        // Setup some data
        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "foo", "bar");
        _storage->write(_buckets[0], "baz", "baz");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }
    void verifyRollbackData(String key, String value) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        ReadSet rs;
        rs[key] = value;
        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], key);
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, true, rs, _1, _2)
        );
        waitForTransaction();
    }
    void testRollback() {
        // Test that rollbacks work correctly if a request fails.
        // This is a bit tricky to test because operations can be
        // grouped so that, e.g., reads and compares are verified
        // before any changes occur.
        //
        // To test it, we run a few different tests involving
        // combinations of valid and invalid writes/erases. To try to
        // be robust, we also change the order of the keys being
        // operated on so that the order in which the parts of the
        // request are performed doesn't affect the results: one of
        // each pair should fail (we do this by testing with, e.g.,
        // keys [a, b], and [c, b]).

        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        // Valid write, invalid erase -> should get original value
        resetRollbackData();
        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "foo", "xxx");
        _storage->erase(_buckets[0], "car");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();
        verifyRollbackData("foo", "bar");
        // Variant with different key ordering
        resetRollbackData();
        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "baz", "xxx");
        _storage->erase(_buckets[0], "car");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, false, ReadSet(), _1, _2)
        );
        waitForTransaction();
        verifyRollbackData("baz", "baz");
    }

};

const OH::Storage::Bucket StorageTestBase::_buckets[2] = {
    OH::Storage::Bucket("72a537a6-c18f-48fe-a97d-90b40727062e", OH::Storage::Bucket::HumanReadable()),
    OH::Storage::Bucket("12345678-9101-1121-3141-516171819202", OH::Storage::Bucket::HumanReadable())
};

#endif //__SIRIKATA_STORAGE_TEST_BASE_HPP__
