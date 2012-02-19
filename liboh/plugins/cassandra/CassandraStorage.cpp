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
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>

#define CF_NAME "persistence"  // Column Family Name

/** Implementation Notes
 *  ====================
 *
 *  Data Layout
 *  -----------
 *  See http://wiki.apache.org/cassandra/DataModel for an explanation of the
 *  Cassandra data
 *  model. http://arin.me/blog/wtf-is-a-supercolumn-cassandra-data-model may be
 *  helpful too. In short, Cassandra is a multidimensional map (each ... means a
 *  set of these elements are maintained):
 *
 *   Keyspace {
 *    Super Column Family {
 *     Row {
 *      Super Column {
 *       Column (each column is key -> value pair w/ timestamp)
 *       ...
 *       Column
 *      } // Super Column
 *      ...
 *      Super Column {}
 *     } // Row
 *     ...
 *     Row {}
 *    } // Super Column Family
 *    ...
 *    Super Column Family {}
 *  } // Keyspace
 *  ...
 *  Keyspace {}
 *
 *  We map to this model as follows:
 *   * A keyspace is shared with other Sirikata storage - 'sirikata' by default.
 *   * A Super Column Family isolates Cassandra Storage - 'persistence' by
 *     default.
 *   * Buckets (i.e. HostedObjects) are each a Row in the persistence Super
 *     Column Family
 *   * Timestamps are each a Super Column.
 *   * Individual key -> value pairs are each a Column within the timestamps
 *     Super Column.
 *  This means you can think of Cassandra as one giant, 5-dimensional map and we
 *  use it as:
 *   Cassandra['sirikata']['persistence'][bucket][timestamp][key] = value;
 *   |--------fixed by settings---------|
 *   |-----------get per object storage---------|
 *   |-------get active timestamp storage for object-------|
 *
 *  You can see this structure when you use cassandra-cli to explore the data:
 *   > cassandra-cli
 *   >> connect localhost/9160;
 *   >> use sirikata; # Use 'sirikata' keyspace.
 *   # Change a few settings that force display to strings, which assumes you're
 *   # using utf8 keys
 *   >> assume persistence keys as utf8; # Bucket (row) names
 *   >> assume persistence comparator as utf8; # Timestamp (super column) names
 *   >> assume persistence sub_comparator as utf8; # Key (column) names
 *   # List 2 rows from persistence
 *   >> list persistence limit 2;
 *    -------------------
 *    RowKey: b10c1c63686e6b050000020002000001
 *    => (super_column=current,
 *         (column=foo, value=120608c0af271005120608808, timestamp=1329197745156495)
 *         (column=bar, value=c0af271005120608808, timestamp=1329197738171366))
 *    -------------------
 *    RowKey: b10c1c63686e6b050000020002000001
 *    => (super_column=current,
 *         (column=foo, value=120608c0af271005120608808, timestamp=1329197745156495)
 *        (column=bar, value=c0af271005120608808, timestamp=1329197738171366))
 *
 */

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
    keyEnd = rhs.keyEnd;
    if (rhs.value != NULL)
        value = new String(*(rhs.value));
    else
        value = NULL;

    return *this;
}

void CassandraStorage::StorageAction::execute(const Bucket& bucket, Columns* columns, Keys* eraseKeys, Keys* readKeys, SliceRanges* readRanges, ReadSet* compares, SliceRanges* eraseRanges, const String& timestamp) {
    switch(type) {
      case Read:
        {
            (*readKeys).push_back(key);
        }
        break;
      case ReadRange:
        {
            SliceRange range;
            range.start = key;
            range.finish = keyEnd;
            range.count = 100000; // currently, set large enough to read all the data
            readRanges->push_back(range);
        }
        break;
      case Compare:
          {
              (*readKeys).push_back(key);
              (*compares)[key] = *value;
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
      case EraseRange:
        {
            SliceRange range;
            range.start = key;
            range.finish = keyEnd;
            range.count = 100000; // currently, set large enough to read all the data
            eraseRanges->push_back(range);
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

    mIOService = new Network::IOService("CassandraStorage IO Thread");
    mWork = new Network::IOWork(*mIOService, "CassandraStorage IO Thread");
    mThread = new Sirikata::Thread(std::tr1::bind(&Network::IOService::runNoReturn, mIOService));

    mIOService->post(std::tr1::bind(&CassandraStorage::initDB, this), "CassandraStorage::initDB");
}

void CassandraStorage::initDB() {
    CassandraDBPtr db = Cassandra::getSingleton().open(mDBHost, mDBPort);
    mDB = db;
}

Storage::Result CassandraStorage::CassandraCommit(CassandraDBPtr db, const Bucket& bucket, Columns* columns, Keys* eraseKeys, Keys* readKeys, SliceRanges* readRanges, ReadSet* compares, SliceRanges* eraseRanges, ReadSet* rs, const String& timestamp) {
    // FIXME *THIS ISN'T EVEN TRYING TO BE ATOMIC*

    Result result = SUCCESS;

    if((*readKeys).size()>0) {
        try{
            *rs=db->db()->getColumnsValues(bucket.rawHexData(),CF_NAME, timestamp, *readKeys);  // batch read
        }
        catch(...){
            result = TRANSACTION_ERROR;
        }
    }
    // Compare keys come out with the read set. We compare values and remove
    // them from the results
    for(ReadSet::iterator it = compares->begin(); result == SUCCESS && it != compares->end(); it++) {
        ReadSet::iterator read_it = rs->find(it->first);
        if (read_it == rs->end() ||
            read_it->second != it->second) {
            result = TRANSACTION_ERROR;
            break;
        }
        rs->erase(read_it);
    }

    for(uint32 i = 0; result == SUCCESS && i < readRanges->size(); i++) {
        try{
            ReadSet rangeData = db->db()->getColumnsValues(bucket.rawHexData(),CF_NAME, timestamp, (*readRanges)[i]);
            for(ReadSet::iterator it = rangeData.begin(); it != rangeData.end(); it++)
                (*rs)[it->first] = it->second;
            if (rangeData.size() == 0)
                result = TRANSACTION_ERROR;
        }
        catch(...){
            result = TRANSACTION_ERROR;
        }
    }

    // Erase ranges are processed before erases -- they just look up the keys to
    // erase and add them to the erase set
    for(uint32 i = 0; result == SUCCESS && i < eraseRanges->size(); i++) {
        try {
            ReadSet rangeData = mDB->db()->getColumnsValues(bucket.rawHexData(), CF_NAME, timestamp, (*eraseRanges)[i]);
            for(ReadSet::iterator it = rangeData.begin(); it != rangeData.end(); it++)
                eraseKeys->push_back(it->first);
        }
        catch(...){
            result = TRANSACTION_ERROR;
        }
    }

    if(result == SUCCESS && (((*columns).size()>0) || ((*eraseKeys).size()>0))){
        try{
            batchTuple tuple=batchTuple(CF_NAME, bucket.rawHexData(), timestamp, *columns, *eraseKeys);
            db->db()->batchMutate(tuple);
        }
        catch(...) {
            SILOG(cassandra-storage, fatal, "Exception Caught when Batch Write/Erase");
            result = TRANSACTION_ERROR;
        }
    }

    delete columns;
    delete eraseKeys;
    delete readKeys;
    delete readRanges;
    delete eraseRanges;
    return result;
}

void CassandraStorage::stop() {
    // Just kill the work that keeps the IO thread alive and wait for thread to
    // finish, i.e. for outstanding transactions to complete
    delete mWork;
    mWork = NULL;
    mThread->join();
    delete mThread;
    mThread = NULL;
    delete mIOService;
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
        completeCommit(trans, cb, SUCCESS, rs);
        return;
    }

    mIOService->post(
        std::tr1::bind(&CassandraStorage::executeCommit, this, bucket, trans, cb, timestamp),
        "CassandraStorage::executeCommit"
    );
}

// Executes a commit. Runs in a separate thread, so the transaction is
// passed in directly
void CassandraStorage::executeCommit(const Bucket& bucket, Transaction* trans, CommitCallback cb, const String& timestamp) {
    ReadSet* rs = new ReadSet;
    Columns* columns = new Columns;
    Keys* eraseKeys = new Keys;
    Keys* readKeys = new Keys;
    SliceRanges* readRanges = new SliceRanges;
    SliceRanges* eraseRanges = new SliceRanges;
    ReadSet* compares = new ReadSet;

    for (Transaction::iterator it = trans->begin(); it != trans->end(); it++) {
        (*it).execute(bucket, columns, eraseKeys, readKeys, readRanges, compares, eraseRanges, timestamp);
    }

    Result result = SUCCESS;
    result = CassandraCommit(mDB, bucket, columns, eraseKeys, readKeys, readRanges, compares, eraseRanges, rs, timestamp);

    if (rs->empty() || (result != SUCCESS)) {
        delete rs;
        rs = NULL;
    }

    mContext->mainStrand->post(
        std::tr1::bind(&CassandraStorage::completeCommit, this, trans, cb, result, rs),
        "CassandraStorage::completeCommit"
    );
}

// Complete a commit back in the main thread, cleaning it up and dispatching
// the callback
void CassandraStorage::completeCommit(Transaction* trans, CommitCallback cb, Result success, ReadSet* rs) {
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

bool CassandraStorage::compare(const Bucket& bucket, const Key& key, const String& value, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::Compare;
    action.key = key;
    action.value = new String(value);

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb, timestamp);

    return true;
}

bool CassandraStorage::rangeRead(const Bucket& bucket, const Key& start, const Key& finish, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::ReadRange;
    action.key = start;
    action.keyEnd = finish;

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb, timestamp);

    return true;
}

bool CassandraStorage::rangeErase(const Bucket& bucket, const Key& start, const Key& finish, const CommitCallback& cb, const String& timestamp) {
    bool is_new = false;
    Transaction* trans = getTransaction(bucket, &is_new);
    trans->push_back(StorageAction());
    StorageAction& action = trans->back();
    action.type = StorageAction::EraseRange;
    action.key = start;
    action.keyEnd = finish;

    // Run commit if this is a one-off transaction
    if (is_new)
        commitTransaction(bucket, cb, timestamp);

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
        std::tr1::bind(&CassandraStorage::executeCount, this, bucket, col_parent, predicate, cb, timestamp),
        "CassandraStorage::executeCount"
    );

    return true;
}

void CassandraStorage::executeCount(const Bucket& bucket, ColumnParent& parent, SlicePredicate& predicate, CountCallback cb, const String& timestamp)
{
    Result result = SUCCESS;
    int32 count = 0;
    try{
    	count = mDB->db()->getCount(bucket.rawHexData(), parent, predicate);
    }
    catch(...) {result = TRANSACTION_ERROR;}

    mContext->mainStrand->post(
        std::tr1::bind(&CassandraStorage::completeCount, this, cb, result, count),
        "CassandraStorage::completeCount"
    );
}

void CassandraStorage::completeCount(CountCallback cb, Result result, int32 count) {
    if (cb) cb(result, count);
}

} //end namespace OH
} //end namespace Sirikata
