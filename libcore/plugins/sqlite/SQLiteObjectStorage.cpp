/*  Sirikata SQLite Plugin
 *  SQLiteObjectStorage.cpp
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
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH package.
 */

#include <util/Platform.hpp>
#include "options/Options.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include "SQLite_Persistence.pbj.hpp"
#include "SQLiteObjectStorage.hpp"
#define OPTION_DATABASE   "db"

namespace Sirikata { namespace Persistence {

 
template <typename StorageSet> void clearValuesFromStorageSet(StorageSet &ss) {
    int len = ss.reads_size();
    for (int i=0;i<len;++i) {
        ss.mutable_reads(i).clear_data();
    }
}
template <typename StorageSet> void clearKeysFromStorageSet(StorageSet &ss) {
    int len = ss.reads_size();
    for (int i=0;i<len;++i) {
        ss.mutable_reads(i).clear_object_uuid();
        ss.mutable_reads(i).clear_field_id();
        ss.mutable_reads(i).clear_field_name();
    }
}

template <typename StorageSet,typename ReadSet> void mergeKeysFromStorageSet(StorageSet &ss, const ReadSet&other) {
    int len = other.reads_size();    
    int sslen = ss.reads_size();    
    int i;
    for (i=0;i<sslen;++i) {
        ss.add_reads();
        mergeStorageKey(ss.mutable_reads(i),other.reads(i));
    }
    for (;i<len;++i) {
        mergeStorageKey(ss.mutable_reads(i),other.reads(i));
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
                           workQueueInstance=new OptionValue("workqueue","0",OptionValueType<void*>(),"Sets the work queue to be used for disk reads to a common work queue"),NULL);
    (mOptions=OptionSet::getOptions("sqlite",epoch+handle_offset))->parse(pl);

    mDiskWorkQueue=(Task::WorkQueue*)workQueueInstance->as<void*>();
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
void SQLiteObjectStorage::applyInternal(const RoutableMessageHeader&rmh,Protocol::Minitransaction*mt, void (*destroyMinitransaction)(Protocol::Minitransaction*)){
    assert(mTransactional == true);

    mDiskWorkQueue->enqueue(new ApplyTransactionMessage(this,mt,rmh,destroyMinitransaction));
}

void SQLiteObjectStorage::applyInternal(const RoutableMessageHeader&rmh,Protocol::ReadWriteSet*mt, void (*destroyReadWriteSet)(Protocol::ReadWriteSet*)){
    assert(mTransactional == true);

    mDiskWorkQueue->enqueue(new ApplyReadWriteMessage(this,mt,rmh,destroyReadWriteSet));
}

void SQLiteObjectStorage::applyInternal(Protocol::ReadWriteSet* rws, const ResultCallback& cb, void (*destroyReadWriteSet)(Protocol::ReadWriteSet*)){
    assert(mTransactional == false);

    mDiskWorkQueue->enqueue(new ApplyReadWriteWorker(this,rws,cb,destroyReadWriteSet));
}

void SQLiteObjectStorage::applyInternal(Protocol::Minitransaction* mt, const ResultCallback& cb, void (*destroyMinitransaction)(Protocol::Minitransaction*)){
    assert(mTransactional == true);

    mDiskWorkQueue->enqueue(new ApplyTransactionWorker(this,mt,cb,destroyMinitransaction));
}
SQLiteObjectStorage::ApplyReadWriteWorker::ApplyReadWriteWorker(SQLiteObjectStorage*parent, Protocol::ReadWriteSet* rws, const ResultCallback&cb, void (*destroyRWS)(Protocol::ReadWriteSet*)){
    mDestroyReadWrite=destroyRWS;
    mParent=parent;
    this->rws=rws;
    this->cb=cb;
    mResponse=NULL;
}

Protocol::Response::ReturnStatus SQLiteObjectStorage::ApplyReadWriteWorker::processReadWrite() {
    SQLiteDBPtr db = SQLite::getSingleton().open(mParent->mDBName);
    sqlite3_busy_timeout(db->db(), mParent->mBusyTimeout);
    Error error = DatabaseLocked;
    int retries =mParent->mRetries;
    for(int tries = 0; tries < retries+1 && error != None; tries++)
        error = mParent->applyReadSet(db, *rws, *mResponse);
    if (error != None) {
        mResponse->set_return_status(convertError(error));
        return mResponse->return_status();
    }else {//FIXME do we want to abort the operation (note it's not a transaction) if we failed to read any items? I think so...
        error = DatabaseLocked;

        for(int tries = 0; tries < retries+1 && error != None; tries++)
            error = mParent->applyWriteSet(db, *rws, retries);
        mResponse->set_return_status(convertError(error));
        return mResponse->return_status();
    }
}

SQLiteObjectStorage::ApplyTransactionWorker::ApplyTransactionWorker(SQLiteObjectStorage*parent, Protocol::Minitransaction* mt, const ResultCallback&cb, void (*destroyMinitransaction)(Protocol::Minitransaction*)){
    mDestroyMinitransaction=destroyMinitransaction;
    mParent=parent;
    this->mt=mt;
    this->cb=cb;
}
Protocol::Response::ReturnStatus SQLiteObjectStorage::ApplyTransactionWorker::processTransaction() {
    Error error = None;
    SQLiteDBPtr db = SQLite::getSingleton().open(mParent->mDBName);
    sqlite3_busy_timeout(db->db(), mParent->mBusyTimeout);
    int retries=mParent->mRetries;
    int tries = retries + 1;
    while( tries > 0 ) {
        mParent->beginTransaction(db);

        error = mParent->checkCompareSet(db, *mt);

        Error read_error = None;

        read_error = mParent->applyReadSet(db, *mt, *mResponse);

        if (error==None) {
            error=read_error;
        }

        // Errors during compares or reads won't be resolved by retrying
        if (error != None) {
            mParent->rollbackTransaction(db);
            break;
        }

        error = mParent->applyWriteSet(db, *mt, 0);
        if (error != None)
            mParent->rollbackTransaction(db);
        else
            mParent->commitTransaction(db);

        tries--;
    }
    mResponse->set_return_status(convertError(error));
    return mResponse->return_status();
}

void SQLiteObjectStorage::ApplyTransactionWorker::operator() () {
    mResponse = new Protocol::Response;
    processTransaction();
    (*mDestroyMinitransaction)(mt);
    cb(mResponse);
    delete this;
}


void SQLiteObjectStorage::ApplyReadWriteWorker::operator() () {
    mResponse = new Protocol::Response;
    processReadWrite();
    (*mDestroyReadWrite)(rws);
    cb(mResponse);
    delete this;
}
SQLiteObjectStorage::ApplyTransactionMessage::ApplyTransactionMessage(SQLiteObjectStorage*parent, Protocol::Minitransaction* mt,const RoutableMessageHeader&hdr, void (*destroyMinitransaction)(Protocol::Minitransaction*)){
    mDestroyMinitransaction=destroyMinitransaction;
    mParent=parent;
    this->mt=mt;
    this->hdr=hdr;
}
void SQLiteObjectStorage::ApplyReadWriteMessage::operator() () {
    Protocol::Response response;
    mResponse = &response;
    processReadWrite();
    assert(mResponse!=NULL);
    (*mDestroyReadWrite)(rws);
    hdr.swap_source_and_destination();
    mParent->forward(hdr,response);
    mResponse=NULL;
    delete this;
}
void SQLiteObjectStorage::ApplyTransactionMessage::operator() () {
    Protocol::Response response;
    mResponse = &response;
    processTransaction();
    assert(mResponse!=NULL);
    (*mDestroyMinitransaction)(mt);
    hdr.swap_source_and_destination();
    mParent->forward(hdr,response);
    mResponse=NULL;
    delete this;
}


SQLiteObjectStorage::ApplyReadWriteMessage::ApplyReadWriteMessage(SQLiteObjectStorage*parent, Protocol::ReadWriteSet* rws,const RoutableMessageHeader&hdr, void (*destroyRWS)(Protocol::ReadWriteSet*)){
    mDestroyReadWrite=destroyRWS;
    mParent=parent;
    this->rws=rws;
    this->hdr=hdr;
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

template<class StorageKey> String SQLiteObjectStorage::getTableName(const StorageKey& key) {
    return key.object_uuid().rawHexData();
}

template <class StorageKey> String SQLiteObjectStorage::getKeyName(const StorageKey& key) {
    std::stringstream ss(key.field_name());
    ss<<'_'<<key.field_id();
    return ss.str();
}

Protocol::Response::ReturnStatus SQLiteObjectStorage::convertError(Error internal) {
    switch(internal) {
      case None:
        return Protocol::Response::SUCCESS;
        break;
      case KeyMissing:
        return Protocol::Response::KEY_MISSING;
        break;
      case ComparisonFailed:
        return Protocol::Response::COMPARISON_FAILED;
        break;
      case DatabaseLocked:
        return Protocol::Response::DATABASE_LOCKED;
        break;
      default:
        return Protocol::Response::INTERNAL_ERROR;
    }
}

template <class ReadSet> SQLiteObjectStorage::Error SQLiteObjectStorage::applyReadSet(const SQLiteDBPtr& db, const ReadSet& rs, Protocol::Response&retval) {

    int num_reads=rs.reads_size();
    retval.clear_reads();
    while (retval.reads_size()<num_reads)
        retval.add_reads();
    SQLiteObjectStorage::Error databaseError=None;
    for (int rs_it=0;rs_it<num_reads;++rs_it) {
        String table_name = getTableName(rs.reads(rs_it));
        String key_name = getKeyName(rs.reads(rs_it));

        String value_query = "SELECT value FROM ";
        value_query += "\"" + table_name + "\"";
        value_query += " WHERE key == ?";

        int rc;
        char* remain;
        sqlite3_stmt* value_query_stmt;
        bool newStep=true;
        bool locked=false;
        rc = sqlite3_prepare_v2(db->db(), value_query.c_str(), -1, &value_query_stmt, (const char**)&remain);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value query statement");
        if (rc==SQLITE_OK) {
            
            rc = sqlite3_bind_text(value_query_stmt, 1, key_name.c_str(), (int)key_name.size(), SQLITE_TRANSIENT);
            SQLite::check_sql_error(db->db(), rc, NULL, "Error binding key name to value query statement");
            if (rc==SQLITE_OK) {
                int step_rc = sqlite3_step(value_query_stmt);
                while(step_rc == SQLITE_ROW) {
                    newStep=false;
                    retval.reads(rs_it).set_data((const char*)sqlite3_column_text(value_query_stmt, 0),sqlite3_column_bytes(value_query_stmt, 0));
                    step_rc = sqlite3_step(value_query_stmt);
                }
                if (step_rc != SQLITE_DONE) {
                    // reset the statement so it'll clean up properly
                    rc = sqlite3_reset(value_query_stmt);
                    SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");
                    if (rc==SQLITE_LOCKED||rc==SQLITE_BUSY)
                        locked=true;
                }         
                
            }
        }
        rc = sqlite3_finalize(value_query_stmt);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");
        if (locked||rc == SQLITE_LOCKED||rc==SQLITE_BUSY) {
            retval.clear_reads();
            return DatabaseLocked;
        }

        if (newStep) {
            retval.reads(rs_it).clear_data();
            databaseError=KeyMissing;
        }
    }
    if (rs.has_options()&&(rs.options()&Protocol::ReadWriteSet::RETURN_READ_NAMES)!=0) {
        // make sure read set is clear before each attempt
        mergeKeysFromStorageSet( retval, rs );
    }
    return databaseError;
}

template <class WriteSet> SQLiteObjectStorage::Error SQLiteObjectStorage::applyWriteSet(const SQLiteDBPtr& db, const WriteSet& ws, int retries) {
    int num_writes=ws.writes_size();
    for (int ws_it=0;ws_it<num_writes;++ws_it) {

        String table_name = getTableName(ws.writes(ws_it));
        String key_name = getKeyName(ws.writes(ws_it));

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
        rc = sqlite3_finalize(table_create_stmt);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing table create statement");
        // Insert or replace the value

        String value_insert = "INSERT OR REPLACE INTO ";
        if (!ws.writes(ws_it).has_data()) {
             value_insert = "DELETE FROM ";
        }
        value_insert += "\"" + table_name + "\"";
        if (!ws.writes(ws_it).has_data()) {
            value_insert+= " WHERE key = ?";
        }else {
        //if (ws.writes(ws_it).has_data()) {
            value_insert += " (key, value) VALUES(?, ?)";
            //}
        }
        sqlite3_stmt* value_insert_stmt;

        rc = sqlite3_prepare_v2(db->db(), value_insert.c_str(), -1, &value_insert_stmt, (const char**)&remain);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value insert statement");

        rc = sqlite3_bind_text(value_insert_stmt, 1, key_name.c_str(), (int)key_name.size(), SQLITE_TRANSIENT);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error binding key name to value insert statement");
        if (ws.writes(ws_it).has_data()) {
            rc = sqlite3_bind_blob(value_insert_stmt, 2, ws.writes(ws_it).data().data(), (int)ws.writes(ws_it).data().size(), SQLITE_TRANSIENT);
            SQLite::check_sql_error(db->db(), rc, NULL, "Error binding value to value insert statement");
        }

        int step_rc = sqlite3_step(value_insert_stmt);
        if (step_rc != SQLITE_OK && step_rc != SQLITE_DONE)
            sqlite3_reset(value_insert_stmt); // allow this to be cleaned up

        rc = sqlite3_finalize(value_insert_stmt);
        SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value insert statement");
        if (step_rc == SQLITE_BUSY || step_rc == SQLITE_LOCKED)
            return DatabaseLocked;

    }

    return None;
}

template <class CompareSet> SQLiteObjectStorage::Error SQLiteObjectStorage::checkCompareSet(const SQLiteDBPtr& db, const CompareSet& cs) {
    int num_compares=cs.compares_size();
    for (int cs_it=0;cs_it<num_compares;++cs_it) {

        String table_name = getTableName(cs.compares(cs_it));
        String key_name = getKeyName(cs.compares(cs_it));

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
        bool passed_test = true;

        if (step_rc == SQLITE_ROW) {
            const char *data=(const char*)sqlite3_column_text(value_query_stmt, 0);
            size_t size=sqlite3_column_bytes(value_query_stmt, 0);
            if(cs.compares(cs_it).has_data()==false) {
                passed_test=false; 
            } else if (cs.compares(cs_it).has_comparator()==false) {
                if (cs.compares(cs_it).data().length() != size) passed_test = false;
                else if (0!=memcmp(cs.compares(cs_it).data().data(), data, size)) passed_test = false;
            }else {
                switch (cs.compares(cs_it).comparator()) {
                  case Protocol::CompareElement::EQUAL:
                    if (cs.compares(cs_it).data().length() != size) passed_test = false;
                    else if (0!=memcmp(cs.compares(cs_it).data().data(), data, size)) passed_test = false;
                    break;
                  case Protocol::CompareElement::NEQUAL:
                    if (cs.compares(cs_it).data().length() == size
                        && 0==memcmp(cs.compares(cs_it).data().data(), data, size)) 
                        passed_test = false;
                    break;
                  default:
                    passed_test=false;
                    break;
                }
            }
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

        if (!passed_test)
            return ComparisonFailed;

    }

    return None;
}

Persistence::Protocol::Minitransaction* SQLiteObjectStorage::createMinitransaction(int numReadKeys, int numWriteKeys, int numCompares) {
    Persistence::Protocol::Minitransaction* retval=new Persistence::Protocol::Minitransaction();
    while (numReadKeys--) {
        retval->add_reads();
    }
    while (numWriteKeys--) {
        retval->add_writes();
    }
    while (numCompares--) {
        retval->add_compares();
    }
    return retval;
}

Persistence::Protocol::ReadWriteSet* SQLiteObjectStorage::createReadWriteSet(int numReadKeys, int numWriteKeys) {
    
    Persistence::Protocol::ReadWriteSet* retval=new Persistence::Protocol::ReadWriteSet();
    while (numReadKeys--) {
        retval->add_reads();
    }
    while (numWriteKeys--) {
        retval->add_writes();
    }
    return retval;
}


void SQLiteObjectStorage::destroyResponse(Persistence::Protocol::Response*res) {
    delete res;
}

bool SQLiteObjectStorage::forwardMessagesTo(MessageService*ms) {
    mDiskWorkQueue->enqueue(new AddMessageServiceMessage(this,ms));    
    return true;
}
void SQLiteObjectStorage::AddMessageServiceMessage::operator() (){
    mParent->mInterestedParties.push_back(toAdd);
    // FIXME: delete this???
}

void SQLiteObjectStorage::RemoveMessageServiceMessage::operator()(){
    std::vector<MessageService*>::iterator where=mParent->mInterestedParties.begin();
    while (where!=mParent->mInterestedParties.end()) {        
        where=std::find(where,mParent->mInterestedParties.end(),toRemove);
        if (where!=mParent->mInterestedParties.end()) {            
            where=mParent->mInterestedParties.erase(where);
        }
    }
    *done=true;
    delete this;
}

bool SQLiteObjectStorage::endForwardingMessagesTo(MessageService*ms) {
    volatile bool complete=false;
    mDiskWorkQueue->enqueue(new RemoveMessageServiceMessage(this,ms,&complete));    
    while(!complete) {
    }
    delete this;
    return true;
}

void SQLiteObjectStorage::processMessage(const RoutableMessageHeader&hdr,MemoryReference ref) {
    if (mTransactional) {        
        Protocol::Minitransaction *trans=createMinitransaction(0,0,0);
        if (trans->ParseFromArray(ref.data(),ref.size())) {
            transactMessage(hdr,trans);
        }
    }else {
        Protocol::ReadWriteSet *rws=createReadWriteSet(0,0);
        if (rws->ParseFromArray(ref.data(),ref.size())) {
            applyMessage(hdr,rws);
        }        

    }

}

void SQLiteObjectStorage::forward (RoutableMessageHeader&hdr, Protocol::Response&resp) {
    String databuf;
    resp.SerializeToString(&databuf);
    MemoryReference membuf(databuf);
    for (std::vector<MessageService*>::iterator i=mInterestedParties.begin(),ie=mInterestedParties.end();
         i!=ie;
         ++i) {
        (*i)->processMessage(hdr,membuf);
    }
}


} }// namespace Sirikata::Persistence
