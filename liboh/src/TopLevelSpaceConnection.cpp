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
#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/SpaceID.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/oh/SpaceConnection.hpp>
#include <sirikata/oh/TopLevelSpaceConnection.hpp>
#include <sirikata/oh/SpaceIDMap.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include "Protocol_Time.pbj.hpp"
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/network/TimeSyncImpl.hpp>
#include <sirikata/oh/SpaceTimeOffsetManager.hpp>
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

TopLevelSpaceConnection::TopLevelSpaceConnection(Network::IOService*io, const String&protocol, OptionSet *protocolOptions):mRegisteredAddress(Network::Address::null()) {
    mParent=NULL;
    mIOService=io;
    mTopLevelStream=Network::StreamFactory::getSingleton().getConstructor(protocol)(io,protocolOptions);
    ObjectHostProxyManager::initialize();
}
void TopLevelSpaceConnection::connect(const std::tr1::weak_ptr<TopLevelSpaceConnection>&thus, ObjectHost * oh,  const SpaceID & id) {
    using std::tr1::placeholders::_1;

    mSpaceID=id;
    mParent=oh;

    oh->spaceIDMap()->lookup(id, std::tr1::bind(&TopLevelSpaceConnection::connectToAddress, thus, oh, id, _1));
}

void TopLevelSpaceConnection::connect(const std::tr1::weak_ptr<TopLevelSpaceConnection>&thus, ObjectHost * oh,  const SpaceID & id, const Network::Address&addy) {
    using std::tr1::placeholders::_1;

    mSpaceID=id;
    mParent=oh;

    connectToAddress(thus, oh, id, &addy);
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
        std::tr1::weak_ptr<Network::TimeSync> weakTimeSync(mTimeSync);
        mTimeSync=std::tr1::shared_ptr<Network::TimeSync>();
        while (weakTimeSync.lock()) {
        }
        if (topLevel) {
            topLevel->close();
        }
        delete topLevel;
    }
}
void TopLevelSpaceConnection::connectToAddress(const std::tr1::weak_ptr<TopLevelSpaceConnection>&weak_thus, ObjectHost*oh, const SpaceID& id, const Network::Address*addy) {
    std::tr1::shared_ptr<TopLevelSpaceConnection>thus=weak_thus.lock();
    if (thus) {
        thus->mParent=oh;
        if (addy) {
            using std::tr1::placeholders::_1;
            using std::tr1::placeholders::_2;

            std::tr1::shared_ptr<Network::TimeSyncImpl<std::tr1::weak_ptr<TopLevelSpaceConnection> > > sync(new Network::TimeSyncImpl<std::tr1::weak_ptr<TopLevelSpaceConnection> >(weak_thus,thus->mIOService));
            std::tr1::weak_ptr<Network::TimeSyncImpl<std::tr1::weak_ptr<TopLevelSpaceConnection> > > weak_sync(sync);
            thus->mTimeSync = sync;

            thus->mTopLevelStream->connect(
                *addy,
                &Network::Stream::ignoreSubstreamCallback,
                std::tr1::bind(&connectionStatus, weak_thus, _1, _2),
                std::tr1::bind(&Network::TimeSyncImpl<std::tr1::weak_ptr<TopLevelSpaceConnection> >::bytesReceived, weak_sync, _1, _2),
                &Network::Stream::ignoreReadySendCallback
            );

            sync->setCallback(std::tr1::bind(&SpaceTimeOffsetManager::setSpaceTimeOffset,id,_1));
            sync->go(sync,3,6,Duration::seconds(10),thus->mTopLevelStream);

            oh->insertAddressMapping(*addy,thus);
        }else {
            thus->mTopLevelStream->close();
        }
    }
}
TopLevelSpaceConnection::~TopLevelSpaceConnection() {
    std::tr1::weak_ptr<Network::TimeSync> weakTimeSync(mTimeSync);
    mTimeSync=std::tr1::shared_ptr<Network::TimeSync>();
    while (weakTimeSync.lock())
        SILOG(objecthost,warning,"Weak Time Sync holding onto resources. Waiting until unacquired");
    ObjectHostProxyManager::destroy();
    if (mParent) {
        removeFromMap();
        delete mTopLevelStream;
    }
}

void TopLevelSpaceConnection::registerHostedObject(const ObjectReference &mRef, const HostedObjectPtr &hostedObj) {
    mHostedObjects.insert(HostedObjectMap::value_type(mRef, hostedObj));
}
void TopLevelSpaceConnection::unregisterHostedObject(const ObjectReference &mRef) {
    HostedObjectMap::iterator iter = mHostedObjects.find(mRef);
    assert (iter != mHostedObjects.end());
    if (iter != mHostedObjects.end()) {
        mHostedObjects.erase(iter);
    }
}
HostedObjectPtr TopLevelSpaceConnection::getHostedObject(const ObjectReference &mref) const {
    HostedObjectMap::const_iterator iter = mHostedObjects.find(mref);
    if (iter != mHostedObjects.end()) {
        HostedObjectPtr obj(iter->second.lock());
        return obj;
    }
    return HostedObjectPtr();
}

}
