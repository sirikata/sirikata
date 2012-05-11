/*  Sirikata SQLite Plugin
 *  SQLite.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#include <sirikata/sqlite/SQLite.hpp>
#include <boost/thread.hpp>
#include <boost/thread/once.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::SQLite);

namespace Sirikata {

std::pair<bool, String> SQLite::check_sql_error(sqlite3* db, int rc, char** sql_error_msg, const String& msg) {
    String result_msg;
    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {
        result_msg = msg + " ... ";
        if (sql_error_msg && *sql_error_msg) {
            result_msg += *sql_error_msg;
            sqlite3_free(*sql_error_msg);
            *sql_error_msg = NULL;
        }
        else {
            result_msg += sqlite3_errmsg(db);
        }
        return std::make_pair(true, result_msg);
    }
    return std::make_pair(false, result_msg);
}

static std::tr1::unordered_map<int, String> rcStrings;
static boost::once_flag rcStringsInitialized = BOOST_ONCE_INIT;
static void initResultCodeStrings() {
    // Unknown values
    rcStrings[-1] = "Unknown";
    // Normal values

    rcStrings[SQLITE_OK] = "SQLITE_OK - Ok";
    rcStrings[SQLITE_ERROR] = "SQLITE_ERROR - SQL error or missing database";
    rcStrings[SQLITE_INTERNAL] = "SQLITE_INTERNAL - Internal logic error in SQLite";
    rcStrings[SQLITE_PERM] = "SQLITE_PERM - Access permission denied";
    rcStrings[SQLITE_ABORT] = "SQLITE_ABORT - Callback routine requested an abort";
    rcStrings[SQLITE_BUSY] = "SQLITE_BUSY - The database file is locked";
    rcStrings[SQLITE_LOCKED] = "SQLITE_LOCKED - A table in the database is locked";
    rcStrings[SQLITE_NOMEM] = "SQLITE_NOMEM - A malloc() failed";
    rcStrings[SQLITE_READONLY] = "SQLITE_READONLY - Attempt to write a readonly database";
    rcStrings[SQLITE_INTERRUPT] = "SQLITE_INTERRUPT - Operation terminated by sqlite3_interrupt()";
    rcStrings[SQLITE_IOERR] = "SQLITE_IOERR - Some kind of disk I/O error occurred";
    rcStrings[SQLITE_CORRUPT] = "SQLITE_CORRUPT - The database disk image is malformed";
    rcStrings[SQLITE_NOTFOUND] = "SQLITE_NOTFOUND - Unknown opcode in sqlite3_file_control()";
    rcStrings[SQLITE_FULL] = "SQLITE_FULL - Insertion failed because database is full";
    rcStrings[SQLITE_CANTOPEN] = "SQLITE_CANTOPEN - Unable to open the database file";
    rcStrings[SQLITE_PROTOCOL] = "SQLITE_PROTOCOL - Database lock protocol error";
    rcStrings[SQLITE_EMPTY] = "SQLITE_EMPTY - Database is empty";
    rcStrings[SQLITE_SCHEMA] = "SQLITE_SCHEMA - The database schema changed";
    rcStrings[SQLITE_TOOBIG] = "SQLITE_TOOBIG - String or BLOB exceeds size limit";
    rcStrings[SQLITE_CONSTRAINT] = "SQLITE_CONSTRAINT - Abort due to constraint violation";
    rcStrings[SQLITE_MISMATCH] = "SQLITE_MISMATCH - Data type mismatch";
    rcStrings[SQLITE_MISUSE] = "SQLITE_MISUSE - Library used incorrectly";
    rcStrings[SQLITE_NOLFS] = "SQLITE_NOLFS - Uses OS features not supported on host";
    rcStrings[SQLITE_AUTH] = "SQLITE_AUTH - Authorization denied";
    rcStrings[SQLITE_FORMAT] = "SQLITE_FORMAT - Auxiliary database format error";
    rcStrings[SQLITE_RANGE] = "SQLITE_RANGE - 2nd parameter to sqlite3_bind out of range";
    rcStrings[SQLITE_NOTADB] = "SQLITE_NOTADB - File opened that is not a database file";
    rcStrings[SQLITE_ROW] = "SQLITE_ROW - sqlite3_step() has another row ready";
    rcStrings[SQLITE_DONE] = "SQLITE_DONE - sqlite3_step() has finished executing";
}

const String& SQLite::resultAsString(int rc) {
    boost::call_once(rcStringsInitialized, initResultCodeStrings);

    std::tr1::unordered_map<int, String>::iterator it = rcStrings.find(rc);
    if (it != rcStrings.end()) return it->second;
    return rcStrings[-1];
}

SQLiteDB::SQLiteDB(const String& name) {
    int rc;

    rc = sqlite3_open( name.c_str(), &mObjectDB );
    if (rc) {
        std::string errormsg = std::string("Couldn't open specified object DB: ") + String( sqlite3_errmsg(mObjectDB) );
        sqlite3_close(mObjectDB);
        throw std::runtime_error(errormsg);
    }
}

SQLiteDB::~SQLiteDB() {
    sqlite3_close(mObjectDB);
}

sqlite3* SQLiteDB::db() const {
    return mObjectDB;
}

namespace {
boost::shared_mutex sSingletonMutex;
}

SQLite::SQLite() {
}

SQLite::~SQLite() {
}

SQLite& SQLite::getSingleton() {
    boost::unique_lock<boost::shared_mutex> lck(sSingletonMutex);

    return AutoSingleton<SQLite>::getSingleton();
}

void SQLite::destroy() {
    boost::unique_lock<boost::shared_mutex> lck(sSingletonMutex);

    AutoSingleton<SQLite>::destroy();
}

SQLiteDBPtr SQLite::open(const String& name) {
    UpgradeLock upgrade_lock(mDBMutex);

    // First get the thread local storage for this database
    DBMap::iterator it = mDBs.find(name);
    if (it == mDBs.end()) {
        // the thread specific store hasn't even been allocated
        UpgradedLock upgraded_lock(upgrade_lock);
        // verify another thread hasn't added it, then add it
        it = mDBs.find(name);
        if (it == mDBs.end()) {
            std::tr1::shared_ptr<ThreadDBPtr> newDb(new ThreadDBPtr());
            mDBs[name]=newDb;
            it = mDBs.find(name);
        }
    }
    assert(it != mDBs.end());
    std::tr1::shared_ptr<ThreadDBPtr> thread_db_ptr_ptr = it->second;
    assert(thread_db_ptr_ptr);

    // Now get the thread specific weak database connection
    WeakSQLiteDBPtr* weak_db_ptr_ptr = thread_db_ptr_ptr->get();
    if (weak_db_ptr_ptr == NULL) {
        // we don't have a weak pointer for this thread
        UpgradedLock upgraded_lock(upgrade_lock);
        weak_db_ptr_ptr = thread_db_ptr_ptr->get();
        if (weak_db_ptr_ptr == NULL) { // verify and create
            weak_db_ptr_ptr = new WeakSQLiteDBPtr();
            thread_db_ptr_ptr->reset( weak_db_ptr_ptr );
        }
    }
    assert(weak_db_ptr_ptr != NULL);

    SQLiteDBPtr db;
    db = weak_db_ptr_ptr->lock();
    if (!db) {
        // the weak pointer for this thread is NULL
        UpgradedLock upgraded_lock(upgrade_lock);
        db = weak_db_ptr_ptr->lock();
        if (!db) {
            db = SQLiteDBPtr(new SQLiteDB(name));
            *weak_db_ptr_ptr = db;
        }
    }
    assert(db);

    return db;
}

} // namespace Sirikata
