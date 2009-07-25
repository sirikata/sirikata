/*  Sirikata liboh -- Object Host
 *  TopLevelSpaceConnection.cpp
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
#include <oh/Platform.hpp>
#include <util/SpaceID.hpp>
#include <network/TCPStream.hpp>
#include "oh/SpaceConnection.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/SpaceIDMap.hpp"
#include "oh/ObjectHost.hpp"
namespace Sirikata {
namespace {
void connectionStatus(const std::tr1::weak_ptr<TopLevelSpaceConnection>&weak_thus,Network::Stream::ConnectionStatus status,const std::string&reason){
    std::tr1::shared_ptr<TopLevelSpaceConnection>thus=weak_thus.lock();
    if (thus) {
        if (status!=Network::Stream::Connected) {//won't get this error until lookup returns
            thus->remoteDisconnection(reason);
        }
    }
}
}

TopLevelSpaceConnection::TopLevelSpaceConnection(Network::IOService*io):mRegisteredAddress(Network::Address::null()) {
    mParent=NULL;
    mTopLevelStream=new Network::TCPStream(*io);
    ObjectHostProxyManager::initialize();
}
void TopLevelSpaceConnection::connect(const std::tr1::weak_ptr<TopLevelSpaceConnection>&thus, ObjectHost * oh,  const SpaceID & id) {
    mSpaceID=id;
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    mTopLevelStream->prepareOutboundConnection(&Network::Stream::ignoreSubstreamCallback,
                                               std::tr1::bind(&connectionStatus, thus,_1,_2),                                               
                                               &Network::Stream::ignoreBytesReceived);
    oh->spaceIDMap()->lookup(id,std::tr1::bind(&TopLevelSpaceConnection::connectToAddress,thus,oh,_1));
}


void TopLevelSpaceConnection::connect(const std::tr1::weak_ptr<TopLevelSpaceConnection>&thus, ObjectHost * oh,  const SpaceID & id, const Network::Address&addy) {
    mSpaceID=id;
    mParent=oh;
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    mTopLevelStream->connect(addy,
                             &Network::Stream::ignoreSubstreamCallback,
                             std::tr1::bind(&connectionStatus, thus,_1,_2),                                               
                             &Network::Stream::ignoreBytesReceived);
}

void TopLevelSpaceConnection::removeFromMap() {
    mParent->removeTopLevelSpaceConnection(mSpaceID,mRegisteredAddress,this);
}
void TopLevelSpaceConnection::remoteDisconnection(const std::string&reason) {
    if (mParent) {
        removeFromMap();//FIXME: is it possible for an object host to connect at exactly this time
        //maybe resolution is to connect nowhere?
        Network::Stream *topLevel=mTopLevelStream;
        mTopLevelStream=NULL;
        topLevel->connect(Network::Address("0.0.0.0","0"));
        delete topLevel;
    }
}
void TopLevelSpaceConnection::connectToAddress(const std::tr1::weak_ptr<TopLevelSpaceConnection>&weak_thus,ObjectHost*oh,const Network::Address*addy) {
    std::tr1::shared_ptr<TopLevelSpaceConnection>thus=weak_thus.lock();
    if (thus) {
        thus->mParent=oh;
        if (addy) {
            thus->mTopLevelStream->connect(*addy);
            oh->insertAddressMapping(*addy,thus);
        }else {
            thus->mTopLevelStream->connect(Network::Address("0.0.0.0","0"));
        }
    }
}
TopLevelSpaceConnection::~TopLevelSpaceConnection() {
    ObjectHostProxyManager::destroy();
    if (mParent) {
        removeFromMap();
        delete mTopLevelStream;
    }
}

void ObjectHostProxyManager::initialize() {
}
void ObjectHostProxyManager::destroy() {
    for (ProxyMap::iterator iter = mProxyMap.begin();
         iter != mProxyMap.end();
         ++iter) {
        iter->second.obj->destroy();
    }
    mProxyMap.clear();
}
ObjectHostProxyManager::~ObjectHostProxyManager() {
	destroy();
}
void ObjectHostProxyManager::createObject(const ProxyObjectPtr &newObj) {
    std::pair<ProxyMap::iterator, bool> result = mProxyMap.insert(
        ProxyMap::value_type(newObj->getObjectReference().object(), newObj));
    if (result.second==true) {
        notify(&ProxyCreationListener::createProxy,newObj);
    }
    ++result.first->second.refCount;
}
void ObjectHostProxyManager::destroyObject(const ProxyObjectPtr &newObj) {
    ProxyMap::iterator iter = mProxyMap.find(newObj->getObjectReference().object());
    if (iter != mProxyMap.end()) {
        if ((--iter->second.refCount)==0) {
            newObj->destroy();
            notify(&ProxyCreationListener::destroyProxy,newObj);
            mProxyMap.erase(iter);
        }
    }
}

void ObjectHostProxyManager::createObjectProximity(
        const ProxyObjectPtr &newObj,
        const UUID &seeker,
        uint32 queryId)
{
    createObject(newObj);
    QueryMap::iterator iter =
        mQueryMap.insert(QueryMap::value_type(
                             std::pair<UUID, uint32>(seeker, queryId),
                             std::set<ProxyObjectPtr>())).first;
    bool success = iter->second.insert(newObj).second;
    assert(success == true);
}

void ObjectHostProxyManager::destroyObjectProximity(
        const ProxyObjectPtr &newObj,
        const UUID &seeker,
        uint32 queryId)
{
    destroyObject(newObj);
    QueryMap::iterator iter =
        mQueryMap.find(std::pair<UUID, uint32>(seeker, queryId));
    if (iter != mQueryMap.end()) {
        std::set<ProxyObjectPtr>::iterator proxyiter = iter->second.find(newObj);
        assert (proxyiter != iter->second.end());
        if (proxyiter != iter->second.end()) {
            iter->second.erase(proxyiter);
        }
    }
}

void ObjectHostProxyManager::destroyProximityQuery(
        const UUID &seeker,
        uint32 queryId)
{
    QueryMap::iterator iter =
        mQueryMap.find(std::pair<UUID, uint32>(seeker, queryId));
    if (iter != mQueryMap.end()) {
        std::set<ProxyObjectPtr> &set = iter->second;
        std::set<ProxyObjectPtr>::const_iterator proxyiter;
        for (proxyiter = set.begin(); proxyiter != set.end(); ++proxyiter) {
            destroyObject(*proxyiter);
        }
        mQueryMap.erase(iter);
    }
}


ProxyObjectPtr ObjectHostProxyManager::getProxyObject(const SpaceObjectReference &id) const {
    if (id.space() == mSpaceID) {
        ProxyMap::const_iterator iter = mProxyMap.find(id.object());
        if (iter != mProxyMap.end()) {
            return (*iter).second.obj;
        }
    }
    return ProxyObjectPtr();
}


}
