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
    void testParse() {
        // This is what we really mean by human readable
        UUID good1("01234567-8901-2345-6789-012345678901", UUID::HumanReadable());
        TS_ASSERT_EQUALS(good1.toString(), "01234567-8901-2345-6789-012345678901");

        // We can actually parse this as "human readable", but only test that we
        // can parse it as hex string
        UUID good2("01234567890123456789012345678901", UUID::HexString());
        TS_ASSERT_EQUALS(good2.toString(), "01234567-8901-2345-6789-012345678901");
    }

    void testInvalidParse() {
        UUID bogus1("not a uuid", UUID::HumanReadable());
        TS_ASSERT_EQUALS(bogus1, UUID::null());

        UUID bogus2("", UUID::HumanReadable());
        TS_ASSERT_EQUALS(bogus2, UUID::null());
    }

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
        Network::IOService* ios = new Network::IOService("UUID Thread Test");
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
