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

// Column Family Names
#define CF_NAME "persistence"
#define LEASES_CF_NAME "persistence_leases"

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
 *  You should be aware that libcassandra, for some reason, doesn't use the
 *  natural order, and sometimes changes the order. For example, in this class,
 *  we're already bound to keyspace 'sirikata', and a call for a batch read
 *  looks like:
 *
 *      *rs = db->db()->getColumnsValues(bucket.rawHexData(), CF_NAME,          timestamp,          *readKeys);
 *                                   //   (row key)          (s-column family) (super column)      (columns)
 *  Note how the row inexplicably comes first. In other cases, e.g. batchTuples
 *  for batch operations, we have:
 *       batchTuple tuple=batchTuple(CF_NAME, bucket.rawHexData(), timestamp, *readColumns, *eraseColumns);
 *  which follows the sensible order (we have two final sets of columns, for
 *  read and erase, but the order of everything makes sense). Since all the keys
 *  are strings, you have to be careful to get the order correct.
 *
 *  Locking
 *  -----------
 *  We need to leasing mechanism, but Cassandra doesn't give us a lot to work
 *  with. We se a wait-chaining algorithm:
 *
 *  http://media.fightmymonster.com/Shared/docs/Wait%20Chain%20Algorithm.pdf
 *
 *  The basic idea is to use TTL keys to register interest, then look for others
 *  interested and order priority based on who observes other's registrations.
 *
 *  Since we just do leasing, we don't need any real transaction support in
 *  Cassandra. We keep locks in a separate row (i.e. each bucket has a row
 *  bucket and a row bucket-lease) to avoid naming collisions.
 *
 *  Note that locks are *on the entire bucket* and the location of locks
 *  reflects that:
 *   Cassandra['sirikata']['persistence'][bucket_lease][key] = value;
 *  We don't have a [timestamp] super column, we only have rows and
 *  columns. We don't use these for leases since there is no indication from the
 *  system when an object is removed *which* timestamps are disappearing with
 *  it, so we would have to scan through all super columns looking for locks to
 *  release. Each key in this case maps to a single client that wants to grab
 *  a lease.
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

CassandraStorage::CassandraStorage(ObjectHostContext* ctx, const String& host, int port, const Duration& lease_duration)
 : mContext(ctx),
   mDBHost(host),
   mDBPort(port),
   mDB(),
   mIOService(NULL),
   mWork(NULL),
   mThread(NULL),
   // TODO(ewencp) do something better than just a random ID
   mClientID(UUID::random().rawHexData()),
   mLeaseDuration(lease_duration),
   mRenewTimer()
{
}

CassandraStorage::~CassandraStorage()
{
}

void CassandraStorage::start() {
    mIOService = new Network::IOService("CassandraStorage IO Thread");
    mWork = new Network::IOWork(*mIOService, "CassandraStorage IO Thread");
    mThread = new Sirikata::Thread("CassandraStorage IO", std::tr1::bind(&Network::IOService::runNoReturn, mIOService));

    mIOService->post(std::tr1::bind(&CassandraStorage::initDB, this), "CassandraStorage::initDB");

    mRenewTimer = Network::IOTimer::create(
        mIOService,
        std::tr1::bind(&CassandraStorage::processRenewals, this)
    );
}

void CassandraStorage::initDB() {
    CassandraDBPtr db = Cassandra::getSingleton().open(mDBHost, mDBPort);
    mDB = db;

    mDB->createColumnFamily("persistence", "Super");
    mDB->createColumnFamily("persistence_leases", "Standard");
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
    // finish, i.e. for outstanding transactions to complete. Stop the renewal
    // timer immediately to avoid having to wait for it to fire again (possibly
    // locking things up until it does).
    mRenewTimer->cancel();

    // Just kill the work that keeps the IO thread alive and wait for thread to
    // finish, i.e. for outstanding transactions to complete
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

CassandraStorage::Transaction* CassandraStorage::getTransaction(const Bucket& bucket, bool* is_new) {
    if (mTransactions.find(bucket) == mTransactions.end()) {
        if (is_new != NULL) *is_new = true;
        mTransactions[bucket] = new Transaction();
    }

    return mTransactions[bucket];
}


void CassandraStorage::leaseBucket(const Bucket& bucket) {
    // We won't do anything up front, instead trying to acquire the lease with
    // the first transaction.
}

void CassandraStorage::releaseBucket(const Bucket& bucket) {
    // Have the storage thread release the lease
    SILOG(cassandra-storage, detailed, "Received releaseBucket call, dispatching handler to remove lease request.");

    // It's possible to get these calls on final cleanup (after stop() was
    // called) so we need to make sure we can still safely do this
    if (mIOService == NULL) return;

    mIOService->post(
        std::tr1::bind(&CassandraStorage::releaseLease, this, bucket),
        "SQLiteStorage::releaseLease"
    );
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
    // Before anything else make sure we've got the lease
    Result result = acquireLease(bucket);
    if (result != SUCCESS) {
        mContext->mainStrand->post(
            std::tr1::bind(&CassandraStorage::completeCommit, this, trans, cb, result, (ReadSet*)NULL),
            "CassandraStorage::completeCommit"
        );
        return;
    }

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



String CassandraStorage::getLeaseBucketName(const Bucket& bucket) {
    return bucket.rawHexData() + "_lease";
}


CassandraStorage::LeaseRequestSet CassandraStorage::readLeaseRequests(const Bucket& bucket) {
    // This range should get any client IDs constructed of normal characters,
    // which should work ok for the client IDs we currently generate.
    SliceRange range;
    range.start = "";
    range.finish = "@";
    range.count = 100000;

    ReadSet readLeaseKeys = mDB->db()->getColumnsValues(getLeaseBucketName(bucket), LEASES_CF_NAME, range);

    LeaseRequestSet results;
    for(ReadSet::iterator it = readLeaseKeys.begin(); it != readLeaseKeys.end(); it++) {
        if (it->first == mClientID)
            continue;
        results.insert(it->first);
    }
    return results;
}

Storage::Result CassandraStorage::acquireLease(const Bucket& bucket) {
    if (mLeases.find(bucket) != mLeases.end())
        return SUCCESS;

    SILOG(cassandra-storage, detailed, "Trying to acquire lease for " << bucket);

    // Note that this doesn't follow the exact process described for a
    // wait-chain approach to locking for Cassandra. We don't actually want to
    // wait for the lock. So this essentially does one "round", just checking to
    // see if we won and failing if we didn't.
    //
    // The basic approach is to write a key indicating we want the lock. If
    // there is contention, when we do a range read to find all interested
    // lockers we'll get multiple results back. Then, we need to resolve the
    // conflict. The value in each key is a list of other clients we've seen
    // requesting the lock (i.e. other keys we got back from the range
    // read). By reporting back what we see, we acknowledge that we're waiting
    // on others. Eventually someone will see that everyone they're waiting for
    // has acked them and will be the first by lexicographical ordering (which
    // allows us to agree on a winner from the group without further
    // communication).
    //
    // This would always result in a winner if we looped, waiting for acks, but
    // here we just try one round. Because of this, we change the process to
    // *simply delete the request* if we wouldn't win within the set of requests
    // we observed, i.e. we aren't lexicographically the smallest. The
    // lexicographically smallest is the only one that sleeps for a bit then
    // tries to read back requests to see if they've cleared (lost) or are still
    // there (an existing request/successful lease that is still in use)
    // (i.e. this is the wait part of wait-chain -- the chain part effectively
    // goes away). You can think of this as request-nack, where the nack is
    // indicated by removal of themselves, instead of request-ack.
    //
    // It is possible we'll end up with nobody getting the lock due to the
    // contention and latency in updating the requests. Be careful with this
    // since it means you could, with sufficient churn/parallel requests, end up
    // with nobody ever getting the lock!
    try {
        // Express interest in the lease by writing our ID as a key. There are
        // no contents since we're not doing the full request-ack model.
        // (We have to use the full form because we need the ttl)
        SILOG(cassandra-storage, detailed, "Inserting lease request for " << bucket);
        mDB->db()->insertColumn(getLeaseBucketName(bucket), LEASES_CF_NAME, "", mClientID, "", org::apache::cassandra::ConsistencyLevel::QUORUM, (int)mLeaseDuration.seconds());

        // Now, read back all columns (requests for lease) in this bucket
        LeaseRequestSet can_be_earlier = readLeaseRequests(bucket);

        // If no other requests were read back, then we're not waiting for
        // anybody -- we've got it. Leave the request in place (which is now
        // really our lease since it blocks all future requests from succeeding)
        if (can_be_earlier.empty()) {
            SILOG(cassandra-storage, detailed, "Acquired lease because no other requests were found for " << bucket << ", adding to set of leases and setting up renew timer");
            mLeases.insert(bucket);
            mRenewTimes.push( BucketRenewTimeout(bucket, Timer::now() + (mLeaseDuration/2)) );
            return SUCCESS;
        }

        // Otherwise, we can only win if we're lexicographically the smallest of
        // the group that could have been ahead of us.
        for(LeaseRequestSet::iterator it = can_be_earlier.begin(); it != can_be_earlier.end(); it++) {
            if (*it < mClientID) {
                // We've lost, remove our request and return failure
                SILOG(cassandra-storage, detailed, "Failed to acquire lease due to ordering for " << bucket << ", removing request");
                mDB->db()->removeColumn(getLeaseBucketName(bucket), LEASES_CF_NAME, "", mClientID);
                return LOCK_ERROR;
            }
        }

        // Now, we've determined that we're the winner if everyone else we saw
        // also saw us (i.e. didn't beat us in the request, meaning they may not
        // be waiting for us). We need to wait a bit to make sure others have a
        // chance to process (do the remove from the above determination of
        // losing by ordering).
        // TODO(ewencp) this blocks things up, it might be better to spread
        // acquireLease across multiple parts to allow other work to happen in
        // the meantime.
        SILOG(cassandra-storage, detailed, "Waiting for contending lease requests to clear for " << bucket);
        Timer::sleep(Duration::milliseconds(100));

        // Then, read back all requests again.
        // TODO(ewencp) we could only (try to) read back the ones we care about...
        LeaseRequestSet updated_earlier = readLeaseRequests(bucket);

        // Now, if any of the ones we were waiting on are still there, then they
        // won by not knowing about us (because we weren't there when they made
        // their request).
        for(LeaseRequestSet::iterator it = can_be_earlier.begin(); it != can_be_earlier.end(); it++) {
            if (updated_earlier.find(*it) != updated_earlier.end()) {
                // They won and still have things locked up, clean up and
                // indicate failure.
                SILOG(cassandra-storage, detailed, "Failed to acquire lease for " << bucket << " because other client already has lock or didn't clear their request, removing request");
                mDB->db()->removeColumn(getLeaseBucketName(bucket), LEASES_CF_NAME, "", mClientID);
                return LOCK_ERROR;
            }
        }

        // Otherwise, we've finally actually won. Record that we have the lease
        // and setup renewals (keeping the TTL request key alive to keep
        // blocking others from acquiring the lock).
        SILOG(cassandra-storage, detailed, "Acquired lease for " << bucket << ", adding to set of leases and setting up renew timer");
        mLeases.insert(bucket);
        mRenewTimes.push( BucketRenewTimeout(bucket, Timer::now() + (mLeaseDuration/2)) );
        return SUCCESS;
    } catch(...) {
        SILOG(cassandra-storage, error, "Exception while acquiring lease key for " << bucket << ", trying to remove request");
        // Try to make sure we've cleaned up after ourselves so we don't keep
        // things locked unnecessarily.
        try {
            mDB->db()->removeColumn(getLeaseBucketName(bucket), LEASES_CF_NAME, "", mClientID);
        } catch(...) {
            // Can't really do anything if even this is failing.
            SILOG(cassandra-storage, detailed, "Exception while trying to cleanup after exception while acquiring lease for " << bucket << ", giving up. This may require waiting for a lease request to clear via TTL.");
        }
        return LOCK_ERROR;
    }

    return LOCK_ERROR;
}

void CassandraStorage::renewLease(const Bucket& bucket) {
    // We need to update the TTL on our key so we'll stay at the head of the
    // queue for longer.
    try {
        SILOG(cassandra-storage, detailed, "Updating TTL to renew lease for " << bucket);
        // Get the existing value so we don't change any state of acks in the
        // process of updating the lease
        String lease_value = mDB->db()->getColumnValue(getLeaseBucketName(bucket), LEASES_CF_NAME, mClientID);
        // Note that there's no convenience wrapper that allows us to specify
        // ttl without some other things, so we use the full form here, manually
        // specifying that there is no supercolumn and consistency = quorum.
        mDB->db()->insertColumn(getLeaseBucketName(bucket), LEASES_CF_NAME, "", mClientID, lease_value, org::apache::cassandra::ConsistencyLevel::QUORUM, (int)mLeaseDuration.seconds());
        mRenewTimes.push( BucketRenewTimeout(bucket, Timer::now() + (mLeaseDuration/2)) );
    }
    catch(...) {
        SILOG(cassandra-storage, error, "Error renewing lease key for " << bucket);
    }
}

void CassandraStorage::releaseLease(const Bucket& bucket) {
    SILOG(cassandra-storage, detailed, "releaseLease for " << bucket);
    if (mLeases.find(bucket) == mLeases.end()) return;

    // We don't have to do anything complicated here because "owning the lease"
    // just means you're currently first in line, i.e. your key was the first
    // written. We just need to make sure any request for the lease is removed.
    try {
        SILOG(cassandra-storage, detailed, "Erasing lease request column to release lease for " << bucket);
        // No convenience wrapper -- manually specify "" for no supercolumn
        mDB->db()->removeColumn(getLeaseBucketName(bucket), LEASES_CF_NAME, "", mClientID);
    }
    catch(...) {
        SILOG(cassandra-storage, error, "Error erasing lease key for " << bucket << ", but removing local record of lease anyway.");
    }
    mLeases.erase(bucket);
}


void CassandraStorage::processRenewals() {
    Time tnow = Timer::now();

    while(!mRenewTimes.empty() && mRenewTimes.front().t < tnow) {
        renewLease(mRenewTimes.front().bucket);
        mRenewTimes.pop();
    }

    if (!mRenewTimes.empty())
        mRenewTimer->wait(mRenewTimes.front().t - tnow);
}



} //end namespace OH
} //end namespace Sirikata
