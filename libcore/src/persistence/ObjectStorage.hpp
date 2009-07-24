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
    template <class ReadWrite> class DestroyReadWrite {public:
        static void destroyReadWriteSet(ReadWrite*mt) {delete mt;}
    };
public:
    virtual ~ReadWriteHandler();
    template <class ReadWrite> ReadWrite *createReadWriteSet(ReadWrite*mt, int numReadKeys, int numWriteKeys) {
        if (mt==NULL) mt = new ReadWrite();
        while(numReadKeys--) {
            mt->add_reads();
        }
        while(numWriteKeys--) {
            mt->add_writes();
        }
        return mt;
    }
    template <class ReadWrite> void apply(ReadWrite*rws, const ResultCallback& cb) {
        applyInternal(rws,cb,&DestroyReadWrite<ReadWrite>::destroyReadWriteSet);
    }
    template <class ReadWrite> void applyMessage(const RoutableMessageHeader &rmh,ReadWrite*rws) {
        applyInternal(rmh,rws,&DestroyReadWrite<ReadWrite>::destroyReadWriteSet);
    }
  protected:
    virtual void applyInternal(Protocol::ReadWriteSet* rws, const ResultCallback& cb,void(*)(Protocol::ReadWriteSet*)) = 0;
    virtual void applyInternal(const RoutableMessageHeader&hdr,Persistence::Protocol::ReadWriteSet*,void(*)(Protocol::ReadWriteSet*))=0;
};

/** MinitransactionHandler is the abstract base class for implementations which
 *  handle ACID minitransactions.  Transactional storage systems should implement
 *  this interface.
 */
class SIRIKATA_EXPORT MinitransactionHandler : public ObjectStorageHandler, public MessageService {
    template <class Minitransaction> class DestroyMinitransaction {public:
        static void destroyMinitransaction(Minitransaction*mt) {delete mt;}
    };
public:
    virtual ~MinitransactionHandler();
    //virtual Persistence::Protocol::Minitransaction* createMinitransaction(int numReadKeys, int numWriteKeys, int numCompares)=0;
    template <class Minitrans> Minitrans* createMinitransaction(Minitrans*mt, int numReadKeys, int numWriteKeys, int numCompares) {
        if (mt==NULL) mt = new Minitrans();
        while(numReadKeys--) {
            mt->add_reads();
        }
        while(numWriteKeys--) {
            mt->add_writes();
        }
        while(numCompares--) {
            mt->add_compares();
        }
        return mt;
    }
    template <class Minitransaction> void transact(Minitransaction*rws, const ResultCallback& cb) {
        applyInternal(rws,cb,&DestroyMinitransaction<Minitransaction>::destroyMinitransaction);
    }
    template <class Minitransaction> void transactMessage(const RoutableMessageHeader &rmh,Minitransaction*rws) {
        applyInternal(rmh,rws,&DestroyMinitransaction<Minitransaction>::destroyMinitransaction);
    }
    
  protected:
    virtual void applyInternal(const RoutableMessageHeader&hdr,Persistence::Protocol::Minitransaction*, void(*minitransactionDestruction)(Protocol::Minitransaction*))=0;
    virtual void applyInternal(Protocol::Minitransaction* mt, const ResultCallback& cb, void(*minitransactionDestruction)(Protocol::Minitransaction*)) = 0;
};


} } // namespace Sirikata::Persistence

#endif //_OBJECT_STORAGE_HPP_
