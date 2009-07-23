/*  Sirikata -- Persistence Services
 *  ReadWriteHandlerTest.cpp
 *
 *  Copyright (c) 2008, Stanford University
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


// These methods are just helpers and generic tests for ReadWriteHandlers.  No tests are
// actually setup in this file.
#include <util/Platform.hpp>
#include <cxxtest/TestSuite.h>
#include "ReadWriteHandlerTest.hpp"

#include "util/AtomicTypes.hpp"
using namespace Sirikata;
using namespace Sirikata::Persistence;


namespace {
void pollMinitransaction(){}

}


class ReadWriteHandlerTestFixture {
public:
    ReadWriteHandlerTestFixture(SetupReadWriteHandlerFunction _setup,
                                CreateReadWriteHandlerFunction create_handler,
                                String pl,
                                TeardownReadWriteHandlerFunction _teardown)
     : teardown(_teardown)
    {
        _setup();

        handler = create_handler(pl);
    }
    ~ReadWriteHandlerTestFixture() {
        delete handler;

        teardown();
    }

    ReadWriteHandler* handler;
    TeardownReadWriteHandlerFunction teardown;
};

static void check_fill_read_write_handler_result(ObjectStorageError& error, bool* done) {
    TS_ASSERT_EQUALS( error.type(), ObjectStorageErrorType_None );
    *done = true;
}

/** Fill a ReadWriteHandler with all the available generated <key,value>
 *  pairs.
 *  \param rwh the ReadWriteHandler to fill with <key,value> pairs
 */
static void fill_read_write_handler(ReadWriteHandler* rwh) {
    ReadWriteSet* rws = new ReadWriteSet();
    for(int i = 0; i < OBJECT_STORAGE_GENERATED_PAIRS; i++)
        rws->writes().addPair( keys()[i], values()[i] );

    bool done = false;

    rwh->apply(rws, std::tr1::bind(check_fill_read_write_handler_result, _1, &done));

    while(!done)
        pollMinitransaction();

    delete rws;
}

// note: we use a StorageSet for expected instead of a ReadSet so we can add pairs to it
static void check_read_write_results(ObjectStorageError& error, ReadWriteSet* rws, StorageSet expected, bool* done) {
    TS_ASSERT_EQUALS( error.type(), ObjectStorageErrorType_None );

    TS_ASSERT_EQUALS( rws->reads().size(), expected.size() );

    for(ReadSet::iterator read_it = rws->reads().begin(), expected_it = expected.begin(); read_it != rws->reads().end(); read_it++, expected_it++) {
        TS_ASSERT_EQUALS( read_it->second->size(), expected_it->second->size() );
        TS_ASSERT_EQUALS(
            *read_it->second,
            *expected_it->second
        );
    }

    *done = true;
}


/** Tests a ReadWriteSet's application.  Attempts to apply the given
 *  ReadWriteSet and checks the values read against an expected set.
 *  \param rwh the ReadWriteHandler to use
 *  \param rws the ReadWriteSet to be applied. Ownership is transferred, will be
 *             cleaned up automatically
 *  \param expected a StorageSet with the values that should be returned in the ReadSet
 */
static void test_read_write_set(ReadWriteHandler* rwh, ReadWriteSet* rws, StorageSet expected) {
    bool done = false;

    rwh->apply(rws, std::tr1::bind(check_read_write_results, _1, rws, expected, &done));

    while( !done )
                pollMinitransaction();

    delete rws;
}

static void check_stress_test_result(ObjectStorageError& error, AtomicValue<Sirikata::uint32>* done) {
    TS_ASSERT_EQUALS( error.type(), ObjectStorageErrorType_None );

    (*done)++;
}


void stress_test_read_write_handler(SetupReadWriteHandlerFunction _setup, CreateReadWriteHandlerFunction create_handler,
                                    String pl, TeardownReadWriteHandlerFunction _teardown,
                                    uint32 num_sets, uint32 num_readswrites)
{
    assert(num_readswrites <= OBJECT_STORAGE_GENERATED_PAIRS);

    ReadWriteHandlerTestFixture fixture(_setup, create_handler, pl, _teardown);

    // fill up the storage to so the stress tests should not get any failures
    fill_read_write_handler(fixture.handler);

    std::vector<ReadWriteSet*> sets;

    // generate a bunch of ReadWriteSets with a lot of items in them
    for(uint32 i = 0; i < num_sets; i++) {
        ReadWriteSet* rws = new ReadWriteSet();
        for(uint32 j = 0; j < num_readswrites; j++) {
            rws->reads().addKey( keys()[j] );
            rws->writes().addPair( keys()[j], values()[j] );
        }
        sets.push_back(rws);
    }

    AtomicValue<Sirikata::uint32> done(0);

    // now quickly submit all of them
    for(uint32 i = 0; i < num_sets; i++)
        fixture.handler->apply(sets[i], std::tr1::bind(check_stress_test_result, _1, &done));

    while( done.read() < num_sets )
        pollMinitransaction();

    for(uint32 i = 0; i < num_sets; i++)
        delete sets[i];
    sets.clear();
}


void test_read_write_handler_order(SetupReadWriteHandlerFunction _setup, CreateReadWriteHandlerFunction create_handler,
                                   String pl, TeardownReadWriteHandlerFunction _teardown) {
    ReadWriteHandlerTestFixture fixture(_setup, create_handler, pl, _teardown);

    // 0 reads, 0 writes
    ReadWriteSet* rws_1 = new ReadWriteSet();
    StorageSet expected_1;
    test_read_write_set(fixture.handler, rws_1, expected_1);

    // 0 reads, 1 write
    ReadWriteSet* rws_2 = new ReadWriteSet();
    rws_2->writes().addPair( keys()[0], values()[0] );
    StorageSet expected_2;
    test_read_write_set(fixture.handler, rws_2, expected_2);

    // 1 read from last set's write, 0 writes
    ReadWriteSet* rws_3 = new ReadWriteSet();
    rws_3->reads().addKey( keys()[0] );
    StorageSet expected_3;
    expected_3.addPair( keys()[0], values()[0] );
    test_read_write_set(fixture.handler, rws_3, expected_3);

    // 1 read from previous set, 1 write
    ReadWriteSet* rws_4 = new ReadWriteSet();
    rws_4->reads().addKey( keys()[0] );
    rws_4->writes().addPair( keys()[1], values()[1] );
    StorageSet expected_4;
    expected_4.addPair( keys()[0], values()[0] );
    test_read_write_set(fixture.handler, rws_4, expected_4);
}
