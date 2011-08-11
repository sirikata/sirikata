/*  Sirikata
 *  CassandraStorage.cpp
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

#include "CassandraStorage.hpp"
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>

#define CF_NAME "persistence"  // Column Family Name

namespace Sirikata {
namespace OH {

CassandraStorage::StorageAction::StorageAction()
 : type(Error),
   key(),
   value(NULL)
{
}

CassandraStorage::StorageAction::StorageAction(const StorageAction& rhs) {
    *this = rhs;
}

CassandraStorage::StorageAction::~StorageAction() {
    if (value != NULL)
        delete value;
}

CassandraStorage::StorageAction& CassandraStorage::StorageAction::operator=(const StorageAction& rhs) {
    type = rhs.type;
    key = rhs.key;
    if (rhs.value != NULL)
        value = new String(*(rhs.value));
    else
        value = NULL;

    return *this;
}

bool CassandraStorage::StorageAction::execute(CassandraDBPtr db, const Bucket& bucket, ColumnTuples& ColTuples, Keys& keys, ReadSet* rs) {
    bool success = true;
    switch(type) {
      case Read:
          {
              try{
                  //(*rs)[key]=db->db()->getColumnValue(bucket.rawHexData(), "Persistence", key);
                  keys.push_back(key);  // push the key into mKeys for batch read
              }
              catch (...){
                  //std::cout <<"Read Exception Caught"<<std::endl;
                  success = false;
              }
          }
        break;
      case Write:
          {
              try{
                  ColumnTuple tuple(CF_NAME, bucket.rawHexData(), key, *value, false);
                  ColTuples.push_back(tuple); // push the tuple into mColumnTuples for batch write
                  //db->db()->insertColumn(bucket.rawHexData(), "Persistence", key, value->c_str());
              }
              catch (...){
                  //std::cout <<"Write Exception Caught"<<std::endl;
                  success = false;
              }
          }
        break;
      case Erase:
          {
              try{
                  ColumnTuple tuple(CF_NAME, bucket.rawHexData(), key, "", true);
                  ColTuples.push_back(tuple);  // push the tuple into mColumnTuples for batch erase
                  //db->db()->remove(bucket.rawHexData(), "Persistence", "", key);
              }
              catch (...){
                  //std::cout <<"Erase Exception Caught"<<std::endl;
                  success = false;
              }
          }
        break;
      case Error:
        SILOG(cassandra-storage, fatal, "Tried to execute an invalid StorageAction.");
        break;
    };
    return success;
}

CassandraStorage::CassandraStorage(ObjectHostContext* ctx, const String& host, int port)
 : mContext(ctx),
   mDBHost(host),
   mDBPort(port),
   mDB()
{
}

CassandraStorage::~CassandraStorage()
{
}

void CassandraStorage::start() {

    mIOService = Network::IOServiceFactory::makeIOService();
    mWork = new Network::IOWork(*mIOService, "CassandraStorage IO Thread");
    mThread = new Sirikata::Thread(std::tr1::bind(&Network::IOService::runNoReturn, mIOService));

    mIOService->post(std::tr1::bind(&CassandraStorage::initDB, this));
}

void CassandraStorage::initDB() {
    CassandraDBPtr db = Cassandra::getSingleton().open(mDBHost, mDBPort);
    mDB = db;

}

bool CassandraStorage::CassandraBeginTransaction() {
    if (mColumnTuples.size()!=0|| mKeys.size()!=0){
        std::cout<<"Transaction List is not empty"<<std::endl;
        return false;
    }
    else
        return true;
}

bool CassandraStorage::CassandraCommit(CassandraDBPtr db, const Bucket& bucket, ReadSet* rs) {
    if(mKeys.size()>0){
        try{
            *rs=db->db()->getColumnsValues(bucket.rawHexData(),CF_NAME, mKeys);  // batch read
        }
        catch(...){
            //std::cout <<"Exception Caught when Batch Read"<<std::endl;
            return false;
        }
    }
    if(mColumnTuples.size()>0){
        try{
            db->db()->batchMutate(mColumnTuples);  // batch write/erase
        }
        catch(...){
            std::cout <<"Exception Caught when Batch Write/Erase"<<std::endl;
            return false;
        }
    }
    mKeys.clear();
    mColumnTuples.clear();
    return true;
}

void CassandraStorage::stop() {
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

CassandraStorage::Transaction* CassandraStorage::getTransaction(const Bucket& bucket, bool* is_new) {
    if (mTransactions.find(bucket) == mTransactions.end()) {
        if (is_new != NULL) *is_new = true;
        mTransactions[bucket] = new Transaction();
    }

    return mTransactions[bucket];
}

void CassandraStorage::beginTransaction(const Bucket& bucket) {
    getTransaction(bucket);
    mKeys.clear();
    mColumnTuples.clear();
}

void CassandraStorage::commitTransaction(const Bucket& bucket, const CommitCallback& cb){
    Transaction* trans = getTransaction(bucket);

    //can remove from mTransactions
    mTransactions.erase(bucket);

    // Short cut for empty transactions. Or maybe these should cause exceptions?
    if(trans->empty()) {
        ReadSet* rs = NULL;
        completeCommit(trans, cb, false, rs);
        return;
    }

    mIOService->post(
        std::tr1::bind(&CassandraStorage::executeCommit, this, bucket, trans, cb)
    );
}

// Executes a commit. Runs in a separate thread, so the transaction is
// passed in directly
void CassandraStorage::executeCommit(const Bucket& bucket, Transaction* trans, CommitCallback cb) {
    ReadSet* rs = new ReadSet;

    bool success = true;
    success = CassandraBeginTransaction();
    for (Transaction::iterator it = trans->begin(); success && it != trans->end(); it++) {
        success = success && (*it).execute(mDB, bucket, mColumnTuples, mKeys, rs);
        if (!success) {
            break;
        }
    }

    if (success) {
        success = CassandraCommit(mDB, bucket, rs);
    }

    if (rs->empty() || !success) {
        delete rs;
        rs = NULL;
        mKeys.clear();
        mColumnTuples.clear();
    }

    mContext->mainStrand->post(std::tr1::bind(&CassandraStorage::completeCommit, this, trans, cb, success, rs));
}

// Complete a commit back in the main thread, cleaning it up and dispatching
// the callback
void CassandraStorage::completeCommit(Transaction* trans, CommitCallback cb, bool success, ReadSet* rs) {

    delete trans;
    if (cb) cb(success, rs);
}

bool CassandraStorage::erase(const Bucket& bucket, const Key& key, const CommitCallback& cb) {
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

bool CassandraStorage::write(const Bucket& bucket, const Key& key, const String& strToWrite, const CommitCallback& cb) {
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

bool CassandraStorage::read(const Bucket& bucket, const Key& key, const CommitCallback& cb) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Read;
    action.key = key;

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb);

    return true;
}

} //end namespace OH
} //end namespace Sirikata
