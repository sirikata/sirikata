/*  Sirikata
 *  ThreadingTest.hpp
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

#include <cxxtest/TestSuite.h>
#include <sirikata/sqlite/SQLite.hpp>
#include <sirikata/core/util/Thread.hpp>

class SQLiteThreadingTest : public CxxTest::TestSuite
{
    //using Sirikata::SQLiteDBPtr;
    static const String dbfile;
public:

    void testOpen(void) {
        SQLiteDBPtr db = SQLite::getSingleton().open(dbfile);
        TS_ASSERT(db);
        TS_ASSERT_DIFFERS(db->db(), (sqlite3*)NULL);
        // Used throughout as a test that a call to the database actually works.
        sqlite3_busy_timeout(db->db(), 1000);
    }

    void testOpenSequential(void) {
        {
            SQLiteDBPtr db = SQLite::getSingleton().open(dbfile);
            TS_ASSERT(db);
            TS_ASSERT_DIFFERS(db->db(), (sqlite3*)NULL);
            sqlite3_busy_timeout(db->db(), 1000);
        }

        {
            SQLiteDBPtr db2 = SQLite::getSingleton().open(dbfile);
            TS_ASSERT(db2);
            TS_ASSERT_DIFFERS(db2->db(), (sqlite3*)NULL);
            sqlite3_busy_timeout(db2->db(), 1000);
        }
    }

    void testOpenMultiple(void) {
        SQLiteDBPtr db = SQLite::getSingleton().open(dbfile);
        TS_ASSERT(db);
        TS_ASSERT_DIFFERS(db->db(), (sqlite3*)NULL);
        sqlite3_busy_timeout(db->db(), 1000);

        SQLiteDBPtr db2 = SQLite::getSingleton().open(dbfile);
        TS_ASSERT(db2);
        TS_ASSERT_DIFFERS(db2->db(), (sqlite3*)NULL);
        sqlite3_busy_timeout(db2->db(), 1000);

        TS_ASSERT_EQUALS(db->db(), db2->db());
    }

    void threadMain(void) {
        SQLiteDBPtr db = SQLite::getSingleton().open(dbfile);
        TS_ASSERT(db);
        TS_ASSERT_DIFFERS(db->db(), (sqlite3*)NULL);
        sqlite3_busy_timeout(db->db(), 1000);
    }

    void testThread(void) {
        Thread t1(std::tr1::bind(&SQLiteThreadingTest::threadMain, this));
        t1.join();
    }

    void testMultipleThread(void) {
        std::vector<Thread*> threads;
        for(int i = 0; i < 10; i++)
            threads.push_back(new Thread(std::tr1::bind(&SQLiteThreadingTest::threadMain, this)));
        for(int i = 0; i < 10; i++) {
            threads[i]->join();
            delete threads[i];
        }
    }
};

const String SQLiteThreadingTest::dbfile("test.db");
