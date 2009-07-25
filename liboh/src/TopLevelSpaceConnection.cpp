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

}
