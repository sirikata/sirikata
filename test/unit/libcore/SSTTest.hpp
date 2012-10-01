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
    String _large_payload;

#define LOSSLESS 0.0
#define LOSSY 0.25

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
        // FIXME I'd expect large to be even larger, but currently things run
        // too slow to make it usable with anything larger than this.
        for(int i = 0; i < 1024*1024*2; i++)
            _large_payload += ('a' + (rand() % 26));
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
    void onConnectListenForData(Mock::ID ep, int err, Mock::Stream::Ptr s, String expected) {
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
    void onConnectSendData(Mock::ID ep, int err, Mock::Stream::Ptr s, String expected) {
        if (err != SST_IMPL_SUCCESS) return;
        writeExpectedData(ep, s, expected, 0);
    }

    // Handle a read operation, listening for expected data. Match the
    // data and, once we've received all of it, notify
    void onExpectedRead(Mock::ID ep, uint8* data, int size, String expected, uint32* read_so_far) {
        TS_ASSERT(*read_so_far + size <= expected.size());
        if (*read_so_far + size > expected.size()) return;

        for(uint32 i = 0; i < (uint32)size; i++)
            TS_ASSERT(data[i] == expected[*read_so_far + i]);
        *read_so_far += size;

        if (*read_so_far == expected.size()) {
            // We're done
            _events.push(ep.toString() + " received expected");
            delete read_so_far;
        }
    }
    // Write expected data. Because of buffers, we may need to do this
    // multiple times to get all the data sent
    void writeExpectedData(Mock::ID ep, Mock::Stream::Ptr s, String expected, uint32 from) {
        int bytes_written = s->write((uint8*)(expected.c_str() + from), expected.size()-from);
        from += bytes_written;
        if (from == expected.size()) {
            _events.push(ep.toString() + " sent");
        }
        else {
            // Need to write more, but should delay since we couldn't
            // write it all in there immediately.
            _ctx->mainStrand->post(
                Duration::milliseconds(10),
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

    // Allows testing different lengths
    void impl_testSendReceiveOneDirection(String payload, Duration channel_delay, float32 channel_drop_rate, Duration timeout) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

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
        if (success) waitForEvent("a received expected", timeout);
    }

    void testSendReceiveOneDirection() {
        impl_testSendReceiveOneDirection(_small_payload, NODELAY_CHANNEL, LOSSLESS, SHORT_TIMEOUT);
    }
    void testSendReceiveOneDirectionLarge() {
        String payload = "";
        impl_testSendReceiveOneDirection(_large_payload, NODELAY_CHANNEL, LOSSLESS, SHORT_TIMEOUT);
    }


    // Test over 50% lossy channel
    void testSendReceiveOneDirectionLossy() {
        impl_testSendReceiveOneDirection(_small_payload, NODELAY_CHANNEL, LOSSY, LONG_TIMEOUT);
    }
    void testSendReceiveOneDirectionLargeLossy() {
        String payload = "";
        impl_testSendReceiveOneDirection(_large_payload, NODELAY_CHANNEL, LOSSY, LONG_TIMEOUT);
    }

    // Test over slow channel
    void testSendReceiveOneDirectionSlow() {
        impl_testSendReceiveOneDirection(_small_payload, SLOW_CHANNEL, LOSSLESS, SHORT_TIMEOUT);
    }
    void testSendReceiveOneDirectionLargeSlow() {
        String payload = "";
        impl_testSendReceiveOneDirection(_large_payload, SLOW_CHANNEL, LOSSLESS, SHORT_TIMEOUT);
    }

    // Test over slow, lossy channel
    void testSendReceiveOneDirectionSlowLossy() {
        impl_testSendReceiveOneDirection(_small_payload, SLOW_CHANNEL, LOSSY, LONG_TIMEOUT);
    }
    void testSendReceiveOneDirectionLargeSlowLossy() {
        String payload = "";
        impl_testSendReceiveOneDirection(_large_payload, SLOW_CHANNEL, LOSSY, LONG_TIMEOUT);
    }
};
