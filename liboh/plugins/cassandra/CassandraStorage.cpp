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

void CassandraStorage::StorageAction::execute(const Bucket& bucket, Columns* columns, Keys* eraseKeys, Keys* readKeys, const String& timestamp) {
    switch(type) {
      case Read:
        {
            (*readKeys).push_back(key);
        }
        break;
      case Write:
        {
            Column col = libcassandra::createColumn(key, *value);
            (*columns).push_back(col);
        }
        break;
      case Erase:
        {
            (*eraseKeys).push_back(key);
        }
        break;
      case Error:
        SILOG(cassandra-storage, fatal, "Tried to execute an invalid StorageAction.");
        break;
    };
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

bool CassandraStorage::CassandraCommit(CassandraDBPtr db, const Bucket& bucket, Columns* columns, Keys* eraseKeys, Keys* readKeys, ReadSet* rs, const String& timestamp) {
    if((*readKeys).size()>0){
        try{
            *rs=db->db()->getColumnsValues(bucket.rawHexData(),CF_NAME, timestamp, *readKeys);  // batch read
        }
        catch(...){
            return false;
        }
    }
    if((*columns).size()>0 || (*eraseKeys).size()>0){
        try{
            batchTuple tuple=batchTuple(CF_NAME, bucket.rawHexData(), timestamp, *columns, *eraseKeys);
            db->db()->batchMutate(tuple);
        }
        catch(...){
            std::cout <<"Exception Caught when Batch Write/Erase"<<std::endl;
            return false;
        }
    }
    delete columns;
    delete eraseKeys;
    delete readKeys;
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
    // FIXME should probably throw an exception if one already exists
    getTransaction(bucket);
}

void CassandraStorage::commitTransaction(const Bucket& bucket, const CommitCallback& cb, const String& timestamp) {
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
        std::tr1::bind(&CassandraStorage::executeCommit, this, bucket, trans, cb, timestamp)
    );
}

// Executes a commit. Runs in a separate thread, so the transaction is
// passed in directly
void CassandraStorage::executeCommit(const Bucket& bucket, Transaction* trans, CommitCallback cb, const String& timestamp) {
    ReadSet* rs = new ReadSet;
    Columns* columns = new Columns;
    Keys* eraseKeys = new Keys;
    Keys* readKeys = new Keys;

    for (Transaction::iterator it = trans->begin(); it != trans->end(); it++) {
        (*it).execute(bucket, columns, eraseKeys, readKeys, timestamp);
    }

    bool success = true;
    success = CassandraCommit(mDB, bucket, columns, eraseKeys, readKeys, rs, timestamp);

    if (rs->empty() || !success) {
        delete rs;
        rs = NULL;
    }

    mContext->mainStrand->post(std::tr1::bind(&CassandraStorage::completeCommit, this, trans, cb, success, rs));
}

// Complete a commit back in the main thread, cleaning it up and dispatching
// the callback
void CassandraStorage::completeCommit(Transaction* trans, CommitCallback cb, bool success, ReadSet* rs) {

    delete trans;
    if (cb) cb(success, rs);
}

bool CassandraStorage::erase(const Bucket& bucket, const Key& key, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Erase;
    action.key = key;

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb, timestamp);

    return true;
}

bool CassandraStorage::write(const Bucket& bucket, const Key& key, const String& strToWrite, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Write;
    action.key = key;
    action.value = new String(strToWrite);

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb, timestamp);

    return true;
}

bool CassandraStorage::read(const Bucket& bucket, const Key& key, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Read;
    action.key = key;

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb, timestamp);

    return true;
}

bool CassandraStorage::rangeRead(const Bucket& bucket, const Key& start, const Key& finish, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);

    ReadSet* rs = new ReadSet;
    SliceRange range;
    range.start = start;
    range.finish = finish;
    range.count = 100000; // currently, set large enough to read all the data

    bool success = true;
    try {
    	*rs = mDB->db()->getColumnsValues(bucket.rawHexData(),CF_NAME, timestamp, range);
    }
    catch(...){
       	success = false;
    }

    mContext->mainStrand->post(std::tr1::bind(&CassandraStorage::completeCommit, this, trans, cb, success, rs));
    return true;
}

bool CassandraStorage::rangeErase(const Bucket& bucket, const Key& start, const Key& finish, const CommitCallback& cb, const String& timestamp) {
    // TODO Ranged deletion is not supported in Cassandra yet
    return true;
}

bool CassandraStorage::count(const Bucket& bucket, const Key& start, const Key& finish, const CountCallback& cb, const String& timestamp) {
	SliceRange range;
    range.start = start;
    range.finish = finish;
    range.count = 100000; // currently, set large enough to read all the data

    ColumnParent col_parent;
    col_parent.__isset.super_column=true;
    col_parent.column_family=CF_NAME;
    col_parent.super_column=timestamp;

    SlicePredicate predicate;
    predicate.__isset.slice_range=true;
    predicate.slice_range=range;

    mIOService->post(
        std::tr1::bind(&CassandraStorage::executeCount, this, bucket, col_parent, predicate, cb, timestamp)
    );

    return true;
}

void CassandraStorage::executeCount(const Bucket& bucket, ColumnParent parent, SlicePredicate predicate, CountCallback cb, const String& timestamp)
{
    bool success = true;
    int32_t count = 0;
    try{
    	count = mDB->db()->getCount(bucket.rawHexData(), parent, predicate);
    }
    catch(...) {success = false;}

    mContext->mainStrand->post(std::tr1::bind(&CassandraStorage::completeCount, this, cb, success, count));
}

void CassandraStorage::completeCount(CountCallback cb, bool success, int32_t count) {
    if (cb) cb(success, count);
}

} //end namespace OH
} //end namespace Sirikata
