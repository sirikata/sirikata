/*  Sirikata Space Node -- Proxy Creation and Destruction manager
 *  SpaceProxyManager.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef SIRIKATA_SPACE_SPACE_PROXY_MANAGER_HPP
#define SIRIKATA_SPACE_SPACE_PROXY_MANAGER_HPP

#include <space/Platform.hpp>
#include <proxyobject/Platform.hpp>
#include <proxyobject/ProxyManager.hpp>
#include <proxyobject/TimeOffsetManager.hpp>
#include <proxyobject/VWObject.hpp>
#include <core/odp/DelegateService.hpp>

namespace Sirikata {
namespace Space {
class Space;
}
class SIRIKATA_SPACE_EXPORT SpaceProxyManager : public VWObject, public ProxyManager {
    typedef std::tr1::unordered_map<ObjectReference, ProxyObjectPtr,ObjectReference::Hasher> ProxyMap;
  protected:
    QueryTracker mQueryTracker;
    SimpleTimeOffsetManager mOffsetManager;
    Space::Space*mSpace;
  private:
    ProxyMap mProxyMap;
    std::tr1::unordered_map<ObjectReference,std::set<uint32>, ObjectReference::Hasher > mQueryMap;

    ODP::DelegateService* mDelegateODPService;
  public:
    SpaceProxyManager(Space::Space*space, Network::IOService*io);
	~SpaceProxyManager();
    void initialize();
    void destroy();
    const TimeOffsetManager* getTimeOffsetManager()const {return &mOffsetManager;}
    void createObject(const ProxyObjectPtr &newObj, QueryTracker*viewer);
    void destroyObject(const ProxyObjectPtr &delObj, QueryTracker*viewer);
    void clearQuery(uint32 query_id);
    QueryTracker *getQueryTracker(const SpaceObjectReference &id);

    ProxyObjectPtr getProxyObject(const SpaceObjectReference &id) const;
    bool isLocal(const SpaceObjectReference&)const;
    ProxyManager* getProxyManager(const SpaceID&);
    QueryTracker* getTracker();
    void addQueryInterest(uint32 query_id, const SpaceObjectReference&);
    void removeQueryInterest(uint32 query_id, const ProxyObjectPtr&, const SpaceObjectReference&);

    // FIXME Why is this even a VWObject?
    // ODP::Service Interface
    virtual ODP::Port* bindODPPort(SpaceID space, ODP::PortID port);
    virtual ODP::Port* bindODPPort(SpaceID space);
    virtual void registerDefaultODPHandler(const ODP::MessageHandler& cb);
  private:
    ODP::DelegatePort* createDelegateODPPort(ODP::DelegateService* parentService, SpaceID space, ODP::PortID port);
    bool delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload);
};
}
#endif
