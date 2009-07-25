
/*  Sirikata liboh -- Object Host
 *  TopLevelSpaceConnection.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#ifndef _SIRIKATA_TOP_LEVEL_SPACE_CONNECTION_HPP_
#define _SIRIKATA_TOP_LEVEL_SPACE_CONNECTION_HPP_

#include <oh/Platform.hpp>
#include <oh/ProxyManager.hpp>
#include <network/Address.hpp>

namespace Sirikata {

class SIRIKATA_OH_EXPORT ObjectHostProxyManager :public ProxyManager, public Noncopyable {
protected:
    struct ObjectHostProxyInfo {
        ProxyObjectPtr obj;
        int refCount;
        ObjectHostProxyInfo(const ProxyObjectPtr &obj)
            : obj(obj), refCount(0) {
        }
        inline bool operator<(const ObjectHostProxyInfo &other) const {
            return obj->getObjectReference() < other.obj->getObjectReference();
        }
        inline bool operator==(const ObjectHostProxyInfo &other) const {
            return obj->getObjectReference() == other.obj->getObjectReference();
        }
    };
    class QueryHasher {
        UUID::Hasher uuidHash;
    public:
        size_t operator()(const std::pair<UUID, uint32>&mypair) const{
            return uuidHash(mypair.first)*17 + mypair.second*19;
        }
    };
    typedef std::tr1::unordered_map<ObjectReference, ObjectHostProxyInfo, ObjectReference::Hasher> ProxyMap;
    typedef std::tr1::unordered_map<std::pair<UUID, uint32>, std::set<ProxyObjectPtr>, QueryHasher > QueryMap;
    ProxyMap mProxyMap;
    QueryMap mQueryMap; // indexed by {ObjectHost::mInternalObjectReference, ProxCall::query_id()}
    SpaceID mSpaceID;
public:
	~ObjectHostProxyManager();
    void initialize();
    void destroy();

    void createObject(const ProxyObjectPtr &newObj);
    void destroyObject(const ProxyObjectPtr &newObj);

    void createObjectProximity(const ProxyObjectPtr &newObj, const UUID &seeker, uint32 queryId);
    void destroyObjectProximity(const ProxyObjectPtr &newObj, const UUID &seeker, uint32 queryId);
    void destroyProximityQuery(const UUID &seeker, uint32 queryId);

    ProxyObjectPtr getProxyObject(const SpaceObjectReference &id) const;
};

class ObjectHost;
class SIRIKATA_OH_EXPORT TopLevelSpaceConnection :public ObjectHostProxyManager {
    ObjectHost*mParent;
    Network::Address mRegisteredAddress;
    Network::Stream *mTopLevelStream;
    void removeFromMap();
    static void connectToAddress(const std::tr1::weak_ptr<TopLevelSpaceConnection>&weak_thus,ObjectHost*oh,const Network::Address*addy);

  public:
    TopLevelSpaceConnection(Network::IOService*);
    ~TopLevelSpaceConnection();
    Network::Stream *topLevelStream(){
        return mTopLevelStream;
    }
    void remoteDisconnection(const std::string&reason);
    const SpaceID&id()const {return mSpaceID;}
    const Network::Address&address()const {return mRegisteredAddress;}
    ///connects the SST stream to the given IP address unless it is null
    void connect(const std::tr1::weak_ptr<TopLevelSpaceConnection>&weak_thus,ObjectHost*,const SpaceID&,const Network::Address&addy);
    void connect(const std::tr1::weak_ptr<TopLevelSpaceConnection>&weak_thus,ObjectHost*,const SpaceID&);
};
/*
class HostedObjectListener {
public:
    virtual void created(HostedObject *hostedObject) {
    }

    virtual void destroyed(HostedObject *hostedObject) = 0;
};

class HostedObject : public Provider<HostedObjectListener*>, ProxyObjectListener {
    SSTStream *mObjectStream;
    SpaceConnection *mSpace;
    ProxyObjectPtr mProxy;

    virtual void destroyed() {
        this->call(&HostedObjectListener::destroyed, this);
        delete this;
    }

protected:
    void creationCallback(const ProxyObjectPtr &proxyPtr) {
        mProxy = proxyPtr;
        this->call(&HostedObjectListener::created, this);
    }
public:
    // Sends initial packet on this stream, notifies ProxyManager when ready.
    HostedObject(SpaceConnection *mSpaceConnection);
    void create();
    const ProxyObjectPtr &getProxyPtr() const {
        return mProxy;
    }
    ProxyObject &getProxy() const {
        return *mProxy;
    }
};
*/
}

#endif
