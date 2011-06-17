/*  Sirikata
 *  SQLiteStorage.cpp
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

#include "SQLiteStorage.hpp"
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>

#define TABLE_NAME "persistence"

namespace Sirikata {
namespace OH {

SQLiteStorage::StorageAction::StorageAction()
 : type(Error),
   key(),
   value(NULL)
{
}

SQLiteStorage::StorageAction::StorageAction(const StorageAction& rhs) {
    *this = rhs;
}

SQLiteStorage::StorageAction::~StorageAction() {
    if (value != NULL)
        delete value;
}

SQLiteStorage::StorageAction& SQLiteStorage::StorageAction::operator=(const StorageAction& rhs) {
    type = rhs.type;
    key = rhs.key;
    if (rhs.value != NULL)
        value = new String(*(rhs.value));
    else
        value = NULL;

    return *this;
}

bool SQLiteStorage::StorageAction::execute(SQLiteDBPtr db, const Bucket& bucket, ReadSet* rs) {
    bool success = true;
    switch(type) {
      case Read:
          {
              String value_query = "SELECT value FROM ";
              value_query += "\"" TABLE_NAME "\"";
              value_query += " WHERE object == \'" + bucket.rawHexData() + "\' AND key == ?";
              int rc;
              char* remain;
              sqlite3_stmt* value_query_stmt;
              bool success = true;
              bool newStep = true;
              bool locked = false;
              rc = sqlite3_prepare_v2(db->db(), value_query.c_str(), -1, &value_query_stmt, (const char**)&remain);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value query statement");
              if (rc==SQLITE_OK) {
                  rc = sqlite3_bind_text(value_query_stmt, 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
                  success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error binding key name to value query statement");
                  if (rc==SQLITE_OK) {
                      int step_rc = sqlite3_step(value_query_stmt);
                      while(step_rc == SQLITE_ROW) {
                          newStep = false;
                          (*rs)[key] = String(
                              (const char*)sqlite3_column_text(value_query_stmt, 0),
                              sqlite3_column_bytes(value_query_stmt, 0)
                          );
                          step_rc = sqlite3_step(value_query_stmt);
                      }
                      if (step_rc != SQLITE_DONE) {
                          // reset the statement so it'll clean up properly
                          rc = sqlite3_reset(value_query_stmt);
                          success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");
                          if (rc==SQLITE_LOCKED||rc==SQLITE_BUSY)
                              locked = true;
                      }
                  }
              }
              rc = sqlite3_finalize(value_query_stmt);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");

              if (newStep) { // no rows were found, key is missing
                  success = false;
              }
          }
        break;
      case Write:
      case Erase:
          {
              // Erase and write use different statements, but the rest is the
              // same since it just needs to execute and check for success.
              int rc;
              char* remain;

              String value_insert;
              if (type == Write) {
                  value_insert = "INSERT OR REPLACE INTO ";
                  value_insert += "\"" TABLE_NAME "\"";
                  value_insert += " (object, key, value) VALUES(\'" + bucket.rawHexData() + "\', ?, ?)";
              }
              else if (type == Erase) {
                  value_insert = "DELETE FROM ";
                  value_insert += "\"" TABLE_NAME "\"";
                  value_insert += " WHERE object = \'" + bucket.rawHexData() + "\' AND key = ?";
              }

              sqlite3_stmt* value_insert_stmt;
              rc = sqlite3_prepare_v2(db->db(), value_insert.c_str(), -1, &value_insert_stmt, (const char**)&remain);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value insert statement");

              rc = sqlite3_bind_text(value_insert_stmt, 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error binding key name to value insert statement");
              if (rc==SQLITE_OK) {
                  if (type == Write) {
                      assert(value != NULL);
                      rc = sqlite3_bind_blob(value_insert_stmt, 2, value->c_str(), (int)value->size(), SQLITE_TRANSIENT);
                      success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error binding value to value insert statement");
                  }
              }

              int step_rc = sqlite3_step(value_insert_stmt);
              if (step_rc != SQLITE_OK && step_rc != SQLITE_DONE)
                  sqlite3_reset(value_insert_stmt); // allow this to be cleaned up

              rc = sqlite3_finalize(value_insert_stmt);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value insert statement");
          }
        break;
      case Error:
        SILOG(sqlite-storage, fatal, "Tried to execute an invalid StorageAction.");
        break;
    };
    return success;
}

SQLiteStorage::SQLiteStorage(ObjectHostContext* ctx, const String& dbpath)
 : mContext(ctx),
   mDBFilename(dbpath),
   mDB()
{
}

SQLiteStorage::~SQLiteStorage()
{
}

void SQLiteStorage::start() {
    // Initialize and start the thread for IO work. This is only separated as a
    // thread rather than strand because we don't have proper multithreading in
    // cppoh.
    mIOService = Network::IOServiceFactory::makeIOService();
    mWork = new Network::IOWork(*mIOService, "SQLiteStorage IO Thread");
    mThread = new Sirikata::Thread(std::tr1::bind(&Network::IOService::runNoReturn, mIOService));

    mIOService->post(std::tr1::bind(&SQLiteStorage::initDB, this));
}

void SQLiteStorage::initDB() {
    SQLiteDBPtr db = SQLite::getSingleton().open(mDBFilename);
    sqlite3_busy_timeout(db->db(), 1000);

    // Create the table for this object if it doesn't exist yet
    String table_create = "CREATE TABLE IF NOT EXISTS ";
    table_create += "\"" TABLE_NAME "\"";
    table_create += "(object TEXT, key TEXT, value TEXT, PRIMARY KEY(object, key))";

    int rc;
    char* remain;
    sqlite3_stmt* table_create_stmt;

    rc = sqlite3_prepare_v2(db->db(), table_create.c_str(), -1, &table_create_stmt, (const char**)&remain);
    !SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing table create statement");

    rc = sqlite3_step(table_create_stmt);
    !SQLite::check_sql_error(db->db(), rc, NULL, "Error executing table create statement");
    rc = sqlite3_finalize(table_create_stmt);
    !SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing table create statement");

    mDB = db;
}

bool SQLiteStorage::sqlBeginTransaction() {
    String begin = "BEGIN DEFERRED TRANSACTION";

    int rc;
    char* remain;
    sqlite3_stmt* begin_stmt;
    bool success = true;

    rc = sqlite3_prepare_v2(mDB->db(), begin.c_str(), -1, &begin_stmt, (const char**)&remain);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error preparing begin transaction statement");

    rc = sqlite3_step(begin_stmt);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error executing begin statement");
    rc = sqlite3_finalize(begin_stmt);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing begin statement");

    return success;
}

bool SQLiteStorage::sqlRollback() {
    String rollback = "ROLLBACK TRANSACTION";

    int rc;
    char* remain;
    sqlite3_stmt* rollback_stmt;
    bool success = true;

    rc = sqlite3_prepare_v2(mDB->db(), rollback.c_str(), -1, &rollback_stmt, (const char**)&remain);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error preparing rollback transaction statement");

    rc = sqlite3_step(rollback_stmt);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error executing rollback statement");
    rc = sqlite3_finalize(rollback_stmt);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing rollback statement");

    return success;
}

bool SQLiteStorage::sqlCommit() {
    String commit = "COMMIT TRANSACTION";

    int rc;
    char* remain;
    sqlite3_stmt* commit_stmt;
    bool success = true;

    rc = sqlite3_prepare_v2(mDB->db(), commit.c_str(), -1, &commit_stmt, (const char**)&remain);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error preparing commit transaction statement");

    rc = sqlite3_step(commit_stmt);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error executing commit statement");
    rc = sqlite3_finalize(commit_stmt);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing commit statement");

    return success;
}

void SQLiteStorage::stop() {
    // Just kill the work that keeps the IO thread alive and wait for thread to
    // finish, i.e. for outstanding transactions to complete
    delete mWork;
    mWork = NULL;
    mThread->join();
    delete mThread;
    mThread = NULL;
    Network::IOServiceFactory::destroyIOService(mIOService);
    mIOService = NULL;

    // Clean up data from any outstanding pending transactions
    for(BucketTransactions::iterator it = mTransactions.begin(); it != mTransactions.end(); it++) {
        Transaction* trans = it->second;
        delete trans;
    }
    mTransactions.clear();
}

SQLiteStorage::Transaction* SQLiteStorage::getTransaction(const Bucket& bucket, bool* is_new) {
    if (mTransactions.find(bucket) == mTransactions.end()) {
        if (is_new != NULL) *is_new = true;
        mTransactions[bucket] = new Transaction();
    }

    
    return mTransactions[bucket];
}

void SQLiteStorage::beginTransaction(const Bucket& bucket) {
    // FIXME should probably throw an exception if one already exists
    getTransaction(bucket);
}

void SQLiteStorage::commitTransaction(const Bucket& bucket, const CommitCallback& cb)
{
    Transaction* trans = getTransaction(bucket);

    //can remove from mTransactions
    mTransactions.erase(bucket);
    
    // Short cut for empty transactions. Or maybe these should cause exceptions?
    if(trans->empty()) {
        ReadSet* rs = NULL;
        completeCommit(bucket, trans, cb, false, rs);
        return;
    }
    
    mIOService->post(
        std::tr1::bind(&SQLiteStorage::executeCommit, this, bucket, trans, cb)
    );
}

// Executes a commit. Runs in a separate thread, so the transaction is
// passed in directly
void SQLiteStorage::executeCommit(const Bucket& bucket, Transaction* trans, CommitCallback cb) {
    ReadSet* rs = new ReadSet;

    bool success = true;
    success = sqlBeginTransaction();
    for (Transaction::iterator it = trans->begin(); success && it != trans->end(); it++) {
        success = success && (*it).execute(mDB, bucket, rs);
        if (!success) {
            sqlRollback();
            break;
        }
    }

    if (success) {
        success = sqlCommit();
    }

    if (rs->empty() || !success) {
        success = false;
        delete rs;
        rs = NULL;
    }


    mContext->mainStrand->post(std::tr1::bind(&SQLiteStorage::completeCommit, this, bucket, trans, cb, success, rs));
}



// Complete a commit back in the main thread, cleaning it up and dispatching
// the callback
void SQLiteStorage::completeCommit(const Bucket& bucket, Transaction* trans, CommitCallback cb, bool success, ReadSet* rs) {

    delete trans;
    if (cb) cb(success, rs);
}

bool SQLiteStorage::erase(const Bucket& bucket, const Key& key, const CommitCallback& cb) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Erase;
    action.key = key;

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb);

    return true;
}


bool SQLiteStorage::write(const Bucket& bucket, const Key& key, const String& strToWrite, const CommitCallback& cb) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Write;
    action.key = key;
    action.value = new String(strToWrite);

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb);

    return true;
}

bool SQLiteStorage::read(const Bucket& bucket, const Key& key, const CommitCallback& cb) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Read;
    action.key = key;

    // Run commit if this is a one-off transaction
    if (is_new)
    {
        commitTransaction(bucket, cb);
    }

    return true;
}


} //end namespace OH
} //end namespace Sirikata
