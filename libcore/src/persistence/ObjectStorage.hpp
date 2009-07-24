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


namespace Sirikata { namespace Persistence { 


namespace Protocol {
class StorageKey;
class StorageValue;
class StorageElement;
class CompareElement;
class ReadSet;
class WriteSet;
class ReadWriteSet;
class Minitransaction;
class Response;
}

template<class StorageX, class StorageY> void copyStorageKey(StorageX a, const StorageY b) {
    if(b.has_object_uuid())
        a.set_object_uuid(b.object_uuid());
    else a.clear_object_uuid();
    if(b.has_field_id())
        a.set_field_id(b.field_id());
    else a.clear_field_id();
    if(b.has_field_name())
        a.set_field_name(b.field_name());
    else a.clear_field_name();
}

template<class StorageX, class StorageY> bool sameStorageKey(StorageX a, const StorageY b) {
    return ((b.has_object_uuid()==false&&a.has_object_uuid()==false)||a.object_uuid()==b.object_uuid())
        &&((b.has_field_id()==false&&a.has_field_id()==false)||a.field_id()==b.field_id())
        &&((b.has_field_name()==false&&a.has_field_name()==false)||a.field_id()==b.field_name());
}

template<class StorageX, class StorageY> bool sameStorageValue(StorageX a, const StorageY b) {
    return (b.has_data()==false&&a.has_data()==false)||a.data()==b.data();
}
template<class StorageX, class StorageY> bool sameStorageElement(StorageX a, const StorageY b) {
    return sameStorageValue(a,b)&&sameStorageKey(a,b);
}


template<class StorageX, class StorageY> bool sameStorageSet(StorageX a, const StorageY b) {
    if (a.reads_size()!=b.reads_size())
        return false;
    int len=a.reads_size();
    for (int i=0;i<len;++i) {
        if (!sameStorageElement(a.reads(i),b.reads(i)))
            return false;
    }
}


template<class StorageX, class StorageY> bool sameStorageValues(StorageX a, const StorageY b) {
    if (a.reads_size()!=b.reads_size())
        return false;
    int len=a.reads_size();
    for (int i=0;i<len;++i) {
        if (!sameStorageValue(a.reads(i),b.reads(i)))
            return false;
    }
}


template<class StorageX, class StorageY> void mergeStorageKey(StorageX a, const StorageY b) {
    if(b.has_object_uuid())
        a.set_object_uuid(b.object_uuid());
    if(b.has_field_id())
        a.set_field_id(b.field_id());
    if(b.has_field_name())
        a.set_field_name(b.field_name());
}
template<class StorageX, class StorageY> void copyStorageValue(StorageX a, const StorageY b) {
    if(b.has_data())
        a.set_data(b.data());
    else a.clear_data();
}

template<class StorageX, class StorageY> void mergeStorageValue(StorageX a, const StorageY b) {
    if(b.has_data())
        a.set_data(b.data());
}

template<class StorageX, class StorageY> void copyStorageElement(StorageX a, const StorageY b) {
    copyStorageKey(a,b);
    copyStorageValue(a,b);
}
template<class StorageX, class StorageY> void mergeStorageElement(StorageX a, const StorageY b) {
    mergeStorageKey(a,b);
    mergeStorageValue(a,b);
}

template<class StorageX, class StorageY> void copyCompareElement(StorageX a, const StorageY b) {
    copyStorageKey(a,b);
    copyStorageValue(a,b);
    if (b.has_comparator()) {
        a.set_comparator(a.comparator());        
    }else {
        a.clear_comparator();
    }
}
template<class StorageX, class StorageY> void mergeCompareElement(StorageX a, const StorageY b) {
    mergeStorageKey(a,b);
    mergeStorageValue(a,b);
    if (b.has_comparator()) {
        a.set_comparator(a.comparator());        
    }
}






/** Base class for ObjectStorage request handlers.  This provides . */
class SIRIKATA_EXPORT ObjectStorageHandler {
public:
    typedef std::tr1::function<void(Protocol::Response*)> ResultCallback;

    virtual ~ObjectStorageHandler();
    virtual void destroyResponse(Protocol::Response*)=0;
};

/** ReadWriteHandler is the abstract base class for implementations which handle
 *  best effort read/write storage sets.  These implementations don't handle
 *  transactions (meaning there's no support for compare sets).  Both temporary
 *  and best effort stored object store systems should implement this interface.
 */
class SIRIKATA_EXPORT ReadWriteHandler : public ObjectStorageHandler, public MessageService {
public:
    virtual ~ReadWriteHandler();
    virtual Protocol::ReadWriteSet* createReadWriteSet(int numReadKeys, int numWriteKeys)=0;
    virtual void apply(Protocol::ReadWriteSet* rws, const ResultCallback& cb) = 0;
    virtual void apply(const RoutableMessageHeader&hdr,Persistence::Protocol::ReadWriteSet*)=0;
};

/** MinitransactionHandler is the abstract base class for implementations which
 *  handle ACID minitransactions.  Transactional storage systems should implement
 *  this interface.
 */
class SIRIKATA_EXPORT MinitransactionHandler : public ObjectStorageHandler, public MessageService {
public:
    virtual ~MinitransactionHandler();
    virtual Persistence::Protocol::Minitransaction* createMinitransaction(int numReadKeys, int numWriteKeys, int numCompares)=0;
    virtual void apply(const RoutableMessageHeader&hdr,Persistence::Protocol::Minitransaction*)=0;
    virtual void apply(Protocol::Minitransaction* mt, const ResultCallback& cb) = 0;
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

    typedef std::tr1::function<void(const Protocol::Response*)> ResultCallback;

    ObjectStorage(ReadWriteHandler* temp, ReadWriteHandler* bestEffort, MinitransactionHandler* trans);
    ~ObjectStorage();

    void temporary(Protocol::ReadWriteSet* rws, const ResultCallback& cb);
    void bestEffort(Protocol::ReadWriteSet* rws, const ResultCallback& cb);
    void transaction(Protocol::Minitransaction* mt, const ResultCallback& cb);

private:
    ObjectStorage();

    ReadWriteHandler* mTemporary;
    ReadWriteHandler* mBestEffort;
    MinitransactionHandler* mTransaction;
}; // class ObjectStorage

} } // namespace Sirikata::Persistence

#endif //_OBJECT_STORAGE_HPP_
