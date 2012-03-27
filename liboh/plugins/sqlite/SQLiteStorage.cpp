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
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>

#define TABLE_NAME "persistence"
#define LEASE_KEY "_____lease_____"

namespace Sirikata {
namespace OH {

/** Implementation Notes
 *  --------------------
 *
 *  This implementation is fairly straightforward since SQLite takes
 *  care of almost everything for us. A single table 'persistence'
 *  holds all data in rows of (object, key, value). All operations
 *  except leases are trivially handled through normal SQLite
 *  transactions.
 *
 *  Leases require a little more work since they span transactions. We
 *  use a special key to keep track of the lessee and expiration. When
 *  transactions are initiated we make sure we have the lease, and
 *  also setup renewals. Checking at the beginning of the transaction
 *  is sufficient because as soon as we read the data, we have a
 *  reader lock and the transaction won't complete if someone else
 *  tried to write to it.
 */


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
    keyEnd = rhs.keyEnd;
    if (rhs.value != NULL)
        value = new String(*(rhs.value));
    else
        value = NULL;

    return *this;
}

Storage::Result SQLiteStorage::StorageAction::execute(SQLiteDBPtr db, const Bucket& bucket, ReadSet* rs) {
    Result result = SUCCESS;
    switch(type) {
        // Read and Compare are identical except that read stores the value and
        // compare checks against its reference data.
      case Read:
      case Compare:
          {
              String value_query = "SELECT value FROM ";
              value_query += "\"" TABLE_NAME "\"";
              value_query += " WHERE object == \'" + bucket.rawHexData() + "\' AND key == ?";
              int rc;
              char* remain;
              sqlite3_stmt* value_query_stmt;
              bool newStep = true;
              bool locked = false;
              rc = sqlite3_prepare_v2(db->db(), value_query.c_str(), -1, &value_query_stmt, (const char**)&remain);
              bool success = true;
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value query statement");
              if (rc==SQLITE_OK) {
                  rc = sqlite3_bind_text(value_query_stmt, 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
                  success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error binding key name to value query statement");
                  if (rc==SQLITE_OK) {
                      int step_rc = sqlite3_step(value_query_stmt);
                      while(step_rc == SQLITE_ROW) {
                          newStep = false;
                          if (type == Read) {
                              (*rs)[key] = String(
                                  (const char*)sqlite3_column_text(value_query_stmt, 0),
                                  sqlite3_column_bytes(value_query_stmt, 0)
                              );
                          }
                          else if (type == Compare) {
                              assert(value != NULL);
                              String db_val(
                                  (const char*)sqlite3_column_text(value_query_stmt, 0),
                                  sqlite3_column_bytes(value_query_stmt, 0)
                              );
                              success = success && (db_val == *value);
                          }
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

              if (!success)
                  result = TRANSACTION_ERROR;
          }
        break;
      case ReadRange:
          {
              String value_query = "SELECT key, value FROM ";
              value_query += "\"" TABLE_NAME "\"";
              value_query += " WHERE object == \'" + bucket.rawHexData() + "\' AND key BETWEEN ? AND ?";

              int rc;
              char* remain;
              sqlite3_stmt* value_query_stmt;
              rc = sqlite3_prepare_v2(db->db(), value_query.c_str(), -1, &value_query_stmt, (const char**)&remain);
              bool success = true;
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value query statement");
              if (rc==SQLITE_OK){
                  rc = sqlite3_bind_text(value_query_stmt, 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
                  success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error binding start key to value query statement");
                  rc = sqlite3_bind_text(value_query_stmt, 2, keyEnd.c_str(), (int)keyEnd.size(), SQLITE_TRANSIENT);
                  success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error binding finish key to value query statement");
                  if (rc==SQLITE_OK) {
                      int step_rc = sqlite3_step(value_query_stmt);
                      int nread = 0;
                      while(step_rc == SQLITE_ROW) {
                          nread++;
                          String key(
                              (const char*)sqlite3_column_text(value_query_stmt, 0),
                              sqlite3_column_bytes(value_query_stmt, 0)
                          );
                          String value(
                              (const char*)sqlite3_column_text(value_query_stmt, 1),
                              sqlite3_column_bytes(value_query_stmt, 1)
                          );
                          (*rs)[key] = value;
                          step_rc = sqlite3_step(value_query_stmt);
                      }
                      if (nread == 0)
                          success = false;
                      if (step_rc != SQLITE_DONE) {
                          // reset the statement so it'll clean up properly
                          rc = sqlite3_reset(value_query_stmt);
                          success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");
                      }
                  }
              }
              rc = sqlite3_finalize(value_query_stmt);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");
              if (!success)
                  result = TRANSACTION_ERROR;
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
              bool success = true;
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
              if (step_rc != SQLITE_OK && step_rc != SQLITE_DONE) {
                  sqlite3_reset(value_insert_stmt); // allow this to be cleaned up
                  success = false;
              }
              else {
                  // Check the number of changes that the statement actually
                  // made. This is update, insertion, or deletion. This should
                  // just be 1 since we expect exactly one change on a
                  // write. On an erase, we ignore missing keys, but
                  // we should see either 0 or 1 ops.
                  int changes = sqlite3_changes(db->db());
                  if (type == Write)
                      success = success && (changes == 1);
                  else if (type == Erase)
                      success = success && (changes == 0 || changes == 1);
              }

              rc = sqlite3_finalize(value_insert_stmt);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value insert statement");

              if (!success)
                  result = TRANSACTION_ERROR;
          }
        break;
      case EraseRange:
          {
              String value_delete = "DELETE FROM ";
              value_delete += "\"" TABLE_NAME "\"";
              value_delete += " WHERE object = \'" + bucket.rawHexData() + "\' AND key BETWEEN ? AND ?";

              int rc;
              char* remain;
              sqlite3_stmt* value_delete_stmt;
              rc = sqlite3_prepare_v2(db->db(), value_delete.c_str(), -1, &value_delete_stmt, (const char**)&remain);
              bool success = true;
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value delete statement");

              rc = sqlite3_bind_text(value_delete_stmt, 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error binding start key to value delete statement");
              rc = sqlite3_bind_text(value_delete_stmt, 2, keyEnd.c_str(), (int)keyEnd.size(), SQLITE_TRANSIENT);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error binding finish key to value delete statement");

              int step_rc = sqlite3_step(value_delete_stmt);
              if (step_rc != SQLITE_OK && step_rc != SQLITE_DONE)
                  sqlite3_reset(value_delete_stmt); // allow this to be cleaned up

              rc = sqlite3_finalize(value_delete_stmt);
              success = success && !SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value delete statement");

              if (!success)
                  result = TRANSACTION_ERROR;
          }
          break;
      case Error:
        SILOG(sqlite-storage, fatal, "Tried to execute an invalid StorageAction.");
        break;
    };
    return result;
}

SQLiteStorage::SQLiteStorage(ObjectHostContext* ctx, const String& dbpath, const Duration& lease_duration)
 : mContext(ctx),
   mDBFilename(dbpath),
   mDB(),
   mIOService(NULL),
   mWork(NULL),
   mThread(NULL),
   // A random UUID is good, but including some other identifying
   // information in here would be better, e.g. process ID, MAC
   // address, etc.
   mSQLClientID(UUID::random().rawHexData()),
   mLeaseDuration(lease_duration),
   mTransactionQueue(std::tr1::bind(&SQLiteStorage::postProcessTransactions, this)),
   mMaxCoalescedTransactions(5),
   mRenewTimer()
{
}

SQLiteStorage::~SQLiteStorage()
{
}

void SQLiteStorage::start() {
    // Initialize and start the thread for IO work. This is only separated as a
    // thread rather than strand because we don't have proper multithreading in
    // cppoh.
    mIOService = new Network::IOService("SQLiteStorage");
    mWork = new Network::IOWork(*mIOService, "SQLiteStorage IO Thread");
    mThread = new Sirikata::Thread("SQLiteStorage IO", std::tr1::bind(&Network::IOService::runNoReturn, mIOService));

    mIOService->post(std::tr1::bind(&SQLiteStorage::initDB, this), "SQLiteStorage::initDB");

    mRenewTimer = Network::IOTimer::create(
        mIOService,
        std::tr1::bind(&SQLiteStorage::processRenewals, this)
    );
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
    SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing table create statement");

    rc = sqlite3_step(table_create_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error executing table create statement");
    rc = sqlite3_finalize(table_create_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing table create statement");

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
    // finish, i.e. for outstanding transactions to complete. Stop the renewal
    // timer immediately to avoid having to wait for it to fire again (possibly
    // locking things up until it does).
    mRenewTimer->cancel();

    delete mWork;
    mWork = NULL;
    mThread->join();
    mRenewTimer.reset();
    delete mThread;
    mThread = NULL;
    delete mIOService;
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

void SQLiteStorage::leaseBucket(const Bucket& bucket) {
    // Don't do anything on lease, we'll take care of it within the commit.
}

void SQLiteStorage::releaseBucket(const Bucket& bucket) {
    // Have the storage thread release the lease. It's possible to get these
    // calls on final cleanup (after stop() was called) so we need to make sure
    // we can still safely do this
    if (mIOService == NULL) return;

    mIOService->post(
        std::tr1::bind(&SQLiteStorage::releaseLease, this, bucket),
        "SQLiteStorage::releaseLease"
    );
}

void SQLiteStorage::beginTransaction(const Bucket& bucket) {
    // FIXME should probably throw an exception if one already exists
    getTransaction(bucket);
}

void SQLiteStorage::commitTransaction(const Bucket& bucket, const CommitCallback& cb, const String& timestamp)
{
    Transaction* trans = getTransaction(bucket);

    //can remove from mTransactions
    mTransactions.erase(bucket);

    // Short cut for empty transactions. Or maybe these should cause exceptions?
    if(trans->empty()) {
        ReadSet* rs = NULL;
        if (cb) cb(SUCCESS, rs);
        return;
    }

    mTransactionQueue.push(
        TransactionData(bucket, trans, cb)
    );
}

void SQLiteStorage::postProcessTransactions() {
    mIOService->post(
        std::tr1::bind(&SQLiteStorage::processTransactions, this),
        "SQLiteStorage::processTransactions"
    );
}


void SQLiteStorage::processTransactions() {


    while(!mTransactionQueue.empty()) {

        // Try to execute up to the maximum number of coalesced transactions so
        // long as we don't encounter a failure for some reason.
        std::vector<TransactionData> transactions;
        std::vector<ReadSet*> read_sets;

        Result result = SUCCESS;
        if (!sqlBeginTransaction())
            result = LOCK_ERROR;
        for(uint32 i = 0;
            (result == SUCCESS) && !mTransactionQueue.empty() && i < mMaxCoalescedTransactions;
            i++)
        {
            TransactionData data;
            bool popped = mTransactionQueue.pop(data);
            assert(popped);
            transactions.push_back(data);

            ReadSet* cur_result = NULL;
            result = executeCommit(data.bucket, data.trans, data.cb, &cur_result);
            if (result == SUCCESS) {
                read_sets.push_back(cur_result);
            }
        }

        // If we succeeded so far, try to commit and move on
        if (result == SUCCESS) {
            if (!sqlCommit())
                result = LOCK_ERROR;
        }
        // If still successful, cleanup, post callbacks, and move on to next
        // round
        if (result == SUCCESS) {
            for(uint32 i = 0; i < transactions.size(); i++) {
                delete transactions[i].trans;
                if (transactions[i].cb) {
                    mContext->mainStrand->post(
                        std::tr1::bind(transactions[i].cb, SUCCESS, read_sets[i]),
                        "SQLiteStorage completeCommit"
                    );
                }
            }
            continue;
        }

        // We'll only get here if we, for some reason, failed to process all of
        // these. Rollback, clean up results we had gotten, and work back
        // through them one at a time.
        sqlRollback();
        for(uint32 i = 0; i < read_sets.size(); i++)
            if (read_sets[i] != NULL) delete read_sets[i];
        read_sets.clear();

        for(uint32 i = 0; i < transactions.size(); i++) {
            result = SUCCESS;
            if (!sqlBeginTransaction())
                result = LOCK_ERROR;

            ReadSet* rs = NULL;
            TransactionData& data = transactions[i];
            result = executeCommit(data.bucket, data.trans, data.cb, &rs);

            if (result == SUCCESS) {
                if (!sqlCommit())
                    result = LOCK_ERROR;
            }

            // Either way, we need to clean up the transaction
            delete data.trans;
            data.trans = NULL;

            if (result != SUCCESS) {
                sqlRollback();
                delete rs;
                rs = NULL;
            }

            //actually have to check if there's a callback here.  otherwise failure.
            if (data.cb)
            {
                mContext->mainStrand->post(
                    std::tr1::bind(data.cb, result, rs),
                    "SQLiteStorage completeCommit"
                );
            }
        }

    }
}

// Executes a commit. Runs in a separate thread, so the transaction is
// passed in directly
Storage::Result SQLiteStorage::executeCommit(const Bucket& bucket, Transaction* trans, CommitCallback cb, ReadSet** read_set_out) {
    ReadSet* rs = new ReadSet;

    // All these operations check the current result first, so if anything
    // fails, including acquiring the lease, we'll just fall through, cleanup,
    // and return the error.
    Result result = acquireLease(bucket);
    for (Transaction::iterator it = trans->begin(); (result == SUCCESS) && it != trans->end(); it++) {
        result = (*it).execute(mDB, bucket, rs);
    }

    if (rs->empty() || (result != SUCCESS)) {
        delete rs;
        rs = NULL;
    }

    *read_set_out = rs;
    return result;
}

String SQLiteStorage::getLeaseString() {
    Time expires = mContext->realTime() + mLeaseDuration;
    return mSQLClientID + "-" + boost::lexical_cast<String>(expires.raw());
}

void SQLiteStorage::parseLeaseString(const String& ls, String* client_out, Time* expiration_out) {
    if (ls.empty()) {
        *client_out = "";
        *expiration_out = Time::null();
        return;
    }

    std::size_t split_pos = ls.find('-');
    *client_out = ls.substr(0, split_pos);
    *expiration_out = Time( boost::lexical_cast<uint64>( ls.substr(split_pos+1) ) );
}

Storage::Result SQLiteStorage::acquireLease(const Bucket& bucket) {
    // This happens within the context of a commit (the first one against this
    // bucket), so we should already be in a transaction.

    // TODO we should probably escape based on the lock key to avoid
    // conflicts. Maybe prefix \0 for internal and \1 for regular
    // keys?

    // Look up lease info
    Result result;
    ReadSet lease_rs;
    {
        StorageAction sa;
        sa.type = StorageAction::Read;
        sa.key = LEASE_KEY;
        result = sa.execute(mDB, bucket, &lease_rs);
    }

    // Decide the next course of action based on whether the lease key
    // was there, and if so, what's in its contents.
    bool already_own_lease = false;
    bool try_to_acquire_lease = false;

    // If we were successful, parse the lease info
    if (result == SUCCESS) {
        String lease_owner;
        Time lease_expiration;
        parseLeaseString(lease_rs[LEASE_KEY], &lease_owner, &lease_expiration);

        bool expired = mContext->realTime() > lease_expiration;
        bool owned = (lease_owner == mSQLClientID);

        if (owned && !expired)
            already_own_lease = true;
        else if (lease_owner.empty() || expired)
            try_to_acquire_lease = true;
        // Default case covers another owner and default values above
        // indicate we don't own the lease and shouldn't try to
        // acquire it.
    }
    // If we weren't successful, then the key wasn't even there -- we
    // can try to acquire it
    else if (result == TRANSACTION_ERROR) {
        try_to_acquire_lease = true;
    }

    // Now we're ready to take some action. In some cases, it's time
    // to give up:
    if (!already_own_lease && !try_to_acquire_lease)
        return LOCK_ERROR;
    // Or, we may need to try acquiring the lease
    if (try_to_acquire_lease) {
        StorageAction sa;
        sa.type = StorageAction::Write;
        sa.key = LEASE_KEY;
        sa.value = new String(getLeaseString());
        ReadSet no_rs;
        result = sa.execute(mDB, bucket, &no_rs);

        // If we succeeded here, we got the lease, otherwise we failed
        // and need to give up.
        if (result != SUCCESS)
            return LOCK_ERROR;

        // We now have a new lease, setup renewal process. There's no guarantee
        // we'll get back to this in time, but we'll make a best effort by
        // renewing after half the time has expired.
        mRenewTimes.push( BucketRenewTimeout(bucket, Timer::now() + (mLeaseDuration/2)) );
        if (mRenewTimes.size() == 1)
            mRenewTimer->wait(mLeaseDuration/2);
    }

    // And finally, if we got here then we either had or acquired the
    // lease, so we can return success and let the transaction continue.
    return SUCCESS;
}

void SQLiteStorage::renewLease(const Bucket& bucket) {
    // Basic idea here is to lookup the lease to verify we still own it, then
    // update it. We need to wrap this in a SQLite transaction ourselves since
    // it happens on its own.

    Result result = SUCCESS;
    if (!sqlBeginTransaction())
        result = LOCK_ERROR;

    // Look up lease info
    ReadSet lease_rs;
    {
        StorageAction sa;
        sa.type = StorageAction::Read;
        sa.key = LEASE_KEY;
        result = sa.execute(mDB, bucket, &lease_rs);
    }

    // Nothing in there? releaseLease was called and removed it (or something
    // else went wrong...). This means we should stop trying to renew at all.
    if (result == TRANSACTION_ERROR) {
        sqlRollback();
        return;
    }

    // Check our ownership
    String lease_owner;
    Time lease_expiration;
    parseLeaseString(lease_rs[LEASE_KEY], &lease_owner, &lease_expiration);
    // Disregard the lease timing info, just check if we're current owner
    if (lease_owner != mSQLClientID) {
        // Could hit this if we released the lease and someone else took
        // it. Ignore.
        sqlRollback();
        return;
    }

    // We own it, so just update it.
    {
        StorageAction sa;
        sa.type = StorageAction::Write;
        sa.key = LEASE_KEY;
        sa.value = new String(getLeaseString());
        ReadSet no_rs;
        result = sa.execute(mDB, bucket, &no_rs);
    }

    // If we failed to write the new key, give up. This really shouldn't happen.
    if (result != SUCCESS) {
        sqlRollback();
        return;
    }

    // Do the commit, giving up if we fail to get the commit through.
    if (!sqlCommit()) {
        sqlRollback();
        return;
    }

    // We now have a new lease, setup renewal process. There's no guarantee
    // we'll get back to this in time, but we'll make a best effort by
    // renewing after half the time has expired.
    mRenewTimes.push( BucketRenewTimeout(bucket, Timer::now() + (mLeaseDuration/2)) );
}

void SQLiteStorage::releaseLease(const Bucket& bucket) {
    // Basic idea here is to lookup the lease to verify we still own it, then
    // clear it if necessary. We need to wrap this in a SQLite transaction
    // ourselves since it happens on its own.

    Result result = SUCCESS;
    if (!sqlBeginTransaction())
        result = LOCK_ERROR;

    // Look up lease info
    ReadSet lease_rs;
    {
        StorageAction sa;
        sa.type = StorageAction::Read;
        sa.key = LEASE_KEY;
        result = sa.execute(mDB, bucket, &lease_rs);
    }

    // Nothing in there? Nothing to do, although it might indicate a problem
    // since we shouldn't be trying to release leases we don't have.
    if (result == TRANSACTION_ERROR) {
        // However, we don't report an error here because this can also happen
        // if we never actually acquire the lease. Since we acquire it on
        // demand, an object which does no transactions may not actually need to
        // clear out the lease. We could alternatively track which objects we
        // have leases for and only try to clear it if we had a lease.
        sqlRollback();
        return;
    }

    // Otherwise, check our ownership
    String lease_owner;
    Time lease_expiration;
    parseLeaseString(lease_rs[LEASE_KEY], &lease_owner, &lease_expiration);
    // Disregard the lease timing info, just check if we're current owner
    if (lease_owner != mSQLClientID) {
        // As above, if we never actually took the lease, we could hit this
        // condition when it's not a real error. We don't report it for that
        // reason.
        sqlRollback();
        return;
    }

    // Clear the lease key.
    {
        StorageAction sa;
        sa.type = StorageAction::Erase;
        sa.key = LEASE_KEY;
        ReadSet no_rs;
        result = sa.execute(mDB, bucket, &no_rs);
    }

    if (result != SUCCESS) {
        SILOG(sqlite-storage, error, "Failed to release valid lease for bucket " << bucket);
        sqlRollback();
        return;
    }

    // Commit
    if (!sqlCommit())
        sqlRollback();
}


void SQLiteStorage::processRenewals() {
    Time tnow = Timer::now();

    while(!mRenewTimes.empty() && mRenewTimes.front().t < tnow) {
        renewLease(mRenewTimes.front().bucket);
        mRenewTimes.pop();
    }

    if (!mRenewTimes.empty())
        mRenewTimer->wait(mRenewTimes.front().t - tnow);
}



bool SQLiteStorage::erase(const Bucket& bucket, const Key& key, const CommitCallback& cb, const String& timestamp) {
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

bool SQLiteStorage::write(const Bucket& bucket, const Key& key, const String& strToWrite, const CommitCallback& cb, const String& timestamp) {
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

bool SQLiteStorage::read(const Bucket& bucket, const Key& key, const CommitCallback& cb, const String& timestamp) {
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

bool SQLiteStorage::compare(const Bucket& bucket, const Key& key, const String& value, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Compare;
    action.key = key;
    action.value = new String(value);

    // Run commit if this is a one-off transaction
    if (is_new)
    {
        commitTransaction(bucket, cb);
    }

    return true;

}

bool SQLiteStorage::rangeRead(const Bucket& bucket, const Key& start, const Key& finish, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::ReadRange;
    action.key = start;
    action.keyEnd = finish;
    // Run commit if this is a one-off transaction
    if (is_new)
    {
        commitTransaction(bucket, cb);
    }

    return true;
}

bool SQLiteStorage::rangeErase(const Bucket& bucket, const Key& start, const Key& finish, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::EraseRange;
    action.key = start;
    action.keyEnd = finish;
    // Run commit if this is a one-off transaction
    if (is_new)
    {
        commitTransaction(bucket, cb);
    }

    return true;
}

bool SQLiteStorage::count(const Bucket& bucket, const Key& start, const Key& finish, const CountCallback& cb, const String& timestamp) {
    // FIXME doesn't fit into transactions...
    String value_count = "SELECT COUNT(*) FROM ";
    value_count += "\"" TABLE_NAME "\"";
    value_count += " WHERE object = \'" + bucket.rawHexData() + "\' AND key BETWEEN ? AND ?";

    mIOService->post(
        std::tr1::bind(&SQLiteStorage::executeCount, this, value_count, start, finish, cb),
        "SQLiteStorage::executeCount"
    );
    return true;
}

void SQLiteStorage::executeCount(const String value_count, const Key& start, const Key& finish, CountCallback cb)
{
    bool success = true;
    int32 count = 0;

	int rc;
    char* remain;
    sqlite3_stmt* value_count_stmt;
    rc = sqlite3_prepare_v2(mDB->db(), value_count.c_str(), -1, &value_count_stmt, (const char**)&remain);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error preparing value count statement");

    if (rc==SQLITE_OK) {
    	rc = sqlite3_bind_text(value_count_stmt, 1, start.c_str(), (int)start.size(), SQLITE_TRANSIENT);
    	success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error binding start key to value count statement");
    	rc = sqlite3_bind_text(value_count_stmt, 2, finish.c_str(), (int)finish.size(), SQLITE_TRANSIENT);
    	success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error binding finish key to value count statement");
    	if (rc==SQLITE_OK) {
    		int step_rc = sqlite3_step(value_count_stmt);
    		count = sqlite3_column_int(value_count_stmt, 0);
    		if (step_rc != SQLITE_OK && step_rc != SQLITE_DONE && step_rc != SQLITE_ROW)
    			sqlite3_reset(value_count_stmt); // allow this to be cleaned up
    	}

    }
    rc = sqlite3_finalize(value_count_stmt);
    success = success && !SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing value delete statement");

    if (cb) {
        Result result = (success ? SUCCESS : TRANSACTION_ERROR);
        mContext->mainStrand->post(
            std::tr1::bind(cb, result, count),
            "SQLiteStorage completeCount"
        );
    }
}

} //end namespace OH
} //end namespace Sirikata
