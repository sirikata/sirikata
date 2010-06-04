/*  Sirikata
 *  SpaceProxyManager.cpp
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

#include "space/SpaceProxyManager.hpp"
#include "space/Space.hpp"
#include "proxyobject/ProxyCameraObject.hpp"

#include <sirikata/core/odp/DelegatePort.hpp>
#include <sirikata/core/odp/Exceptions.hpp>
#include <sirikata/core/util/RoutableMessageHeader.hpp>

namespace Sirikata {
SpaceProxyManager::SpaceProxyManager(Space::Space*space, Network::IOService*io)
 : mSpace(space)
{
    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &SpaceProxyManager::createDelegateODPPort, this,
            _1, _2, _3
        )
    );
    mQueryTracker = new QueryTracker(bindODPPort(mSpace->id()), io);
}
SpaceProxyManager::~SpaceProxyManager() {
    delete mQueryTracker;
    delete mDelegateODPService;
}

void SpaceProxyManager::createObject(const ProxyObjectPtr &newObj, QueryTracker*tracker){
    if (tracker!=NULL&&tracker!=mQueryTracker) {
        SILOG(space,error,"Trying to add interest to a new object with an incorrect QueryTracker");
        return;
    }
    ProxyMap::iterator iter = mProxyMap.find(newObj->getObjectReference().object());
    if (iter == mProxyMap.end()) {
        mProxyMap[newObj->getObjectReference().object()]=newObj;
        notify(&ProxyCreationListener::onCreateProxy,newObj);
        ProxyCameraObject* cam = dynamic_cast<ProxyCameraObject*>(&*newObj);
        if (cam) {
            cam->attach("",0,0);
        }
    }
}
void SpaceProxyManager::destroyObject(const ProxyObjectPtr &obj, QueryTracker*tracker){
    if (tracker!=NULL&&tracker!=mQueryTracker) {
        SILOG(space,error,"Trying to add interest to a new object with an incorrect QueryTracker");
        return;
    }
    if (mSpace->id()==obj->getObjectReference().space()) {
        ProxyMap::iterator where=mProxyMap.find(obj->getObjectReference().object());
        if (where!=mProxyMap.end()) {
            obj->destroy(mOffsetManager.now(*obj));
            notify(&ProxyCreationListener::onDestroyProxy,obj);
            mProxyMap.erase(where);
        }else {
            SILOG(space,error,"Trying to remove query interest in object "<<obj->getObjectReference()<<" from space when it is not in Proxy Map");
        }
    }else {
        SILOG(space,error,"Trying to remove query interest in space "<<obj->getObjectReference().space()<<" from space ID "<<mSpace->id());
    }

}
void SpaceProxyManager::addQueryInterest(uint32 query_id, const SpaceObjectReference&id) {
    mQueryMap[id.object()].insert(query_id);
}
void SpaceProxyManager::removeQueryInterest(uint32 query_id, const ProxyObjectPtr& obj, const SpaceObjectReference&id) {
    std::tr1::unordered_map<ObjectReference,std::set<uint32> >::iterator where=mQueryMap.find(id.object());
    if (where!=mQueryMap.end()) {
        if (where->second.find(query_id)!=where->second.end()) {
            where->second.erase(query_id);
            if (where->second.empty()) {
                mQueryMap.erase(where);
                destroyObject(obj,mQueryTracker);
            }
        }else {
            SILOG(space,error,"Trying to erase query "<<query_id<<" for object "<<id<<" query_id invalid");
        }
    }else {
        SILOG(space,error,"Trying to erase query for object "<<id<<" not in query set");
    }
}
void SpaceProxyManager::clearQuery(uint32 query_id){
    std::vector<ObjectReference>mfd;
    for (std::tr1::unordered_map<ObjectReference,std::set<uint32> >::iterator i=mQueryMap.begin(),
             ie=mQueryMap.end();
         i!=ie;
         ++i) {
        if (i->second.find(query_id)!=i->second.end()) {
            mfd.push_back(i->first);
        }
    }
    {
        for (std::vector<ObjectReference>::iterator i=mfd.begin(),ie=mfd.end();i!=ie;++i) {
            SpaceObjectReference id(mSpace->id(),*i);
            removeQueryInterest(query_id,getProxyObject(id),id);
        }
    }
}
bool SpaceProxyManager::isLocal(const SpaceObjectReference&id) const{
    return false;
}
ProxyManager* SpaceProxyManager::getProxyManager(const SpaceID&space) {
    if (mSpace->id()==space) return this;
    return NULL;
}
QueryTracker* SpaceProxyManager::getTracker(const SpaceID& space) {
    return mQueryTracker;
}

void SpaceProxyManager::initialize() {

}
void SpaceProxyManager::destroy() {
  for (ProxyMap::iterator iter = mProxyMap.begin();
         iter != mProxyMap.end();
         ++iter) {
        iter->second->destroy(mSpace->now());
    }
    mProxyMap.clear();
}
ProxyObjectPtr SpaceProxyManager::getProxyObject(const Sirikata::SpaceObjectReference&id) const{
    ProxyMap::const_iterator where=mProxyMap.find(id.object());
    if (where!=mProxyMap.end()) {
        if(id.space()==mSpace->id()){
            return where->second;
        }
    }
    return ProxyObjectPtr();
}
QueryTracker * SpaceProxyManager::getQueryTracker(const SpaceObjectReference&id) {
    return mQueryTracker;
}


// ODP::Service Interface

ODP::Port* SpaceProxyManager::bindODPPort(SpaceID space, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(space, port);
}

ODP::Port* SpaceProxyManager::bindODPPort(SpaceID space) {
    return mDelegateODPService->bindODPPort(space);
}

void SpaceProxyManager::registerDefaultODPHandler(const ODP::MessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

ODP::DelegatePort* SpaceProxyManager::createDelegateODPPort(ODP::DelegateService* parentService, SpaceID space, ODP::PortID port) {
    if (space != mSpace->id())
        throw ODP::PortAllocationException("SpaceProxyManager::createDelegateODPPort can't allocate port because the specified space does not match the ProxyManager's space.");

    ODP::Endpoint port_ep(space, ObjectReference::spaceServiceID(), port);
    return new ODP::DelegatePort(
        mDelegateODPService,
        port_ep,
        std::tr1::bind(
            &SpaceProxyManager::delegateODPPortSend, this,
            port_ep, _1, _2
        )
    );
}

bool SpaceProxyManager::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload) {
    assert(source_ep.space() == mSpace->id());
    assert(dest_ep.space() == mSpace->id());
    assert(source_ep.object() == ObjectReference::spaceServiceID());

    RoutableMessageHeader hdr;
    hdr.set_source_space( source_ep.space() );
    hdr.set_source_object( source_ep.object() );
    hdr.set_source_port( source_ep.port() );
    hdr.set_destination_space( dest_ep.space() );
    hdr.set_destination_object( dest_ep.object() );
    hdr.set_destination_port( dest_ep.port() );

    // FIXME either this should be able to accurately indicate whether the data
    // was accepted or this class shouldn't be implementing this interface
    // (which it is currently forced to via VWObject).  I'm guessing the latter
    // is the correct solution.
    mSpace->processMessage(hdr, payload);
    return true;
}

}
