#include <util/Platform.hpp>
#include "SQLiteObjectStorage.hpp"
#include "options/Options.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#define OPTION_DATABASE   "db"

namespace Sirikata {

 
static void clearValuesFromStorageSet(StorageSet& ss) {
    for(StorageSet::iterator ss_it = ss.begin(); ss_it != ss.end(); ss_it++) {
        if (ss_it->second != NULL) {
            delete ss_it->second;
            ss_it->second = NULL;
        }
    }
}

SQLiteObjectStorage*SQLiteObjectStorage::create(bool t, const String&s){
    return new SQLiteObjectStorage(t,s);
}

ReadWriteHandler* SQLiteObjectStorage::createReadWriteHandler(const String& pl) {
    return new SQLiteObjectStorage(false, pl);
}

MinitransactionHandler* SQLiteObjectStorage::createMinitransactionHandler(const String& pl) {
    return new SQLiteObjectStorage(true, pl);
}
namespace {
Any pointerparser(std::string s) {
    Task::WorkQueue*retval=NULL;
    size_t temp=0;
    std::stringstream ss(s);
    ss>>temp;
    retval=(Task::WorkQueue*)temp;
    Any anyret(retval);
    Task::WorkQueue * test=anyret.as<Task::WorkQueue*>();
    return anyret;
}
}
SQLiteObjectStorage::SQLiteObjectStorage(bool transactional, const String& pl)
 : mTransactional(transactional),
   mDBName(),
   mRetries(5),
   mBusyTimeout(1000)
{
    OptionValue*databaseFile;
    OptionValue*workQueueInstance;
    unsigned char * epoch=NULL;
    static AtomicValue<int> counter(0);
    int handle_offset=counter++;
    InitializeClassOptions("sqlite",epoch+handle_offset,
                           databaseFile=new OptionValue("databasefile","",OptionValueType<String>(),"Sets the database to be used for storage"),
                           workQueueInstance=new OptionValue("workqueue","0","Sets the work queue to be used for disk reads to a common work queue",std::tr1::function<Any(std::string)>(&pointerparser)),NULL);
    (mOptions=OptionSet::getOptions("sqlite",epoch+handle_offset))->parse(pl);

    mDiskWorkQueue=workQueueInstance->as<Task::WorkQueue*>();
    mWorkQueueThread=NULL;
    if(mDiskWorkQueue==NULL) {
        mDiskWorkQueue=&_mLocalWorkQueue;
        mWorkQueueThread=mDiskWorkQueue->createWorkerThreads(1);        
    }

    mDBName = databaseFile->as<String>();
    assert( !mDBName.empty() );

}

SQLiteObjectStorage::~SQLiteObjectStorage() {
    if(mWorkQueueThread) {
        _mLocalWorkQueue.destroyWorkerThreads(mWorkQueueThread);
    }
}

void SQLiteObjectStorage::apply(ReadWriteSet* rws, const ResultCallback& cb) {
    assert(mTransactional == false);

    mDiskWorkQueue->enqueue(new ApplyReadWriteWorker(this,rws,cb));
}

void SQLiteObjectStorage::apply(Minitransaction* mt, const ResultCallback& cb) {
    assert(mTransactional == true);
    mDiskWorkQueue->enqueue(new ApplyTransactionWorker(this,mt,cb));
}
SQLiteObjectStorage::ApplyReadWriteWorker::ApplyReadWriteWorker(SQLiteObjectStorage*parent, ReadWriteSet* rws, const ResultCallback&cb){
    mParent=parent;
    this->rws=rws;
    this->cb=cb;
}

void SQLiteObjectStorage::ApplyReadWriteWorker::operator()() {
    SQLiteDBPtr db = SQLite::getSingleton().open(mParent->mDBName);

    sqlite3_busy_timeout(db->db(), mParent->mBusyTimeout);

    Error error = DatabaseLocked;
    int retries =mParent->mRetries;
    for(int tries = 0; tries < retries+1 && error != None; tries++)
        error = mParent->applyReadSet(db, rws->reads());
    if (error != None) {
        cb(convertError(error));
    }else {//FIXME do we want to abort the operation (note it's not a transaction) if we failed to read any items? I think so...
        error = DatabaseLocked;
        for(int tries = 0; tries < retries+1 && error != None; tries++)
            error = mParent->applyWriteSet(db, rws->writes(), retries);
        cb(convertError(error));
    }
}

SQLiteObjectStorage::ApplyTransactionWorker::ApplyTransactionWorker(SQLiteObjectStorage*parent, Minitransaction* mt, const ResultCallback&cb){
    mParent=parent;
    this->mt=mt;
    this->cb=cb;
}
void SQLiteObjectStorage::ApplyTransactionWorker::operator()() {
    Error error = None;
    SQLiteDBPtr db = SQLite::getSingleton().open(mParent->mDBName);

    sqlite3_busy_timeout(db->db(), mParent->mBusyTimeout);
    int retries=mParent->mRetries;
    int tries = retries + 1;
    while( tries > 0 ) {
        mParent->beginTransaction(db);

        error = mParent->checkCompareSet(db, mt->compares());
        // Errors during compares won't be resolved by retrying
        if (error != None) {
            mParent->rollbackTransaction(db);
            break;
        }

        // make sure read set is clear before each attempt
        clearValuesFromStorageSet( mt->reads() );
        error = mParent->applyReadSet(db, mt->reads());
        // Errors during reads won't be resolved by retrying
        if (error != None) {
            mParent->rollbackTransaction(db);
            break;
        }

        error = mParent->applyWriteSet(db, mt->writes(), 0);
        if (error != None)
            mParent->rollbackTransaction(db);
        else
            mParent->commitTransaction(db);

        tries--;
    }
    cb(convertError(error));
}
/*
void SQLiteObjectStorage::handleResult(const EventPtr& evt) const {
    SQLiteObjectStorageResultEventPtr revt = DowncastEvent<SQLiteObjectStorageResultEvent>(evt);

    revt->callback()(revt->error());
}
*/
void SQLiteObjectStorage::beginTransaction(const SQLiteDBPtr& db) {
    String begin = "BEGIN DEFERRED TRANSACTION";

    int rc;
    char* remain;
    sqlite3_stmt* begin_stmt;

    rc = sqlite3_prepare_v2(db->db(), begin.c_str(), -1, &begin_stmt, (const char**)&remain);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing begin transaction statement");

    rc = sqlite3_step(begin_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error executing begin statement");
    rc = sqlite3_finalize(begin_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing begin statement");
}

void SQLiteObjectStorage::rollbackTransaction(const SQLiteDBPtr& db) {
    String rollback = "ROLLBACK TRANSACTION";

    int rc;
    char* remain;
    sqlite3_stmt* rollback_stmt;

    rc = sqlite3_prepare_v2(db->db(), rollback.c_str(), -1, &rollback_stmt, (const char**)&remain);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing rollback transaction statement");

    rc = sqlite3_step(rollback_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error executing rollback statement");
    rc = sqlite3_finalize(rollback_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing rollback statement");
}

bool SQLiteObjectStorage::commitTransaction(const SQLiteDBPtr& db) {
    String commit = "COMMIT TRANSACTION";

    int rc;
    char* remain;
    sqlite3_stmt* commit_stmt;

    rc = sqlite3_prepare_v2(db->db(), commit.c_str(), -1, &commit_stmt, (const char**)&remain);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing commit transaction statement");

    rc = sqlite3_step(commit_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error executing commit statement");
    rc = sqlite3_finalize(commit_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing commit statement");

    return true;
}

String SQLiteObjectStorage::getTableName(const StorageKey& key) {
    UUID keyVWObj = keyVWObject(key);

    return keyVWObj.rawHexData();
}

String SQLiteObjectStorage::getKeyName(const StorageKey& key) {
    uint32 keyObj = keyObject(key);

    return boost::lexical_cast<String>(keyObj) + "_" + keyField(key);
}

ObjectStorageError SQLiteObjectStorage::convertError(Error internal) {
    switch(internal) {
      case None:
        return ObjectStorageError(ObjectStorageErrorType_None);
        break;
      case KeyMissing:
        return ObjectStorageError(ObjectStorageErrorType_KeyMissing);
        break;
      case ComparisonFailed:
        return ObjectStorageError(ObjectStorageErrorType_ComparisonFailed);
        break;
      case DatabaseLocked:
        return ObjectStorageError(ObjectStorageErrorType_Internal);
        break;
    }
    // if we don't even recognize the error, just mark it as internal
    return ObjectStorageError(ObjectStorageErrorType_Internal);
}

SQLiteObjectStorage::Error SQLiteObjectStorage::applyReadSet(const SQLiteDBPtr& db, ReadSet& rs) {
    ReadSet::iterator rs_it = rs.begin();
    while(rs_it != rs.end()) {
        const StorageKey& key = rs_it->first;

        String table_name = getTableName(key);
        String key_name = getKeyName(key);

        String value_query = "SELECT value FROM ";
        value_query += "\"" + table_name + "\"";
        value_query += " WHERE key == ?";

        int rc;
        char* remain;
        sqlite3_stmt* value_query_stmt;

        rc = sqlite3_prepare_v2(db->db(), value_query.c_str(), -1, &value_query_stmt, (const char**)&remain);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value query statement");

        rc = sqlite3_bind_text(value_query_stmt, 1, key_name.c_str(), (int)key_name.size(), SQLITE_TRANSIENT);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error binding key name to value query statement");

        int step_rc = sqlite3_step(value_query_stmt);
        while(step_rc == SQLITE_ROW) {
            assert(rs_it->second == NULL);
            rs_it->second = new StorageValue((const char*)sqlite3_column_text(value_query_stmt, 0), sqlite3_column_bytes(value_query_stmt, 0));

            step_rc = sqlite3_step(value_query_stmt);
        }
        if (step_rc != SQLITE_DONE) {
            // reset the statement so it'll clean up properly
            sqlite3_reset(value_query_stmt);
        }
        rc = sqlite3_finalize(value_query_stmt);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");
        if (step_rc != SQLITE_DONE) {
            clearValuesFromStorageSet(rs);
            return DatabaseLocked;
        }

        if (rs_it->second == NULL) {
            clearValuesFromStorageSet(rs);
            return KeyMissing;
        }

        rs_it++;
    }

    return None;
}

SQLiteObjectStorage::Error SQLiteObjectStorage::applyWriteSet(const SQLiteDBPtr& db, WriteSet& ws, int retries) {
    WriteSet::iterator ws_it = ws.begin();
    while(ws_it != ws.end()) {
        const StorageKey& key = ws_it->first;

        String table_name = getTableName(key);
        String key_name = getKeyName(key);

        // Create the table for this object if it doesn't exist yet
        String table_create = "CREATE TABLE IF NOT EXISTS ";
        table_create += "\"" + table_name + "\"";
        table_create += "(key TEXT UNIQUE, value TEXT)";

        int rc;
        char* remain;
        sqlite3_stmt* table_create_stmt;

        rc = sqlite3_prepare_v2(db->db(), table_create.c_str(), -1, &table_create_stmt, (const char**)&remain);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing table create statement");

        rc = sqlite3_step(table_create_stmt);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error executing table create statement");


        // Insert or replace the value
        String value_insert = "INSERT OR REPLACE INTO ";
        value_insert += "\"" + table_name + "\"";
        value_insert += " (key, value) VALUES(?, ?)";

        sqlite3_stmt* value_insert_stmt;

        rc = sqlite3_prepare_v2(db->db(), value_insert.c_str(), -1, &value_insert_stmt, (const char**)&remain);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value insert statement");

        rc = sqlite3_bind_text(value_insert_stmt, 1, key_name.c_str(), (int)key_name.size(), SQLITE_TRANSIENT);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error binding key name to value insert statement");

        rc = sqlite3_bind_blob(value_insert_stmt, 2, &(*(ws_it->second))[0], (int)ws_it->second->size(), SQLITE_TRANSIENT);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error binding value to value insert statement");

        int step_rc = sqlite3_step(value_insert_stmt);
        if (step_rc != SQLITE_OK && step_rc != SQLITE_DONE)
            sqlite3_reset(value_insert_stmt); // allow this to be cleaned up

        rc = sqlite3_finalize(value_insert_stmt);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value insert statement");

        if (step_rc == SQLITE_BUSY || step_rc == SQLITE_LOCKED)
            return DatabaseLocked;

        ws_it++;
    }

    return None;
}

SQLiteObjectStorage::Error SQLiteObjectStorage::checkCompareSet(const SQLiteDBPtr& db, CompareSet& cs) {
    CompareSet::iterator cs_it = cs.begin();
    while(cs_it != cs.end()) {
        const StorageKey& key = cs_it->first;
        StorageValue* value = cs_it->second;

        String table_name = getTableName(key);
        String key_name = getKeyName(key);

        String value_query = "SELECT value FROM ";
        value_query += "\"" + table_name + "\"";
        value_query += " WHERE key == ?";

        int rc;
        char* remain;
        sqlite3_stmt* value_query_stmt;

        rc = sqlite3_prepare_v2(db->db(), value_query.c_str(), -1, &value_query_stmt, (const char**)&remain);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value query statement");

        rc = sqlite3_bind_text(value_query_stmt, 1, key_name.c_str(), (int)key_name.size(), SQLITE_TRANSIENT);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error binding key name to value query statement");

        int step_rc = sqlite3_step(value_query_stmt);
        bool equal = true;

        if (step_rc == SQLITE_ROW) {
            StorageValue stored_value((const char*)sqlite3_column_text(value_query_stmt, 0), sqlite3_column_bytes(value_query_stmt, 0));
            if (*value != stored_value) equal = false;
        }
        else {
            sqlite3_reset(value_query_stmt); // allow proper clean up
        }

        rc = sqlite3_finalize(value_query_stmt);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");

        if (step_rc == SQLITE_BUSY || step_rc == SQLITE_LOCKED)
            return DatabaseLocked;

        if (step_rc == SQLITE_DONE)
            return KeyMissing;

        if (!equal)
            return ComparisonFailed;

        cs_it++;
    }

    return None;
}

} // namespace Meru
