/*  Sirikata -- Persistence Services
 *  ObjectStorage.hpp
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



#ifndef _OBJECT_STORAGE_HPP_
#define _OBJECT_STORAGE_HPP_


#include "util/UUID.hpp"


namespace Sirikata {

/** A key in the storage system. The key is constructed of an object identifier followed
 *  by a hierarchy of names.
 */
class SIRIKATA_EXPORT StorageKey {
public:
    StorageKey(const UUID& vwobj, uint32 obj, const String& field);

    bool operator<(const StorageKey& rhs) const;

    String toString() const;
private:
    // These allow the implementations of storage systems to access the
    // internal representation for efficiency purposes but keeps it
    // private from the rest of the system.
    friend class ObjectStorageHandler;

    UUID mVWObject;
    uint32 mObject;
    String mField;
};

/** A value in the storage system, just a simple memory buffer. */
class SIRIKATA_EXPORT StorageValue : public MemoryBuffer {
public:
    template<class T> StorageValue(const T& m) : MemoryBuffer(m) {}
    StorageValue(const char* input, std::size_t size) : MemoryBuffer((const unsigned char*)input, (const unsigned char*)input+size) {}
    template <class InputIterator> StorageValue(InputIterator begin, InputIterator end) : MemoryBuffer(begin, end) {}
    StorageValue() {}

    bool operator==(const StorageValue& rhs) const;
    bool operator!=(const StorageValue& rhs) const;
};

/** A storage set, which is simply a set of <key,value> pairs.  This can represent
 *  read sets, write sets, and compare sets.
 */
class SIRIKATA_EXPORT StorageSet {
protected:
    typedef std::map<StorageKey, StorageValue*> StorageKeyValueMap;

public:

    typedef StorageKeyValueMap::iterator iterator;
    typedef StorageKeyValueMap::const_iterator const_iterator;

    StorageSet();
    StorageSet(const StorageSet& cpy);
    ~StorageSet();

    StorageSet& operator=(const StorageSet& rhs);

    /** Get the size of this storage set. */
    uint32 size() const;

    /** Add a key without a value to this storage set.
     *  \param key the key to add to the set
     */
    void addKey(const StorageKey& key);

    /** Add a key-value pair to this storage set.  The value will be copied.
     *  \param key the key for the pair
     *  \param value the value for the pair
     */
    void addPair(const StorageKey& key, const StorageValue& value);

    /** Add a key-value pair to this storage set.  The set takes ownership of the value.
     *  \param key the key for the pair
     *  \param value the value for the pair
     */
    void addPair(const StorageKey& key, StorageValue* value);

    /** Set the value for a key.  The key must already be present in the storage
     *  set.  The value is copied.
     *  \param key the key to set the value for
     *  \param value the value to associate with the key
     */
    void setValue(const StorageKey& key, const StorageValue& value);

    /** Set the value for a key.  The key must already be present in the storage
     *  set.  The set takes ownership of the value.
     *  \param key the key to set the value for
     *  \param value the value to associate with the key
     */
    void setValue(const StorageKey& key, StorageValue* value);

    /** Get the value for a key.  The key must already be present with a value
     *  in the storage set.
     *  \param key the key to get the value for
     *  \returns a reference to the StorageValue for the given key
     */
    const StorageValue& getValue(const StorageKey& key) const;

    /** Returns true if this set contains the specified key.
     *  \param key the key to check for
     */
    bool hasKey(const StorageKey& key) const;

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

private:
    StorageKeyValueMap mMap;
};

/** Restricted version of StorageSet that represents a read set. */
class SIRIKATA_EXPORT ReadSet : public StorageSet {
public:
    ReadSet();
    ReadSet(const ReadSet& cpy);
    ~ReadSet();

    ReadSet& operator=(const ReadSet& rhs);

private:
    // These methods from StorageSet are overridden to hide them
    void addPair(const StorageKey& key, const StorageValue& value);
    void addPair(const StorageKey& key, StorageValue* value);
};

/** Restricted version of StorageSet that represents a write set. */
class SIRIKATA_EXPORT WriteSet : public StorageSet {
public:
    WriteSet();
    WriteSet(const WriteSet& cpy);
    ~WriteSet();

    WriteSet& operator=(const WriteSet& rhs);

private:
    // These methods from StorageSet are overridden to hide them
    void addKey(const StorageKey& key);
    void setValue(const StorageKey& key, const StorageValue& value);
    void setValue(const StorageKey& key, StorageValue* value);
};

/** Restricted version of StorageSet that represents a compare set. */
class SIRIKATA_EXPORT CompareSet : public StorageSet {
public:
    CompareSet();
    CompareSet(const CompareSet& cpy);
    ~CompareSet();

    CompareSet& operator=(const CompareSet& rhs);

private:
    // These methods from StorageSet are overridden to hide them
    void addKey(const StorageKey& key);
    void setValue(const StorageKey& key, const StorageValue& value);
    void setValue(const StorageKey& key, StorageValue* value);
};

/** Represents a set of read operations and a set of write operations.  Simply
 *  a combination of a ReadSet and WriteSet.  Used for temporary and best effort
 *  storage operations.
 */
class SIRIKATA_EXPORT ReadWriteSet {
public:
    ReadWriteSet();
    ReadWriteSet(const ReadWriteSet& cpy);

    ReadWriteSet& operator=(const ReadWriteSet& rhs);

    ReadSet& reads();
    const ReadSet& reads() const;
    WriteSet& writes();
    const WriteSet& writes() const;

private:
    ReadSet mReads;
    WriteSet mWrites;
};

/** A storage microtransaction.  Minitransactions perform their operations with
 *  ACID semantics in the following order:
 *  1) Compare values in the compare set, abort if any are not equal
 *  2) Read values from read set locations
 *  3) Write values to write set locations
 */
class SIRIKATA_EXPORT Minitransaction {
public:
    Minitransaction();
    Minitransaction(const Minitransaction& cpy);

    Minitransaction& operator=(const Minitransaction& rhs);

    CompareSet& compares();
    const CompareSet& compares() const;
    ReadSet& reads();
    const ReadSet& reads() const;
    WriteSet& writes();
    const WriteSet& writes() const;
private:
    CompareSet mCompares;
    ReadSet mReads;
    WriteSet mWrites;
};


enum ObjectStorageErrorType {
    ObjectStorageErrorType_None,
    ObjectStorageErrorType_KeyMissing,
    ObjectStorageErrorType_ComparisonFailed,
    ObjectStorageErrorType_Internal
};

class SIRIKATA_EXPORT ObjectStorageError {
public:

    explicit ObjectStorageError();
    explicit ObjectStorageError(const ObjectStorageErrorType t);
    ObjectStorageError(const ObjectStorageErrorType t, const String& msg);

    ObjectStorageErrorType type() const;

private:
    ObjectStorageErrorType mType;
    String mMsg;
};


/** Base class for ObjectStorage request handlers.  This provides . */
class SIRIKATA_EXPORT ObjectStorageHandler {
public:
    typedef std::tr1::function<void(ObjectStorageError)> ResultCallback;

    virtual ~ObjectStorageHandler();
protected:
    /** Returns the VWObject UUID section of a storage key. */
    static const UUID& keyVWObject(const StorageKey& key);
    /** Returns the sub-object ID section of a storage key. */
    static uint32 keyObject(const StorageKey& key);
    /** Returns the field name section of a storage key. */
    static const String& keyField(const StorageKey& key);
};

/** ReadWriteHandler is the abstract base class for implementations which handle
 *  best effort read/write storage sets.  These implementations don't handle
 *  transactions (meaning there's no support for compare sets).  Both temporary
 *  and best effort stored object store systems should implement this interface.
 */
class SIRIKATA_EXPORT ReadWriteHandler : public ObjectStorageHandler {
public:
    virtual ~ReadWriteHandler();

    virtual void apply(ReadWriteSet* rws, const ResultCallback& cb) = 0;
};

/** MinitransactionHandler is the abstract base class for implementations which
 *  handle ACID minitransactions.  Transactional storage systems should implement
 *  this interface.
 */
class SIRIKATA_EXPORT MinitransactionHandler : public ObjectStorageHandler {
public:
    virtual ~MinitransactionHandler();

    virtual void apply(Minitransaction* mt, const ResultCallback& cb) = 0;
};


/** Base class for object storage.  This replaces the old ObjectStore interface.
 *  It provides storage with various levels of guarantees and performance.
 *  The basic interface for all of these is a <key,value> store within an object's
 *  namespace.  Three types of storage are provided.  Temporary storage is not
 *  guaranteed to persist at all.  Best effort does as much as it can to persist
 *  but losses are possible.  Caching will make it look like values have been persisted
 *  even before they have.  Both temporary and best effort only allow reads and writes
 *  and these operations are *not* atomic for the set.  Atomic transactions provide the
 *  greatest guarantees - they are atomic, have read, write, and compare sets, and
 *  guarantee persistence, at the cost of much higher latency.
 */
class SIRIKATA_EXPORT ObjectStorage {
public:

    typedef std::tr1::function<void(ObjectStorageError, const ReadSet&)> ResultCallback;

    ObjectStorage(ReadWriteHandler* temp, ReadWriteHandler* bestEffort, MinitransactionHandler* trans);
    ~ObjectStorage();

    void temporary(ReadWriteSet* rws, const ResultCallback& cb);
    void bestEffort(ReadWriteSet* rws, const ResultCallback& cb);
    void transaction(Minitransaction* mt, const ResultCallback& cb);

private:
    ObjectStorage();

    void handleReadWriteResult(ReadWriteSet* rws, const ResultCallback& cb, const ObjectStorageError& error);
    void handleTransactionResult(Minitransaction* mt, const ResultCallback& cb, const ObjectStorageError& error);

    ReadWriteHandler* mTemporary;
    ReadWriteHandler* mBestEffort;
    MinitransactionHandler* mTransaction;
}; // class ObjectStorage

} // namespace Sirikata

#endif //_OBJECT_STORAGE_HPP_
