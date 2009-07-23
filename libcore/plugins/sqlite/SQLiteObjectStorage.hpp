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

#include "persistence/ObjectStorage.hpp"
#include "task/WorkQueue.hpp"
#include "SQLite.hpp"
#include "util/ThreadSafeQueue.hpp"
namespace Sirikata { namespace Persistence {

/** SQLite based object storage.  This class provides both ReadWriteHandler and
 *  MinitransactionHandler interfaces, but each instance should only be used for
 *  one type at any given time.  It is fine to instantiate one as a ReadWriteHandler
 *  and one as a MinitransactionHandler.  They will reference different tables in the
 *  database and so can happily coexist.
 */
class SQLiteObjectStorage : public ReadWriteHandler, public MinitransactionHandler {
public:
    static ReadWriteHandler* createReadWriteHandler(const String& pl);
    static MinitransactionHandler* createMinitransactionHandler(const String& pl);
    virtual ~SQLiteObjectStorage();

    virtual Persistence::Protocol::Minitransaction* createMinitransaction(int numReadKeys, int numWriteKeys, int numCompares);
    virtual void apply(const RoutableMessageHeader&rmh,Protocol::Minitransaction*);
    
    virtual Persistence::Protocol::ReadWriteSet* createReadWriteSet(int numReadKeys, int numWriteKeys)=0;
    virtual void apply(const RoutableMessageHeader&rmh,Protocol::ReadWriteSet*);
    bool forwardMessagesTo(MessageService*);
    bool endForwardingMessagesTo(MessageService*);
    void processMessage(const RoutableMessageHeader&,MemoryReference);
    virtual void apply(ReadWriteSet* rws, const ResultCallback& cb);
    virtual void apply(Minitransaction* mt, const ResultCallback& cb);
    static SQLiteObjectStorage*create(bool transactional,const String&);
private:
    OptionSet*mOptions;
    Task::WorkQueue *mDiskWorkQueue;
    Task::ThreadSafeWorkQueue _mLocalWorkQueue;
    Task::WorkQueueThread* mWorkQueueThread;
    enum Error {
        None,
        KeyMissing,
        ComparisonFailed,
        DatabaseLocked
    };

    SQLiteObjectStorage(bool transactional, const String& pl);

    /** Worker method which will be executed in another thread to perform the
     *  application of the ReadWriteSet.
     */
    class ApplyReadWriteWorker:public Task::WorkItem{
        SQLiteObjectStorage*mParent;
        ResultCallback cb;
        ReadWriteSet*rws;
    public:
        ApplyReadWriteWorker(SQLiteObjectStorage*parent, ReadWriteSet* rws, const ResultCallback&cb);
        void operator()();
    };

    /** Worker method which will be executed in another thread to perform the
     *  application of the Minitransaction.
     */
    class ApplyTransactionWorker:public Task::WorkItem{
        SQLiteObjectStorage*mParent;
        ResultCallback cb;
        Minitransaction*mt;
    public:
        ApplyTransactionWorker(SQLiteObjectStorage*parent, Minitransaction* rws, const ResultCallback&cb);
        void operator()();
    };


    /** Starts a database transaction. */
    void beginTransaction(const SQLiteDBPtr& db);
    /** Attempts to commit a database transaction. */
    bool commitTransaction(const SQLiteDBPtr& db);
    /** Roll back a transaction in progress. */
    void rollbackTransaction(const SQLiteDBPtr& db);

    /** Reads values for the keys specified in a ReadSet. This is a suboperation -
     *  it may or may not complete immediately depending on whether a transaction
     *  has been started.
     *  \param db the database connection
     *  \param rs the ReadSet
     *  \returns an error code or None if there was no error
     */
    Error applyReadSet(const SQLiteDBPtr& db, ReadSet& rs);
    /** Writes the <key,value> pairs specified in a WriteSet. This is a suboperation -
     *  it may or may not complete immediately depending on whether a transaction
     *  has been started.
     *  \param db the database connection
     *  \param ws the WriteSet
     *  \param retries number of times to retry a write if it fails
     *  \returns an error code or None if there was no error
     */
    Error applyWriteSet(const SQLiteDBPtr& db, WriteSet& ws, int retries);
    /** Checks the values in the compare set against the ones in the database.
     *  \returns true if all the values match, false otherwise
     *  \param db the database connection
     *  \param cs the CompareSet
     *  \returns an error code or None if there was no error
     */
    Error checkCompareSet(const SQLiteDBPtr& db, CompareSet& cs);

    /** Helper method to extract the database table name from a storage key. */
    String getTableName(const StorageKey& key);
    /** Helper method to extract the key within a database table for a storage
     *   key.
     */
    String getKeyName(const StorageKey& key);

    /** Translates internal errors to ObjectStorageErrors. */
    static ObjectStorageError convertError(Error internal);

    bool mTransactional;
    String mDBName;
    int mRetries;
    int mBusyTimeout; // locked database timeout in milliseconds
};

} }// namespace Sirikata::Persistence


#endif //_SQLITE_OBJECT_STORAGE_HPP_
