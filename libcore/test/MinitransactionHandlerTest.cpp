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
#include <util/Platform.hpp>
#include <cxxtest/TestSuite.h>
#include "MinitransactionHandlerTest.hpp"

#include "util/AtomicTypes.hpp"

using namespace Sirikata;

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

static void check_fill_minitransaction_handler_result(ObjectStorageError& error, bool* done) {
    TS_ASSERT_EQUALS( error.type(), ObjectStorageErrorType_None );
    *done = true;
}

/** Fill a MinitransactionHandler with all the available generated <key,value>
 *  pairs.
 *  \param mth the MinitransactionHandler to fill with <key,value> pairs
 */
static void fill_minitransaction_handler(MinitransactionHandler* mth) {
    Minitransaction* trans = new Minitransaction();
    for(int i = 0; i < OBJECT_STORAGE_GENERATED_PAIRS; i++)
        trans->writes().addPair( keys()[i], values()[i] );

    bool done = false;

    mth->apply(trans, std::tr1::bind(check_fill_minitransaction_handler_result, _1, &done));

    while(!done)
        pollMinitransaction();

    delete trans;
}

// note: we use a StorageSet for expected instead of a ReadSet so we can add pairs to it
static void check_minitransaction_results(ObjectStorageError& error, Minitransaction* trans, ObjectStorageErrorType expected_error, StorageSet expected, bool* done, int testnum) {
    TS_ASSERT_EQUALS( error.type(), expected_error );

    if (expected_error != ObjectStorageErrorType_None) {
        *done = true;
        return;
    }

    TS_ASSERT_EQUALS( trans->reads().size(), expected.size() );

    for(ReadSet::iterator read_it = trans->reads().begin(), expected_it = expected.begin(); read_it != trans->reads().end(); read_it++, expected_it++) {
        TS_ASSERT_EQUALS( read_it->second->size(), expected_it->second->size() );
        TS_ASSERT_EQUALS(
            *read_it->second,
            *expected_it->second
        );
        if (!(*read_it->second==*expected_it->second)) {
            int THIS_TEST_FAILED=-1;
            TS_ASSERT_EQUALS(testnum,THIS_TEST_FAILED)
        }
    }

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
static void test_minitransaction(MinitransactionHandler* mth, Minitransaction* trans, ObjectStorageErrorType expected_error, StorageSet expected, int testnum) {
    bool done = false;

    mth->apply(trans, std::tr1::bind(check_minitransaction_results, _1, trans, expected_error, expected, &done, testnum));

    while( !done )
        pollMinitransaction();

    delete trans;
}

static void check_stress_test_result(ObjectStorageError& error, AtomicValue<Sirikata::uint32>* done) {
    TS_ASSERT_EQUALS( error.type(), ObjectStorageErrorType_None );

    (*done)++;
}


void stress_test_minitransaction_handler(SetupMinitransactionHandlerFunction _setup, CreateMinitransactionHandlerFunction create_handler,
                                         String pl, TeardownMinitransactionHandlerFunction _teardown,
                                         uint32 num_trans, uint32 num_ops)
{
    TS_ASSERT(num_ops <= OBJECT_STORAGE_GENERATED_PAIRS);

    MinitransactionHandlerTestFixture fixture(_setup, create_handler, pl, _teardown);

    // fill up the storage to so the stress tests should not get any failures
    fill_minitransaction_handler(fixture.handler);

    std::vector<Minitransaction*> transactions;

    // generate a bunch of MinitransactionSets with a lot of items in them
    for(uint32 i = 0; i < num_trans; i++) {
        Minitransaction* trans = new Minitransaction();
        for(uint32 j = 0; j < num_ops; j++) {
            trans->reads().addKey( keys()[j] );
            trans->writes().addPair( keys()[j], values()[j] );
        }
        transactions.push_back(trans);
    }

    AtomicValue<Sirikata::uint32> done(0);

    // now quickly submit all of them
    for(uint32 i = 0; i < num_trans; i++)
        fixture.handler->apply(transactions[i], std::tr1::bind(check_stress_test_result, _1, &done));

    while( done.read() < num_trans )
        pollMinitransaction();

    for(uint32 i = 0; i < num_trans; i++)
        delete transactions[i];

    transactions.clear();
}


void test_minitransaction_handler_order(SetupMinitransactionHandlerFunction _setup, CreateMinitransactionHandlerFunction create_handler,
                                        Sirikata::String pl, TeardownMinitransactionHandlerFunction _teardown) {
    MinitransactionHandlerTestFixture fixture(_setup, create_handler, pl, _teardown);

    // 0 reads, 0 writes
    Minitransaction* trans_1 = new Minitransaction();
    StorageSet expected_1;
    test_minitransaction(fixture.handler, trans_1, ObjectStorageErrorType_None, expected_1,1);

    // 0 reads, 1 write
    Minitransaction* trans_2 = new Minitransaction();
    trans_2->writes().addPair( keys()[0], values()[0] );
    StorageSet expected_2;
    test_minitransaction(fixture.handler, trans_2, ObjectStorageErrorType_None, expected_2,2);

    // 1 read from last set's write, 0 writes
    Minitransaction* trans_3 = new Minitransaction();
    trans_3->reads().addKey( keys()[0] );
    StorageSet expected_3;
    expected_3.addPair( keys()[0], values()[0] );
    test_minitransaction(fixture.handler, trans_3, ObjectStorageErrorType_None, expected_3,3);

    // 1 read from previous set, 1 write
    Minitransaction* trans_4 = new Minitransaction();
    trans_4->reads().addKey( keys()[0] );
    trans_4->writes().addPair( keys()[1], values()[1] );
    StorageSet expected_4;
    expected_4.addPair( keys()[0], values()[0] );
    test_minitransaction(fixture.handler, trans_4, ObjectStorageErrorType_None, expected_4,4);

    // 1 successful compare
    Minitransaction* trans_5 = new Minitransaction();
    trans_5->compares().addPair( keys()[0], values()[0] );
    StorageSet expected_5;
    test_minitransaction(fixture.handler, trans_5, ObjectStorageErrorType_None, expected_5,5);

    // 1 failed compare
    Minitransaction* trans_6 = new Minitransaction();
    trans_6->compares().addPair( keys()[0], values()[1] );
    StorageSet expected_6;
    test_minitransaction(fixture.handler, trans_6, ObjectStorageErrorType_ComparisonFailed, expected_6,6);

    // 1 successful compare, 1 read
    Minitransaction* trans_7 = new Minitransaction();
    trans_7->compares().addPair( keys()[0], values()[0] );
    trans_7->reads().addKey( keys()[1] );
    StorageSet expected_7;
    expected_7.addPair( keys()[1], values()[1] );
    test_minitransaction(fixture.handler, trans_7, ObjectStorageErrorType_None, expected_7,7);

    // 1 failed compare, 1 read (which will not be performed)
    Minitransaction* trans_8 = new Minitransaction();
    trans_8->compares().addPair( keys()[0], values()[1] );
    trans_8->reads().addKey( keys()[1] );
    StorageSet expected_8;
    test_minitransaction(fixture.handler, trans_8, ObjectStorageErrorType_ComparisonFailed, expected_8,8);

    // 1 successful compare, 1 read, 1 write
    Minitransaction* trans_9 = new Minitransaction();
    trans_9->compares().addPair( keys()[0], values()[0] );
    trans_9->reads().addKey( keys()[1] );
    trans_9->writes().addPair( keys()[2], values()[2] );
    StorageSet expected_9;
    expected_9.addPair( keys()[1], values()[1] );
    test_minitransaction(fixture.handler, trans_9, ObjectStorageErrorType_None, expected_9,9);

    // 1 failed compare, 1 read, 1 write (which will not be performed)
    Minitransaction* trans_10 = new Minitransaction();
    trans_10->compares().addPair( keys()[0], values()[1] );
    trans_10->reads().addKey( keys()[1] );
    trans_10->writes().addPair( keys()[2], values()[3] );
    StorageSet expected_10;
    test_minitransaction(fixture.handler, trans_10, ObjectStorageErrorType_ComparisonFailed, expected_10,10);


    // finally just verify that the previous write did not happen
    Minitransaction* trans_11 = new Minitransaction();
    trans_11->reads().addKey( keys()[2] );
    StorageSet expected_11;
    expected_11.addPair( keys()[2], values()[2] );
    test_minitransaction(fixture.handler, trans_11, ObjectStorageErrorType_None, expected_11,11);
}
