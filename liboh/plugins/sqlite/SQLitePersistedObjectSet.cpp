/*  Sirikata
 *  SQLitePersistentObjectSet.cpp
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

#include "SQLitePersistedObjectSet.hpp"
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>

#define TABLE_NAME "objects"

namespace Sirikata {
namespace OH {

SQLitePersistedObjectSet::SQLitePersistedObjectSet(ObjectHostContext* ctx, const String& dbpath)
 : mContext(ctx),
   mDBFilename(dbpath),
   mDB()
{
}

SQLitePersistedObjectSet::~SQLitePersistedObjectSet()
{
}

void SQLitePersistedObjectSet::start() {
    // Initialize and start the thread for IO work. This is only separated as a
    // thread rather than strand because we don't have proper multithreading in
    // cppoh.
    mIOService = Network::IOServiceFactory::makeIOService();
    mWork = new Network::IOWork(*mIOService, "SQLitePersistedObjectSet IO Thread");
    mThread = new Sirikata::Thread(std::tr1::bind(&Network::IOService::runNoReturn, mIOService));

    mIOService->post(std::tr1::bind(&SQLitePersistedObjectSet::initDB, this));
}

void SQLitePersistedObjectSet::initDB() {
    SQLiteDBPtr db = SQLite::getSingleton().open(mDBFilename);
    sqlite3_busy_timeout(db->db(), 1000);

    // Create the table for this object if it doesn't exist yet
    String table_create = "CREATE TABLE IF NOT EXISTS ";
    table_create += "\"" TABLE_NAME "\"";
    table_create += "(object TEXT PRIMARY KEY, script_type TEXT, script_args TEXT, script_contents TEXT)";

    int rc;
    char* remain;
    sqlite3_stmt* table_create_stmt;

    rc = sqlite3_prepare_v2(db->db(), table_create.c_str(), -1, &table_create_stmt, (const char**)&remain);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing table create statement");

    rc = sqlite3_step(table_create_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error executing table create statement");
    rc = sqlite3_finalize(table_create_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing table create statement");

    mDB = db;
}

void SQLitePersistedObjectSet::stop() {
    // Just kill the work that keeps the IO thread alive and wait for thread to
    // finish, i.e. for outstanding transactions to complete
    delete mWork;
    mWork = NULL;
    mThread->join();
    delete mThread;
    mThread = NULL;
    Network::IOServiceFactory::destroyIOService(mIOService);
    mIOService = NULL;
}

void SQLitePersistedObjectSet::requestPersistedObject(const UUID& internal_id, const String& script_type, const String& script_args, const String& script_contents, RequestCallback cb) {
    mIOService->post(
        std::tr1::bind(&SQLitePersistedObjectSet::performUpdate, this, internal_id, script_type, script_args, script_contents, cb)
    );
}

void SQLitePersistedObjectSet::performUpdate(const UUID& internal_id, const String& script_type, const String& script_args, const String& script_contents, RequestCallback cb) {
    bool success = true;

    int rc;
    char* remain;
    String value_insert;
    value_insert = "INSERT OR REPLACE INTO ";
    value_insert += "\"" TABLE_NAME "\"";
    value_insert += " (object, script_type, script_args, script_contents) VALUES(?, ?, ?, ?)";

    sqlite3_stmt* value_insert_stmt;
    rc = sqlite3_prepare_v2(mDB->db(), value_insert.c_str(), -1, &value_insert_stmt, (const char**)&remain);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error preparing value insert statement");

    String id_str = internal_id.rawHexData();
    rc = sqlite3_bind_text(value_insert_stmt, 1, id_str.c_str(), (int)id_str.size(), SQLITE_TRANSIENT);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error binding object internal ID to value insert statement");
    rc = sqlite3_bind_text(value_insert_stmt, 2, script_type.c_str(), (int)script_type.size(), SQLITE_TRANSIENT);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error binding script_type name to value insert statement");
    rc = sqlite3_bind_blob(value_insert_stmt, 3, script_args.c_str(), (int)script_args.size(), SQLITE_TRANSIENT);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error binding script_args to value insert statement");
    rc = sqlite3_bind_blob(value_insert_stmt, 4, script_contents.c_str(), (int)script_contents.size(), SQLITE_TRANSIENT);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error binding script_contents to value insert statement");

    int step_rc = SQLITE_OK;
    while(step_rc == SQLITE_OK) {
        step_rc = sqlite3_step(value_insert_stmt);
    }
    if (step_rc != SQLITE_OK && step_rc != SQLITE_DONE) {
        success = false;
        sqlite3_reset(value_insert_stmt); // allow this to be cleaned
                                          // up
    }

    rc = sqlite3_finalize(value_insert_stmt);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing value insert statement");

    if (cb != 0)
        mContext->mainStrand->post(std::tr1::bind(cb, success));
}

} //end namespace OH
} //end namespace Sirikata
