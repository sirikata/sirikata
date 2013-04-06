// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <cxxtest/TestSuite.h>
#include "MockSST.hpp"

#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>

using namespace Sirikata;

class SstTest : public CxxTest::TestSuite {
public:
    // Helpers for getting event loop setup/torn down
    Sirikata::Trace::Trace* _trace;
    Sirikata::Network::IOService* _ios;
    Sirikata::Network::IOStrand* _mainStrand;
    Sirikata::Network::IOWork* _work;
    Sirikata::Context* _ctx;

    Sirikata::Mock::Service* _mock_service;
    Sirikata::Mock::ConnectionManager* _conn_mgr;

    // Events are triggered from worker threads. We use simple strings (allowing
    // formatting, e.g. indicating which endpoint received data) and push them
    // through a queue, which the main test thread monitors to verify events
    // occur.
    typedef String Event;
    typedef std::vector<Event> EventList;
    typedef std::multiset<String> EventSet;
    ThreadSafeQueue<Event> _events;

    // List of endpoints for convenience, and we'll just always set
    // them all up for
    std::vector<Mock::ID> _endpoints;
    // Sample data
    String _small_payload;
    String _medium_payload;
    String _large_payload;

#define LOSSLESS 0.0
#define LOSSY 0.1

#define NODELAY_CHANNEL Duration::zero()
#define SLOW_CHANNEL Duration::milliseconds(50)

#define SHORT_TIMEOUT Duration::seconds(5)
#define LONG_TIMEOUT Duration::seconds(15)

    SstTest()
     : _trace(NULL),
       _ios(NULL),
       _mainStrand(NULL),
       _work(NULL),
       _ctx(NULL),
       _mock_service(NULL),
       _conn_mgr(NULL)
    {
        for(int i = 0; i < 10; i++)
            _endpoints.push_back(Mock::ID(String(1, 'a' + i)));

        _small_payload = "this is the payload";
#define MEDIUM_PAYLOAD_SIZE 1024*512
        _medium_payload.resize(MEDIUM_PAYLOAD_SIZE);
        for(int i = 0; i < MEDIUM_PAYLOAD_SIZE; i++)
            _medium_payload[i] = ('a' + (i % 26));
#define LARGE_PAYLOAD_SIZE 1024*1024*200
        // Reuse medium payload because rand() gets expensive with large
        // payloads.
        _large_payload.resize(LARGE_PAYLOAD_SIZE);
        for(int i = 0; i < LARGE_PAYLOAD_SIZE; i += MEDIUM_PAYLOAD_SIZE)
            memcpy((void*)(_large_payload.c_str() + i), _medium_payload.c_str(), MEDIUM_PAYLOAD_SIZE);
    }

    // Helpers for waiting for events to occur:
    // In specific order one at a time:
    bool waitForEvent(const Event& evt, Duration timeout=Duration::seconds(5)) {
        Event read_evt;
        bool got_event = _events.blockingPop(read_evt, timeout);
        TS_ASSERT(got_event);
        if (!got_event) return false;
        TS_ASSERT_EQUALS(evt, read_evt);
        if (evt != read_evt) return false;
        return true;
    }
    // In specific order many at a time:
    bool waitForEventList(const EventList& evts, Duration timeout=Duration::seconds(5)) {
        for(uint idx = 0; idx < evts.size(); idx++) {
            Event read_evt;
            bool got_event = _events.blockingPop(read_evt, timeout);
            TS_ASSERT(got_event);
            if (!got_event) return false;
            TS_ASSERT_EQUALS(evts[idx], read_evt);
            if (evts[idx] != read_evt) return false;
        }
        return true;
    }
    // In any order many at a time:
    bool waitForEventSet(const EventSet& evts, Duration timeout=Duration::seconds(5)) {
        EventSet read_evts;
        for(uint idx = 0; idx < evts.size(); idx++) {
            Event read_evt;
            bool got_event = _events.blockingPop(read_evt, timeout);
            TS_ASSERT(got_event);
            if (!got_event) return false;
            read_evts.insert(read_evt);
        }
        for(EventSet::iterator evt_it = evts.begin(), read_evt_it = read_evts.begin();
            evt_it != evts.end(), read_evt_it != read_evts.end();
            evt_it++, read_evt_it++)
        {
            TS_ASSERT_EQUALS(*evt_it, *read_evt_it);
            if (*evt_it != *read_evt_it) return false;
        }
        return true;
    }


    // Helpers that interact with SST and eventually forwra

    // Initial
    void createDatagramLayers() {
        for(uint32 i = 0; i < _endpoints.size(); i++) {
            _conn_mgr->createDatagramLayer(
                _endpoints[i], _ctx, _mock_service
            );
        }
    }
    // Handle connection by finishing with event
    void onConnectNotify(Mock::ID ep, int err, Mock::Stream::Ptr s) {
        if (err == SST_IMPL_SUCCESS)
            _events.push(ep.toString() + " connected");
        else
            _events.push(ep.toString() + " connection failed");
    }
    // Connection handler that listens for data
    void onConnectListenForData(Mock::ID ep, int err, Mock::Stream::Ptr s, String* expected) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        if (err != SST_IMPL_SUCCESS) return;

        // Need this to last across many calls to read
        uint32* read_so_far = new uint32(0);
        s->registerReadCallback(
            std::tr1::bind(&SstTest::onExpectedRead, this, ep, _1, _2, expected, read_so_far)
        );
        _events.push(ep.toString() + " listening for data");
    }
    // Connection handler that only sends some data
    void onConnectSendData(Mock::ID ep, int err, Mock::Stream::Ptr s, String* expected) {
        if (err != SST_IMPL_SUCCESS) return;
        writeExpectedData(ep, s, expected, 0);
    }

    // Handle a read operation, listening for expected data. Match the
    // data and, once we've received all of it, notify
    void onExpectedRead(Mock::ID ep, uint8* data, int size, String* expected, uint32* read_so_far) {
        TS_ASSERT(*read_so_far + size <= expected->size());
        if (*read_so_far + size > expected->size()) return;

        TS_ASSERT_EQUALS(memcmp(data, (expected->c_str() + *read_so_far), size), 0);
        *read_so_far += size;

        SILOG(test, detailed, "Read progress " << *read_so_far << " of " << expected->size() << " (" << (*read_so_far/(float32)expected->size())*100 << "%)");
        if (*read_so_far == expected->size()) {
            // We're done
            _events.push(ep.toString() + " received expected");
            delete read_so_far;
        }
    }
    // Write expected data. Because of buffers, we may need to do this
    // multiple times to get all the data sent
    void writeExpectedData(Mock::ID ep, Mock::Stream::Ptr s, String* expected, uint32 from) {
        int bytes_written = s->write((uint8*)(expected->c_str() + from), expected->size()-from);
        from += bytes_written;
        if (from == expected->size()) {
            _events.push(ep.toString() + " sent");
        }
        else {
            // Need to write more, but should delay since we couldn't
            // write it all in there immediately.
            _ctx->mainStrand->post(
                Duration::milliseconds(1),
                std::tr1::bind(&SstTest::writeExpectedData, this, ep, s, expected, from)
            );
        }
    }


    void setUp() {
        // Make sure there are not any leftover events from another test
        std::deque<Event> empty_evts;
        _events.swap(empty_evts);

        // Storage is tied to the main event loop, which requires quite a bit of setup
        _trace = new Sirikata::Trace::Trace("dummy.trace");
        _ios = new Sirikata::Network::IOService("SSTTest Service");
        _mainStrand = _ios->createStrand("SSTTest Main Strand");
        _work = new Sirikata::Network::IOWork(*_ios, "SSTTest IOWork");
        Sirikata::Time start_time = Sirikata::Timer::now();
        _ctx = new Sirikata::Context("sst test", _ios, _mainStrand, _trace, start_time);

        _mock_service = new Mock::Service(_ctx);
        _conn_mgr = new Mock::ConnectionManager();

        _ctx->add(_ctx);
        _ctx->add(_conn_mgr);

        createDatagramLayers();

        // Run the context, but we need to make sure it only lives in other
        // threads, otherwise we'd block up this one.  We include 4 threads to
        // exercise support for multiple threads.
        _ctx->run(4, Sirikata::Context::AllNew);
    }

    void tearDown() {
        delete _work;
        _work = NULL;

        _ctx->shutdown();

        _trace->prepareShutdown();

        delete _ctx;
        _ctx = NULL;

        _trace->shutdown();
        delete _trace;
        _trace = NULL;

        delete _mainStrand;
        _mainStrand = NULL;
        delete _ios;
        _ios = NULL;

        // Note: These two *must* currently come after the deletion of ios
        // because handlers still have shared_ptrs and are in the queue, causing
        // them to access some conn_mgr (and therefore mock_service) state when
        // the ios is being deleted. Proper cleanup of SST timers would fix this.
        delete _conn_mgr;
        _conn_mgr = NULL;

        delete _mock_service;
        _mock_service = NULL;
    }

    void testListenConnect() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        // Test that A listens, B tries to connect
        _conn_mgr->listen(
            std::tr1::bind(&SstTest::onConnectNotify, this, _endpoints[0], _1, _2),
            Mock::Endpoint(_endpoints[0], 1)
        );
        _conn_mgr->connectStream(
            Mock::Endpoint(_endpoints[1], 1),
            Mock::Endpoint(_endpoints[0], 1),
            std::tr1::bind(&SstTest::onConnectNotify, this, _endpoints[1], _1, _2)
        );

        EventSet conn_evts;
        conn_evts.insert("a connected");
        conn_evts.insert("b connected");
        waitForEventSet(conn_evts);
    }

    typedef Sirikata::SST::ReceivedSegmentList ReceivedSegmentList;
    typedef ReceivedSegmentList::SegmentRange SegmentRange;
    // One insert + readyRange
    void testReceivedSegmentListOne() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 200);
    }

    // Ordered insert + one readyRange extraction
    void testReceivedSegmentListOrderedInsert() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        rsl.insert(200, 100);
        rsl.insert(300, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 400);
    }

    // Ordered insert + one readyRange extraction
    void testReceivedSegmentListOrderedInsertMultiple() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        rsl.insert(300, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 200);
        r = rsl.readyRange(200, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 200);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 400);
    }


    // Insert before first range in list and make sure we get valid result
    void testReceivedSegmentListInsertBeforeFirst() {
        ReceivedSegmentList rsl;
        rsl.insert(200, 100);
        rsl.insert(100, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 300);
    }

    // Insert before second (not first) range in list and make sure we get valid result
    void testReceivedSegmentListInsertBeforeMiddle() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        rsl.insert(400, 100);
        rsl.insert(300, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 200);
        r = rsl.readyRange(200, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 200);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 500);
    }


    // Insert after first range in list and make sure we get valid result
    void testReceivedSegmentListInsertAfterFirst() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        rsl.insert(400, 100);
        rsl.insert(200, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 300);
        r = rsl.readyRange(300, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 300);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 500);
    }

    // Insert after second (not first) range in list and make sure we get valid result
    void testReceivedSegmentListInsertAfterMiddle() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        rsl.insert(300, 100);
        rsl.insert(600, 100);
        rsl.insert(400, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 200);
        r = rsl.readyRange(200, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 200);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 500);
        r = rsl.readyRange(500, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 500);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 700);
    }


    // Insert simple range multiple times to ensure it still just has one proper entry
    void testReceivedSegmentListInsertRepeatedFirst() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        rsl.insert(100, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 200);
        TS_ASSERT(rsl.empty());
    }

    // Insert simple range multiple times to ensure it still just has one proper entry
    void testReceivedSegmentListInsertRepeatedFirstPartial() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 200);
        rsl.insert(100, 100);
        rsl.insert(200, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 300);
        TS_ASSERT(rsl.empty());
    }


    // Insert simple range multiple times to ensure it still just has one proper entry
    void testReceivedSegmentListInsertRepeatedMiddle() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        rsl.insert(300, 100);
        rsl.insert(500, 100);
        rsl.insert(300, 100);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 200);
        r = rsl.readyRange(200, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 200);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 400);
        r = rsl.readyRange(400, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 400);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 600);
        TS_ASSERT(rsl.empty());
    }

    // Insert simple range multiple times to ensure it still just has one proper entry
    void testReceivedSegmentListInsertRepeatedMiddlePartial() {
        ReceivedSegmentList rsl;
        rsl.insert(100, 100);
        rsl.insert(300, 100);
        rsl.insert(500, 100);
        rsl.insert(300, 50);
        rsl.insert(350, 50);
        SegmentRange r = rsl.readyRange(0, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 0);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 200);
        r = rsl.readyRange(200, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 200);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 400);
        r = rsl.readyRange(400, 100);
        TS_ASSERT_EQUALS(ReceivedSegmentList::StartByte(r), 400);
        TS_ASSERT_EQUALS(ReceivedSegmentList::EndByte(r), 600);
        TS_ASSERT(rsl.empty());
    }







    // Allows testing different lengths
    void impl_testSendReceiveOneDirection(String* payload, Duration channel_delay, float32 channel_drop_rate, Duration timeout) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        Time t_start = Timer::now();

        _mock_service->setDelay(channel_delay);
        _mock_service->setDropRate(channel_drop_rate);

        // Test that send/receive works when only sending in one
        // direction
        _conn_mgr->listen(
            std::tr1::bind(&SstTest::onConnectListenForData, this, _endpoints[0], _1, _2, payload),
            Mock::Endpoint(_endpoints[0], 1)
        );
        _conn_mgr->connectStream(
            Mock::Endpoint(_endpoints[1], 1),
            Mock::Endpoint(_endpoints[0], 1),
            std::tr1::bind(&SstTest::onConnectSendData, this, _endpoints[1], _1, _2, payload)
        );

        // Both start up and listen/send
        EventSet conn_evts;
        conn_evts.insert("a listening for data");
        conn_evts.insert("b sent");
        bool success = waitForEventSet(conn_evts, timeout);
        // Then completion when a receives all data
        if (success) success = waitForEvent("a received expected", timeout);

        if (success) {
            Time t_end = Timer::now();
            SILOG(test, info, "Finished in " << (t_end-t_start));
        }
    }

    void testSendReceiveOneDirection() {
        impl_testSendReceiveOneDirection(&_small_payload, NODELAY_CHANNEL, LOSSLESS, SHORT_TIMEOUT);
    }
    void testSendReceiveOneDirectionLarge() {
        String payload = "";
        impl_testSendReceiveOneDirection(&_large_payload, NODELAY_CHANNEL, LOSSLESS, LONG_TIMEOUT);
    }


    // Test over 50% lossy channel
    void testSendReceiveOneDirectionLossy() {
        impl_testSendReceiveOneDirection(&_small_payload, NODELAY_CHANNEL, LOSSY, LONG_TIMEOUT);
    }
    // Note medium payload. Large payload with lossy or slow
    // connections takes too long
    void testSendReceiveOneDirectionLargeLossy() {
        String payload = "";
        impl_testSendReceiveOneDirection(&_medium_payload, NODELAY_CHANNEL, LOSSY, LONG_TIMEOUT);
    }

    // Test over slow channel
    void testSendReceiveOneDirectionSlow() {
        impl_testSendReceiveOneDirection(&_small_payload, SLOW_CHANNEL, LOSSLESS, SHORT_TIMEOUT);
    }
    void testSendReceiveOneDirectionLargeSlow() {
        String payload = "";
        impl_testSendReceiveOneDirection(&_large_payload, SLOW_CHANNEL, LOSSLESS, LONG_TIMEOUT);
    }

    // Test over slow, lossy channel
    void testSendReceiveOneDirectionSlowLossy() {
        impl_testSendReceiveOneDirection(&_small_payload, SLOW_CHANNEL, LOSSY, LONG_TIMEOUT);
    }
    void testSendReceiveOneDirectionLargeSlowLossy() {
        String payload = "";
        impl_testSendReceiveOneDirection(&_medium_payload, SLOW_CHANNEL, LOSSY, LONG_TIMEOUT);
    }
};
