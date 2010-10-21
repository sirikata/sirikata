/*  Sirikata -- SQLite plugin -- Persistence Services
 *  SQLiteObjectStorage.hpp
 *
 *  Copyright (c) 2008, Stanford University
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
#ifndef _SQLITE_OBJECT_STORAGE_HPP_
#define _SQLITE_OBJECT_STORAGE_HPP_

#include <sirikata/core/persistence/ObjectStorage.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include "SQLite.hpp"
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
namespace Sirikata { namespace Persistence {

/** SQLite based object storage.  This class provides both ReadWriteHandler and
 *  MinitransactionHandler interfaces, but each instance should only be used for
 *  one type at any given time.  It is fine to instantiate one as a ReadWriteHandler
 *  and one as a MinitransactionHandler.  They will reference different tables in the
 *  database and so can happily coexist.
 */
class SQLiteObjectStorage : public ReadWriteHandler, public MinitransactionHandler {
    Persistence::Protocol::Minitransaction* createMinitransaction(int numReadKeys, int numWriteKeys, int numCompares);
public:
    static ReadWriteHandler* createReadWriteHandler(const String& pl);
    static MinitransactionHandler* createMinitransactionHandler(const String& pl);
    virtual ~SQLiteObjectStorage();


    virtual void destroyResponse(Persistence::Protocol::Response*);

    virtual Persistence::Protocol::ReadWriteSet* createReadWriteSet(int numReadKeys, int numWriteKeys);
    virtual void applyInternal(Sirikata::Persistence::Protocol::Minitransaction*, const ResultCallback&, void(*minitransactionDestruction)(Protocol::Minitransaction*));
    virtual void applyInternal(Sirikata::Persistence::Protocol::ReadWriteSet*, const ResultCallback&,void(*)(Protocol::ReadWriteSet*));

    static SQLiteObjectStorage*create(bool transactional,const String&);
private:
    std::vector<MessageService*>mInterestedParties;
    OptionSet*mOptions;
    Task::WorkQueue *mDiskWorkQueue;
    Task::ThreadSafeWorkQueue _mLocalWorkQueue;
    Task::WorkQueueThread* mWorkQueueThread;
    SQLiteObjectStorage(bool transactional, const String& pl);

    /** Worker method which will be executed in another thread to perform the
     *  application of the ReadWriteSet.
     */
    class ApplyReadWriteWorker:public Task::WorkItem{
    protected:
        void (*mDestroyReadWrite)(Protocol::ReadWriteSet*);
        SQLiteObjectStorage*mParent;
        ResultCallback cb;
        Protocol::ReadWriteSet*rws;
        Protocol::Response*mResponse;
        Protocol::Response::ReturnStatus processReadWrite();
        ApplyReadWriteWorker(){rws=NULL;mResponse=NULL;mParent=NULL;}
    public:
        ApplyReadWriteWorker(SQLiteObjectStorage*parent, Protocol::ReadWriteSet* rws, const ResultCallback&cb,void (*mDestroyReadWrite)(Protocol::ReadWriteSet*));
        void operator()();
    };
    class ApplyReadWriteMessage:public ApplyReadWriteWorker{
    public:
        ApplyReadWriteMessage(SQLiteObjectStorage*parent, Protocol::ReadWriteSet* rws, void (*mDestroyReadWrite)(Protocol::ReadWriteSet*));
        void operator()();
    };

    /** Worker method which will be executed in another thread to perform the
     *  application of the Minitransaction.
     */
    class ApplyTransactionWorker:public Task::WorkItem{
    protected:
        void (*mDestroyMinitransaction)(Protocol::Minitransaction*);
        SQLiteObjectStorage*mParent;
        ResultCallback cb;
        Protocol::Minitransaction*mt;
        Protocol::Response*mResponse;
        Protocol::Response::ReturnStatus processTransaction();
        ApplyTransactionWorker(){mt=NULL;mResponse=NULL;mParent=NULL;}
    public:
        ApplyTransactionWorker(SQLiteObjectStorage*parent, Protocol::Minitransaction* rws, const ResultCallback&cb,void (*mDestroyMinitransaction)(Protocol::Minitransaction*));
        void operator()();
    };
    class ApplyTransactionMessage:public ApplyTransactionWorker{
    public:
        ApplyTransactionMessage(SQLiteObjectStorage*parent, Protocol::Minitransaction* rws, void (*mDestroyMinitransaction)(Protocol::Minitransaction*));
        void operator()();
    };

    /** Starts a database transaction. */
    void beginTransaction(const SQLiteDBPtr& db);
    /** Attempts to commit a database transaction. */
    bool commitTransaction(const SQLiteDBPtr& db);
    /** Roll back a transaction in progress. */
    void rollbackTransaction(const SQLiteDBPtr& db);
    enum Error {
        None,
        KeyMissing,
        ComparisonFailed,
        DatabaseLocked
    };

    /** Reads values for the keys specified in a ReadSet. This is a suboperation -
     *  it may or may not complete immediately depending on whether a transaction
     *  has been started.
     *  \param db the database connection
     *  \param rs the ReadSet
     *  \param retval output response from the operation.
     *  \returns an error code or None if there was no error
     */
    template <class ReadSet> Error applyReadSet(const SQLiteDBPtr& db, const ReadSet& rs, Protocol::Response&retval);
    /** Writes the <key,value> pairs specified in a WriteSet. This is a suboperation -
     *  it may or may not complete immediately depending on whether a transaction
     *  has been started.
     *  \param db the database connection
     *  \param ws the WriteSet
     *  \param retries number of times to retry a write if it fails
     *  \returns an error code or None if there was no error
     */
    template <class WriteSet>Error applyWriteSet(const SQLiteDBPtr& db, const WriteSet& ws, int retries);
    /** Checks the values in the compare set against the ones in the database.
     *  \returns true if all the values match, false otherwise
     *  \param db the database connection
     *  \param cs the CompareSet
     *  \returns an error code or None if there was no error
     */
    template <class CompareSet> Error checkCompareSet(const SQLiteDBPtr& db, const CompareSet& cs);

    /** Helper method to extract the database table name from a storage key. */
    template <class StorageKey> String getTableName(const StorageKey& key);
    /** Helper method to extract the key within a database table for a storage
     *   key.
     */
    template <class StorageKey> String getKeyName(const StorageKey& key);

    /** Translates internal errors to ObjectStorageErrors. */
    static Protocol::Response::ReturnStatus convertError(Error internal);

    bool mTransactional;
    String mDBName;
    SQLiteDBPtr mDB;
    int mRetries;
    int mBusyTimeout; // locked database timeout in milliseconds
};

} }// namespace Sirikata::Persistence


#endif //_SQLITE_OBJECT_STORAGE_HPP_
