// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <cxxtest/TestSuite.h>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/Thread.hpp>

using namespace Sirikata;

class UUIDTest : public CxxTest::TestSuite {
public:
    void testNoDuplicates() {
        // Generate a bunch of UUIDs and verify we don't see any duplicates. Not
        // a guarantee, but a good sanity check since we had a broken UUID
        // implementation that would repeat after about 10-20k when generating
        // UUIDs quickly.

        std::tr1::unordered_set<UUID, UUID::Hasher> uuids;
        for(uint32 i = 0; i < 1000000; i++) {
            UUID u = UUID::random();
            TS_ASSERT_EQUALS(uuids.find(u), uuids.end());
            uuids.insert(u);
        }
    }

    static void generateUUIDs() {
        for(uint32 i = 0; i < 10000; i++) {
            UUID u = UUID::random();
            // Make sure we use it so it won't get optimized out.
            u.toString();
        }
    }
    void testThreading() {
        // Test threaded creation of random UUIDs
        IOService* ios = new IOService("UUID Thread Test");
        // A bunch of tasks, each with a bunch of UUIDs to generate
        for(uint32 i = 0; i < 100; i++)
            ios->post(std::tr1::bind(&UUIDTest::generateUUIDs));
        std::vector<Thread*> threads;
        for(int i = 0; i < 4; i++) {
            threads.push_back(
                new Thread(
                    "UUID Thread Test",
                    std::tr1::bind(&Network::IOService::runNoReturn, ios)
                )
            );
        }
        // Threads should exit automatically once the tasks run out
        for(uint32 i = 0; i < threads.size(); i++)
            threads[i]->join();
    }
};
