/*  Sirikata
 *  CassandraStorage.hpp
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

#ifndef __SIRIKATA_OH_STORAGE_CASSANDRA_HPP__
#define __SIRIKATA_OH_STORAGE_CASSANDRA_HPP__

#include <sirikata/oh/Storage.hpp>
#include <sirikata/cassandra/Cassandra.hpp>

namespace Sirikata {
namespace OH {

class FileStorageEvent;

class CassandraStorage : public Storage
{
public:
    CassandraStorage(ObjectHostContext* ctx, const String& host, int port);
    ~CassandraStorage();

    virtual void start();
    virtual void stop();

    virtual void beginTransaction(const Bucket& bucket);
    virtual void commitTransaction(const Bucket& bucket, const CommitCallback& cb = 0, const String& timestamp="current");
    virtual bool erase(const Bucket& bucket, const Key& key, const CommitCallback& cb = 0, const String& timestamp="current");
    virtual bool write(const Bucket& bucket, const Key& key, const String& value, const CommitCallback& cb = 0, const String& timestamp="current");
    virtual bool read(const Bucket& bucket, const Key& key, const CommitCallback& cb = 0, const String& timestamp="current");
    virtual bool rangeRead(const Bucket& bucket, const Key& start, const Key& finish, const CommitCallback& cb = 0, const String& timestamp="current");
    virtual bool rangeErase(const Bucket& bucket, const Key& start, const Key& finish, const CommitCallback& cb = 0, const String& timestamp="current");
    virtual bool compare(const Bucket& bucket, const Key& key, const String& value, const CommitCallback& cb = 0, const String& timestamp="current");
    virtual bool count(const Bucket& bucket, const Key& start, const Key& finish, const CountCallback& cb = 0, const String& timestamp="current");


private:

    typedef std::vector<String> Keys;
    typedef org::apache::cassandra::Column Column;
    typedef std::vector<Column> Columns;
    typedef org::apache::cassandra::SliceRange SliceRange;
    typedef std::vector<SliceRange> SliceRanges;
    typedef org::apache::cassandra::ColumnParent ColumnParent;
    typedef org::apache::cassandra::SlicePredicate SlicePredicate;

    typedef std::tr1::tuple<String,   //column family
                            String,   //row key
                            String,   //super column name
                            Columns,  //columns to write
                            Keys      //keys to erase
                          > batchTuple;

    // StorageActions are individual actions to take, i.e. read, write,
    // erase. We queue them up in a list and eventually fire them off in a
    // transaction.
    struct StorageAction {
        enum Type {
            Read,
            ReadRange,
            Compare,
            Write,
            Erase,
            EraseRange,
            Error
        };

        StorageAction();
        StorageAction(const StorageAction& rhs);
        ~StorageAction();

        StorageAction& operator=(const StorageAction& rhs);

        // Executes this action: push action to lists and wait for commitment
        void execute(const Bucket& bucket, Columns* columns, Keys* eraseKeys, Keys* readKeys, SliceRanges* readRanges, ReadSet* compares, SliceRanges* eraseRanges, const String& timestamp);

        // Bucket is implicit, passed into execute
        Type type;
        Key key;
        Key keyEnd;
        String* value;
    };

    typedef std::vector<StorageAction> Transaction;
    typedef std::tr1::unordered_map<Bucket, Transaction*, Bucket::Hasher> BucketTransactions;

    // Initializes the database.
    void initDB();

    // Gets the current transaction or creates one. Also can return whether the
    // transaction was just created, e.g. to tell whether an operation is an
    // implicit transaction.
    Transaction* getTransaction(const Bucket& bucket, bool* is_new = NULL);

    // Executes a commit. Runs in a separate thread, so the transaction is
    // passed in directly
    void executeCommit(const Bucket& bucket, Transaction* trans, CommitCallback cb, const String& timestamp);

    void executeCount(const Bucket& bucket, ColumnParent& parent, SlicePredicate& predicate, CountCallback cb, const String& timestamp);

    // Complete a commit back in the main thread, cleaning it up and dispatching the callback
    void completeCommit(Transaction* trans, CommitCallback cb, bool success, ReadSet* rs);
    void completeCount(CountCallback cb, bool success, int32 count);

    // Call libcassandra methods to commit transcation
    bool CassandraCommit(CassandraDBPtr db, const Bucket& bucket, Columns* columns, Keys* eraseKeys, Keys* readKeys, SliceRanges* readRanges, ReadSet* compares, SliceRanges* eraseRanges, ReadSet* rs, const String& timestamp);

    ObjectHostContext* mContext;
    BucketTransactions mTransactions;
    String mDBHost;              //host name of Cassandra server
    int mDBPort;
    CassandraDBPtr mDB;

    Network::IOService* mIOService;
    Network::IOWork* mWork;
    Thread* mThread;
};

}//end namespace OH
}//end namespace Sirikata

#endif //__SIRIKATA_OH_STORAGE_Cassandra_HPP__
