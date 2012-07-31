// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <cxxtest/TestSuite.h>

#include <sirikata/core/util/Thread.hpp>
#include <sirikata/core/network/IOStrand.hpp>


using namespace Sirikata;

class StrandTest : public CxxTest::TestSuite {
    IOService* ios;
    IOStrand* strand;
    IOWork* work;
    std::vector<Thread*> threads;

public:
    void setUp() {
        ios = new IOService("StrandTest");
        strand = ios->createStrand("StrandTest Strand");
        work = new IOWork(ios);
        for(int i = 0; i < 4; i++) {
            threads.push_back(
                new Thread(
                    "StrandTest Thread",
					std::tr1::bind(&Network::IOService::runNoReturn, ios)
                )
            );
        }
    }

    void waitForWorkers() {
        if (work != NULL) {
            delete work; work = NULL;
        }
        for(int i = 0; i < threads.size(); i++) {
            threads[i]->join();
        }
        threads.clear();
    }
    void tearDown() {
        waitForWorkers();
        delete strand; strand = NULL;
        delete ios; ios = NULL;
    }

    void checkHandlerOrder(int my_id, int* nextHandlerToExecute) {
        TS_ASSERT_EQUALS(my_id, *nextHandlerToExecute);
        *nextHandlerToExecute += 1;
    }
    void testHandlerOrder() {
        // To check handler order, just generate a ton of them and allow their
        // handlers to check that they are executing in the expected order. Note
        // we use posting here because post+dispatch have ordering guarantees,
        // whereas wrap does not.

        int nextHandlerToExecute = 0;
        for(int i = 0; i < 1000000; i++) {
            strand->post( std::tr1::bind(&StrandTest::checkHandlerOrder, this, i, &nextHandlerToExecute) );
        }

        // Need nextHandlerToExecute to remain valid until workers finish
        waitForWorkers();
    }
};
