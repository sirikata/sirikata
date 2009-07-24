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
#include "Test_Persistence.pbj.hpp"
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

static void check_fill_read_write_handler_result(Protocol::Response *response, bool* done, Protocol::Response**returned_response) {
    if (response->has_return_status())
        TS_ASSERT_EQUALS(response->return_status(), Protocol::Response::SUCCESS );
    *returned_response=response;
    *done = true;
}

/** Fill a ReadWriteHandler with all the available generated <key,value>
 *  pairs.
 *  \param rwh the ReadWriteHandler to fill with <key,value> pairs
 */
static void fill_read_write_handler(ReadWriteHandler* rwh) {
    using namespace Sirikata::Persistence::Protocol;
    ReadWriteSet* trans = rwh->createReadWriteSet(0,OBJECT_STORAGE_GENERATED_PAIRS);
    for(int i = 0; i < OBJECT_STORAGE_GENERATED_PAIRS; i++) {
        copyStorageElement(trans->mutable_writes(i),keyvalues()[i]);
    }
    bool done = false;
    Response*final_reads=NULL;
    rwh->apply(trans, std::tr1::bind(check_fill_read_write_handler_result, _1, &done,&final_reads));
    while(!done)
        pollMinitransaction();
    TS_ASSERT(final_reads!=NULL);
    rwh->destroyResponse(final_reads);
}

// note: we use a StorageSet for expected instead of a ReadSet so we can add pairs to it
static void check_read_write_results(ReadWriteHandler* rwh, Protocol::Response *response, bool* done, Protocol::Response::ReturnStatus expected_error, Protocol::StorageSet expected, int testnum) {
    using namespace Sirikata::Persistence::Protocol;
    if (response->has_return_status())
        TS_ASSERT_EQUALS( response->return_status(), Response::SUCCESS );
    TS_ASSERT_EQUALS( response->reads_size(), expected.reads_size() );
    int len=response->reads_size();
    if (len==expected.reads_size()) {
        for (int i=0;i<len;++i) {
            TS_ASSERT_EQUALS(response->reads(i).data(),expected.reads(i).data());
            if (response->reads(i).data()!=expected.reads(i).data()) {
                int THIS_TEST_FAILED=-1;
                TS_ASSERT_EQUALS(testnum,THIS_TEST_FAILED);
            }
        }
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
static void test_read_write(ReadWriteHandler* rwh, Protocol::ReadWriteSet* trans, Protocol::Response::ReturnStatus expected_error, const Protocol::StorageSet &expected, int testnum) {
    bool done = false;

    rwh->apply(trans, std::tr1::bind(check_read_write_results, rwh, _1, &done, expected_error, expected, testnum));

    while( !done )
        pollMinitransaction();

}

static void check_stress_test_result(ReadWriteHandler*rwh, Protocol::Response* result, AtomicValue<Sirikata::uint32>* done) {
    if (result->has_return_status()) 
        TS_ASSERT_EQUALS( result->return_status(), Protocol::Response::SUCCESS );
    
    rwh->destroyResponse(result);
    (*done)++;
}


void stress_test_read_write_handler(SetupReadWriteHandlerFunction _setup, CreateReadWriteHandlerFunction create_handler,
                                    String pl, TeardownReadWriteHandlerFunction _teardown,
                                    uint32 num_sets, uint32 num_readswrites)
{
    using namespace Sirikata::Persistence::Protocol;
    assert(num_readswrites <= OBJECT_STORAGE_GENERATED_PAIRS);

    ReadWriteHandlerTestFixture fixture(_setup, create_handler, pl, _teardown);

    // fill up the storage to so the stress tests should not get any failures
    fill_read_write_handler(fixture.handler);

    std::vector<ReadWriteSet*> transactions;
    int counter=0;
    // generate a bunch of MinitransactionSets with a lot of items in them
    for(uint32 i = 0; i < num_sets; i++) {
        ReadWriteSet* trans = fixture.handler->createReadWriteSet(num_readswrites,num_readswrites);
        for(uint32 j = 0; j < num_readswrites; j++) {
            copyStorageKey(trans->mutable_reads(i),keyvalues()[j]);

            copyStorageKey(trans->mutable_writes(i),keyvalues()[j]);
            copyStorageValue(trans->mutable_writes(i),keyvalues()[(counter+=5)%keyvalues().size()]);
        }
        transactions.push_back(trans);
    }

    AtomicValue<Sirikata::uint32> done(0);

    // now quickly submit all of them
    for(uint32 i = 0; i < num_sets; i++)
        fixture.handler->apply(transactions[i], std::tr1::bind(check_stress_test_result, fixture.handler, _1, &done));

    while( done.read() < num_sets )
        pollMinitransaction();

   transactions.clear();
}


void test_read_write_handler_order(SetupReadWriteHandlerFunction _setup, CreateReadWriteHandlerFunction create_handler,
                                   String pl, TeardownReadWriteHandlerFunction _teardown) {
    ReadWriteHandlerTestFixture fixture(_setup, create_handler, pl, _teardown);
    using namespace Sirikata::Persistence::Protocol;
    // 0 reads, 0 writes
    ReadWriteSet* trans_1 = fixture.handler->createReadWriteSet(0,0);
    StorageSet expected_1;
    test_read_write(fixture.handler, trans_1, Response::SUCCESS, expected_1,1);

    // 0 reads, 1 write
    ReadWriteSet* trans_2 = fixture.handler->createReadWriteSet(0,1);
    copyStorageElement(trans_2->mutable_writes(0),keyvalues()[0]);
    StorageSet expected_2;
    test_read_write(fixture.handler, trans_2, Response::SUCCESS, expected_2,2);

    // 1 read from last set's write, 0 writes
    ReadWriteSet* trans_3 = fixture.handler->createReadWriteSet(1,0);
    copyStorageKey(trans_3->mutable_reads(0), keyvalues()[0] );
    StorageSet expected_3;
    expected_3.add_reads();
    copyStorageElement(expected_3.mutable_reads(0),keyvalues()[0] );

    test_read_write(fixture.handler, trans_3, Response::SUCCESS, expected_3,3);

    // 1 read from previous set, 1 write
    ReadWriteSet* trans_4 = fixture.handler->createReadWriteSet(1,1);
    copyStorageKey(trans_4->mutable_reads(0), keyvalues()[0] );
    copyStorageElement(trans_4->mutable_writes(0), keyvalues()[1] );
    StorageSet expected_4;
    expected_4.add_reads();
    copyStorageElement(expected_4.mutable_reads(0),keyvalues()[0]);
    test_read_write(fixture.handler, trans_4, Response::SUCCESS, expected_4,4);
}
