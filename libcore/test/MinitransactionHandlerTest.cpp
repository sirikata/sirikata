/*  Sirikata -- Persistence Services
 *  MinitransactionHandlerTest.cpp
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

// These methods are just helpers and generic tests for MinitransactionHandlers.  No tests are
// actually setup in this file.
#include <sirikata/core/util/Standard.hh>
#include <cxxtest/TestSuite.h>
#include "MinitransactionHandlerTest.hpp"

#include <sirikata/core/util/AtomicTypes.hpp>
#include "Protocol_Persistence.pbj.hpp"
using namespace Sirikata;
using namespace Sirikata::Persistence;

namespace {
void pollMinitransaction(){}
}
class MinitransactionHandlerTestFixture {
public:
    MinitransactionHandlerTestFixture(SetupMinitransactionHandlerFunction _setup,
                                      CreateMinitransactionHandlerFunction create_handler,
                                      String pl,
                                      TeardownMinitransactionHandlerFunction _teardown)
     : teardown(_teardown)
    {
        _setup();

        handler = create_handler(pl);
    }
    ~MinitransactionHandlerTestFixture() {
        delete handler;

        teardown();
    }

    MinitransactionHandler* handler;
    TeardownMinitransactionHandlerFunction teardown;
};

static void check_fill_minitransaction_handler_result(Protocol::Response *response, volatile bool* done, Protocol::Response**returned_response) {
    if (response->has_return_status())
        TS_ASSERT_EQUALS(response->return_status(), Protocol::Response::SUCCESS );
    *returned_response=response;
    *done = true;
}

/** Fill a MinitransactionHandler with all the available generated <key,value>
 *  pairs.
 *  \param mth the MinitransactionHandler to fill with <key,value> pairs
 */
static void fill_minitransaction_handler(MinitransactionHandler* mth) {
    using namespace Sirikata::Persistence::Protocol;
    Minitransaction* trans = mth->createMinitransaction((Minitransaction*)NULL,0,OBJECT_STORAGE_GENERATED_PAIRS,0);
    for(int i = 0; i < OBJECT_STORAGE_GENERATED_PAIRS; i++) {
        copyStorageElement(trans->mutable_writes(i),keyvalues()[i]);
    }
    volatile bool done = false;
    Response*final_reads=NULL;
    mth->transact(trans, std::tr1::bind(check_fill_minitransaction_handler_result, _1, &done,&final_reads));
    while(!done)
        pollMinitransaction();
    TS_ASSERT(final_reads!=NULL);
    mth->destroyResponse(final_reads);
}

// note: we use a StorageSet for expected instead of a ReadSet so we can add pairs to it
static void check_minitransaction_results(MinitransactionHandler* mth, Protocol::Response *response, volatile bool* done, Protocol::Response::ReturnStatus expected_error, Protocol::StorageSet expected, int testnum) {

    if (expected_error != Protocol::Response::SUCCESS) {
        TS_ASSERT(response->has_return_status());
        TS_ASSERT_EQUALS( response->return_status(), expected_error );
    }else if (response->has_return_status()) {
        TS_ASSERT_EQUALS(response->return_status(),Protocol::Response::SUCCESS);
    }

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
    mth->destroyResponse(response);
    *done = true;
}


/** Tests a MinitransactionSet's application.  Attempts to apply the given
 *  Minitransaction and checks the values read against an expected set.
 *  \param mth the MinitransactionHandler to use
 *  \param trans the Minitransaction to be applied. Ownership is transferred, will be
 *               cleaned up automatically
 *  \param expected_error an ObjectStorageErrorType indicating the expected outcome of this
 *                        Minitransaction
 *  \param expected a StorageSet with the values that should be returned in the ReadSet,
 *                  only used if expected_error is None
 */
static void test_minitransaction(MinitransactionHandler* mth, Protocol::Minitransaction* trans, Protocol::Response::ReturnStatus expected_error, const Protocol::StorageSet &expected, int testnum) {
    volatile bool done = false;

    mth->transact(trans, std::tr1::bind(check_minitransaction_results, mth, _1, &done, expected_error, expected, testnum));

    while( !done )
        pollMinitransaction();

}

static void check_stress_test_result(MinitransactionHandler*mth, Protocol::Response* result, AtomicValue<Sirikata::uint32>* done) {
    if (result->has_return_status())
        TS_ASSERT_EQUALS( result->return_status(), Protocol::Response::SUCCESS );

    mth->destroyResponse(result);
    (*done)++;
}


void stress_test_minitransaction_handler(SetupMinitransactionHandlerFunction _setup, CreateMinitransactionHandlerFunction create_handler,
                                         String pl, TeardownMinitransactionHandlerFunction _teardown,
                                         uint32 num_trans, uint32 num_ops)
{
    using namespace Sirikata::Persistence::Protocol;
    TS_ASSERT(num_ops <= OBJECT_STORAGE_GENERATED_PAIRS);

    MinitransactionHandlerTestFixture fixture(_setup, create_handler, pl, _teardown);

    // fill up the storage to so the stress tests should not get any failures
    fill_minitransaction_handler(fixture.handler);

    std::vector<Minitransaction*> transactions;
    int counter=0;
    // generate a bunch of MinitransactionSets with a lot of items in them
    for(uint32 i = 0; i < num_trans; i++) {
        Minitransaction* trans = fixture.handler->createMinitransaction((Minitransaction*)NULL,num_ops,num_ops,0);
        for(uint32 j = 0; j < num_ops; j++) {
            copyStorageKey(trans->mutable_reads(j),keyvalues()[j]);

            copyStorageKey(trans->mutable_writes(j),keyvalues()[j]);
            copyStorageValue(trans->mutable_writes(j),keyvalues()[(counter+=5)%keyvalues().size()]);
        }
        transactions.push_back(trans);
    }

    AtomicValue<Sirikata::uint32> done(0);

    // now quickly submit all of them
    for(uint32 i = 0; i < num_trans; i++)
        fixture.handler->transact(transactions[i], std::tr1::bind(check_stress_test_result, fixture.handler,_1, &done));

    while( done.read() < num_trans )
        pollMinitransaction();

    transactions.clear();
}


void test_minitransaction_handler_order(SetupMinitransactionHandlerFunction _setup, CreateMinitransactionHandlerFunction create_handler,
                                        Sirikata::String pl, TeardownMinitransactionHandlerFunction _teardown) {
    MinitransactionHandlerTestFixture fixture(_setup, create_handler, pl, _teardown);
    using namespace Sirikata::Persistence;
    using namespace Sirikata::Persistence::Protocol;
    // 0 reads, 0 writes
    Minitransaction* trans_1 = fixture.handler->createMinitransaction((Minitransaction*)NULL,0,0,0);
    StorageSet expected_1;
    test_minitransaction(fixture.handler, trans_1, Response::SUCCESS, expected_1,1);

    // 0 reads, 1 write
    Minitransaction* trans_2 = fixture.handler->createMinitransaction((Minitransaction*)NULL,0,1,0);
    copyStorageElement(trans_2->mutable_writes(0),keyvalues()[0]);
    StorageSet expected_2;
    test_minitransaction(fixture.handler, trans_2, Response::SUCCESS, expected_2,2);

    // 1 read from last set's write, 0 writes
    Minitransaction* trans_3 = fixture.handler->createMinitransaction((Minitransaction*)NULL,1,0,0);
    copyStorageKey(trans_3->mutable_reads(0), keyvalues()[0] );
    StorageSet expected_3;
    expected_3.add_reads();
    copyStorageElement(expected_3.mutable_reads(0),keyvalues()[0] );

    test_minitransaction(fixture.handler, trans_3, Response::SUCCESS, expected_3,3);

    // 1 read from previous set, 1 write
    Minitransaction* trans_4 = fixture.handler->createMinitransaction((Minitransaction*)NULL,1,1,0);
    copyStorageKey(trans_4->mutable_reads(0), keyvalues()[0] );
    copyStorageElement(trans_4->mutable_writes(0), keyvalues()[1] );
    StorageSet expected_4;
    expected_4.add_reads();
    copyStorageElement(expected_4.mutable_reads(0),keyvalues()[0]);
    test_minitransaction(fixture.handler, trans_4, Response::SUCCESS, expected_4,4);

    // 1 successful compare
    Minitransaction* trans_5 = fixture.handler->createMinitransaction((Minitransaction*)NULL,0,0,1);
    copyStorageElement(trans_5->mutable_compares(0), keyvalues()[0] );
    //trans_5->mutable_compares(0).set_comparator(CompareElement::EQUAL);//implicit
    StorageSet expected_5;
    test_minitransaction(fixture.handler, trans_5, Response::SUCCESS, expected_5,5);

    // 1 failed compare
    Minitransaction* trans_6 = fixture.handler->createMinitransaction((Minitransaction*)NULL,0,0,1);
    copyStorageKey(trans_6->mutable_compares(0),keyvalues()[0]);
    copyStorageValue(trans_6->mutable_compares(0),keyvalues()[1]);
    StorageSet expected_6;
    test_minitransaction(fixture.handler, trans_6, Response::COMPARISON_FAILED, expected_6,6);

    // 1 successful compare, 1 read
    Minitransaction* trans_7 = fixture.handler->createMinitransaction((Minitransaction*)NULL,1,0,1);
    copyStorageElement(trans_7->mutable_compares(0),keyvalues()[0]);
    copyStorageKey(trans_7->mutable_reads(0),keyvalues()[1]);
    StorageSet expected_7;
    expected_7.add_reads();
    copyStorageElement(expected_7.mutable_reads(0),keyvalues()[1]);
    test_minitransaction(fixture.handler, trans_7, Response::SUCCESS, expected_7,7);

    // 1 failed compare, 1 read
    Minitransaction* trans_8 = fixture.handler->createMinitransaction((Minitransaction*)NULL,1,0,1);
    copyStorageKey(trans_8->mutable_compares(0),keyvalues()[0]);
    copyStorageValue(trans_8->mutable_compares(0),keyvalues()[1]);
    copyStorageKey(trans_8->mutable_reads(0),keyvalues()[1]);
    StorageSet expected_8;
    expected_8.add_reads();
    copyStorageValue(expected_8.mutable_reads(0),keyvalues()[1]);
    test_minitransaction(fixture.handler, trans_8, Response::COMPARISON_FAILED, expected_8,8);

    // 1 successful compare, 1 read, 1 write
    Minitransaction* trans_9 = fixture.handler->createMinitransaction((Minitransaction*)NULL,1,1,1);
    copyStorageElement(trans_9->mutable_compares(0),keyvalues()[0]);
    copyStorageKey(trans_9->mutable_reads(0),keyvalues()[1]);
    copyStorageElement(trans_9->mutable_writes(0),keyvalues()[2]);

    StorageSet expected_9;
    expected_9.add_reads();
    copyStorageElement(expected_9.mutable_reads(0),keyvalues()[1]);

    test_minitransaction(fixture.handler, trans_9, Response::SUCCESS, expected_9,9);

    // 1 failed compare, 1 read, 1 write (which will not be performed)
    Minitransaction* trans_10 = fixture.handler->createMinitransaction((Minitransaction*)NULL,1,1,1);
    copyStorageKey(trans_10->mutable_compares(0),keyvalues()[0]);
    copyStorageValue(trans_10->mutable_compares(0),keyvalues()[1]);
    copyStorageKey(trans_10->mutable_reads(0),keyvalues()[1]);
    copyStorageKey(trans_10->mutable_writes(0),keyvalues()[2]);
    copyStorageValue(trans_10->mutable_writes(0),keyvalues()[3]);

    StorageSet expected_10;
    expected_10.add_reads();
    copyStorageValue(expected_10.mutable_reads(0),keyvalues()[1]);
    test_minitransaction(fixture.handler, trans_10, Response::COMPARISON_FAILED, expected_10,10);


    // finally just verify that the previous write did not happen
    Minitransaction* trans_11 = fixture.handler->createMinitransaction((Minitransaction*)NULL,1,0,0);

    copyStorageKey(trans_11->mutable_reads(0),keyvalues()[2]);
    StorageSet expected_11;
    expected_11.add_reads();
    copyStorageElement(expected_11.mutable_reads(0),keyvalues()[2]);
    test_minitransaction(fixture.handler, trans_11, Response::SUCCESS, expected_11,11);
}
