/*  Sirikata
 *  StressTestBase.hpp
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

#ifndef __SIRIKATA_STRESS_TEST_BASE_HPP__
#define __SIRIKATA_STRESS_TEST_BASE_HPP__

#include <sys/time.h>
#include <cxxtest/TestSuite.h>
#include <sirikata/oh/Storage.hpp>
#include "DataFiles.hpp"

class StressTestBase
{
protected:
    typedef OH::Storage::ReadSet ReadSet;

    DataFiles _data;
    static const OH::Storage::Bucket _buckets[100];
    static const String _dataIndex[20];

    int _initialized;

    String _plugin;
    String _type;
    String _args;

    PluginManager _pmgr;
    OH::Storage* _storage;

    // Helpers for getting event loop setup/torn down
    Trace::Trace* _trace;
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
    StressTestBase(String plugin, String type, String args)
     : _initialized(0),
       _plugin(plugin),
       _type(type),
       _args(args),
       _data(),
       _storage(NULL),
       _trace(NULL),
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
        _ios = Network::IOServiceFactory::makeIOService();
        _mainStrand = _ios->createStrand();
        _work = new Network::IOWork(*_ios, "StressTest");
        Time start_time = Timer::now(); // Just for stats in ObjectHostContext.
        Duration duration = Duration::zero(); // Indicates to run forever.
        _ctx = new ObjectHostContext("test", oh_id, _ios, _mainStrand, _trace, start_time, duration);

        _storage = OH::StorageFactory::getSingleton().getConstructor(_type)(_ctx, _args);

        _ctx->add(_ctx);
        _ctx->add(_storage);

        // Run the context, but we need to make sure it only lives in other
        // threads, otherwise we'd block up this one.  We include 4 threads to
        // exercise support for multiple threads.
        _ctx->run(4, Context::AllNew);
	
	//std::cout<<_data.dataSet["3-100K"].size()<<std::endl;
	//std::cout<<_data.dataSet["0-10"]<<std::endl;
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

        delete _mainStrand;
        _mainStrand = NULL;
        Network::IOServiceFactory::destroyIOService(_ios);
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

    void checkSuccessImpl(bool expected_success, ReadSet expected, bool success, ReadSet* rs){
        TS_ASSERT_EQUALS(expected_success, success);
    }

    void checkSuccess(bool expected_success, ReadSet expected, bool success, ReadSet* rs){
        boost::unique_lock<boost::mutex> lock(_mutex);
        checkSuccessImpl(expected_success, expected, success, rs);
        _cond.notify_one();
    }

    void waitForTransaction() {
        boost::unique_lock<boost::mutex> lock(_mutex);
        _cond.wait(lock);
    }

    void testSetupTeardown() {
        TS_ASSERT(_storage);
    }
  
    void testSingleWrites(String length, int keyNum, int bucketNum) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
	
        timeval ts;
        gettimeofday(&ts,NULL);
        long int time1_s = ts.tv_sec;
        int time1_us=ts.tv_usec;

        String key;
        for (int i=0; i<bucketNum; i++){
            for(int j=0; j<keyNum; j++){
                key=_dataIndex[j]+"-"+length;
                _storage->write(_buckets[i], key, _data.dataSet[key],
	                        std::tr1::bind(&StressTestBase::checkSuccess, this, true, ReadSet(), _1, _2)
	                       );
                waitForTransaction();
            }
        }

        gettimeofday(&ts,NULL);
        long int time2_s = ts.tv_sec;
        int time2_us=ts.tv_usec;
        long int diff_s=time2_s - time1_s;
        int diff_us=time2_us - time1_us;
        double diff_t=diff_s*1000+diff_us/(double)1000;
        std::cout<<"Write time: "<<diff_t<<std::endl;
    }

    void testSingleReads(String length, int keyNum, int bucketNum) {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        ReadSet rs=_data.dataSet;

        timeval ts;
        gettimeofday(&ts,NULL);
        long int time1_s = ts.tv_sec;
        int time1_us=ts.tv_usec;

        String key;
        for (int i=0; i<bucketNum; i++){
            for(int j=0; j<keyNum; j++){
                key=_dataIndex[j]+"-"+length;
                _storage->read(_buckets[i], key,
                               std::tr1::bind(&StressTestBase::checkSuccess, this, true, ReadSet(), _1, _2)
                              );
                waitForTransaction();
            }
        }

        gettimeofday(&ts,NULL);
        long int time2_s = ts.tv_sec;
        int time2_us=ts.tv_usec;
        long int diff_s=time2_s - time1_s;
        int diff_us=time2_us - time1_us;
        double diff_t=diff_s*1000+diff_us/(double)1000;
        std::cout<<"Read time:  "<<diff_t<<std::endl;

    }
  
    void testSingleErases(String length, int keyNum, int bucketNum) {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        timeval ts;
        gettimeofday(&ts,NULL);
        long int time1_s = ts.tv_sec;
        int time1_us=ts.tv_usec;

        String key;
        for (int i=0; i<bucketNum; i++){
            for(int j=0; j<keyNum; j++){
                key=_dataIndex[j]+"-"+length;
                _storage->erase(_buckets[i], key,
                                std::tr1::bind(&StressTestBase::checkSuccess, this, true, ReadSet(), _1, _2)
                               );
                waitForTransaction();
            }
        }

        gettimeofday(&ts,NULL);
        long int time2_s = ts.tv_sec;
        int time2_us=ts.tv_usec;
        long int diff_s=time2_s - time1_s;
        int diff_us=time2_us - time1_us;
        double diff_t=diff_s*1000+diff_us/(double)1000;
        std::cout<<"Erase time: "<<diff_t<<std::endl;

        for (int i=0; i<bucketNum; i++){
            for(int j=0; j<keyNum; j++){
                key=_dataIndex[j]+"-"+length;
                _storage->read(_buckets[i], key,
                               std::tr1::bind(&StressTestBase::checkSuccess, this, false, ReadSet(), _1, _2)
                              );
                waitForTransaction();
            }
        }

    }

    void testBatchWrites(String length, int keyNum, int bucketNum) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        timeval ts;
        gettimeofday(&ts,NULL);
        long int time1_s = ts.tv_sec;
        int time1_us=ts.tv_usec;

        String key;
        for (int i=0; i<bucketNum; i++){
            _storage->beginTransaction(_buckets[i]);
            for(int j=0; j<keyNum; j++){
                key=_dataIndex[j]+"-"+length;
                _storage->write(_buckets[i], key, _data.dataSet[key]);
            }
            _storage->commitTransaction(_buckets[i],
                                        std::tr1::bind(&StressTestBase::checkSuccess, this, true, ReadSet(), _1, _2)
                                       );
            waitForTransaction();
        }

        gettimeofday(&ts,NULL);
        long int time2_s = ts.tv_sec;
        int time2_us=ts.tv_usec;
        long int diff_s=time2_s - time1_s;
        int diff_us=time2_us - time1_us;
        double diff_t=diff_s*1000+diff_us/(double)1000;
        std::cout<<"Write time: "<<diff_t<<std::endl;
    }

    void testBatchReads(String length, int keyNum, int bucketNum) {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        ReadSet rs=_data.dataSet;

        timeval ts;
        gettimeofday(&ts,NULL);
        long int time1_s = ts.tv_sec;
        int time1_us=ts.tv_usec;

        String key;
        for (int i=0; i<bucketNum; i++){
            _storage->beginTransaction(_buckets[i]);
            for(int j=0; j<keyNum; j++){
                key=_dataIndex[j]+"-"+length;
                _storage->read(_buckets[i], key);
            }
            _storage->commitTransaction(_buckets[i],
                                        std::tr1::bind(&StressTestBase::checkSuccess, this, true, ReadSet(), _1, _2)
                                       );
            waitForTransaction();
        }

        gettimeofday(&ts,NULL);
        long int time2_s = ts.tv_sec;
        int time2_us=ts.tv_usec;
        long int diff_s=time2_s - time1_s;
        int diff_us=time2_us - time1_us;
        double diff_t=diff_s*1000+diff_us/(double)1000;
        std::cout<<"Read time:  "<<diff_t<<std::endl;

    }

    void testBatchErases(String length, int keyNum, int bucketNum) {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        timeval ts;
        gettimeofday(&ts,NULL);
        long int time1_s = ts.tv_sec;
        int time1_us=ts.tv_usec;

        String key;
        for (int i=0; i<bucketNum; i++){
            _storage->beginTransaction(_buckets[i]);
            for(int j=0; j<keyNum; j++){
                key=_dataIndex[j]+"-"+length;
                _storage->erase(_buckets[i], key);
            }
            _storage->commitTransaction(_buckets[i],
                                        std::tr1::bind(&StressTestBase::checkSuccess, this, true, ReadSet(), _1, _2)
                                       );
            waitForTransaction();
        }

        gettimeofday(&ts,NULL);
        long int time2_s = ts.tv_sec;
        int time2_us=ts.tv_usec;
        long int diff_s=time2_s - time1_s;
        int diff_us=time2_us - time1_us;
        double diff_t=diff_s*1000+diff_us/(double)1000;
        std::cout<<"Erase time: "<<diff_t<<std::endl;

        for (int i=0; i<bucketNum; i++){
            for(int j=0; j<keyNum; j++){
                key=_dataIndex[j]+"-"+length;
                _storage->read(_buckets[i], key,
                               std::tr1::bind(&StressTestBase::checkSuccess, this, false, ReadSet(), _1, _2)
                              );
                waitForTransaction();
            }
        }
    }

  
  void testMultiRounds(String length, int keyNum, int bucketNum, int times){
    
    std::cout<<"dataLength: "<<length<<", keyNum: "<<keyNum<<", bucketNum: "<<bucketNum<<", rounds: "<<times<<"\n\n";
    for (int i=0; i<times; i++){
      std::cout<<"Round "<<i+1<<std::endl;
      std::cout<<"testSingleWrites -- ";
      testSingleWrites(length, keyNum, bucketNum);
      std::cout<<"testSingleReads  -- ";
      testSingleReads(length, keyNum, bucketNum);
      std::cout<<"testSingleErases -- ";
      testSingleErases(length, keyNum, bucketNum);
      std::cout<<"testBatchWrites  -- ";
      testBatchWrites(length, keyNum, bucketNum);
      std::cout<<"testBatchReads   -- ";
      testBatchReads(length, keyNum, bucketNum);
      std::cout<<"testBatchErases  -- ";
      testBatchErases(length, keyNum, bucketNum);
      std::cout<<std::endl;
    }
  }
  
};

const OH::Storage::Bucket StressTestBase::_buckets[100] = DataFiles::buckets;
const String StressTestBase::_dataIndex[20]={"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t"};

#endif //__SIRIKATA_STRESS_TEST_BASE_HPP__
