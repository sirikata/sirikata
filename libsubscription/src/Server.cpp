/*  Sirikata Subscription and Broadcasting System -- Subscription Services
 *  Server.hpp
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
#include <subscription/Platform.hpp>

#include "network/TCPStream.hpp"
#include "network/TCPStreamListener.hpp"
#include "network/IOServiceFactory.hpp"
#include "util/UUID.hpp"
#include "subscription/Server.hpp"
#include "subscription/SubscriptionState.hpp"
#include <boost/thread.hpp>
using namespace Sirikata::Network;
namespace Sirikata { namespace Subscription {

class Server::UniqueLock :public boost::mutex{
    
};
Server::Server(Network::IOService*broadcastIOService,Network::StreamListener*broadcastListener, const Network::Address&broadcastAddress, Network::StreamListener*subscriberListener, const Network::Address&subscriberAddress){
    mBroadcastIOService=broadcastIOService;
    mBroadcastersSubscriptionsLock=new UniqueLock;
    mBroadcastListener=broadcastListener;
    if (!broadcastListener->listen(broadcastAddress,std::tr1::bind(&Server::broadcastStreamCallback,this,_1,_2))) {
        SILOG(subscription,error,"Error listening to broadcast on port "<<broadcastAddress.getHostName()<<':'<<broadcastAddress.getService());
    }
    
    mSubscriberListener=subscriberListener;
    if (!subscriberListener->listen(subscriberAddress,std::tr1::bind(&Server::subscriberStreamCallback,this,_1,_2))) {
        SILOG(subscription,error,"Error listening to broadcast on port "<<broadcastAddress.getHostName()<<':'<<broadcastAddress.getService());
    }
}
    
void Server::subscriberStreamCallback(Network::Stream*newStream,Network::Stream::SetCallbacks&cb){
    std::tr1::shared_ptr<Stream> newStreamPtr(newStream);
    std::tr1::weak_ptr<Stream> weakNewStreamPtr(newStreamPtr);
    std::tr1::shared_ptr<std::tr1::shared_ptr<Stream> >indirectedStream(new std::tr1::shared_ptr<Stream>(newStreamPtr));
    cb(std::tr1::bind(&Server::subscriberConnectionCallback,indirectedStream,_1,_2),
       std::tr1::bind(&Server::subscriberBytesReceivedCallback,this,indirectedStream,_1));
}
void Server::subscriberBytesReceivedCallback(const std::tr1::shared_ptr<std::tr1::shared_ptr<Network::Stream> >&stream,const Network::Chunk&dat){
    Protocol::Subscribe subscriptionRequest;
    bool success=false;
    if (!dat.empty()&&subscriptionRequest.ParseFromArray(&dat[0],dat.size())&&subscriptionRequest.has_broadcast_name()) {
        Network::IOServiceFactory::
            dispatchServiceMessage(mBroadcastIOService,
                                   std::tr1::bind(&Server::subscriberBytesReceivedCallbackOnBroadcastIOService,
                                                  this,
                                                  stream,
                                                  subscriptionRequest));
    }else {
        std::tr1::shared_ptr<Stream> strongStream(*stream);
        if(strongStream) {
            strongStream->close();
        }
        *stream=std::tr1::shared_ptr<Stream>();
    }
}
void Server::subscriberBytesReceivedCallbackOnBroadcastIOService(const std::tr1::shared_ptr<std::tr1::shared_ptr<Network::Stream> >&stream,const Protocol::Subscribe&subscriptionRequest){
    bool success=false;
    {
        boost::lock_guard<boost::mutex>lok(*mBroadcastersSubscriptionsLock);
        std::tr1::unordered_map<UUID,SubscriptionState*,UUID::Hasher>::iterator where
            =mSubscriptions.find(subscriptionRequest.broadcast_name());
        if (where!=mSubscriptions.end()) {
            if (*stream) {
                where->second->registerSubscriber(*stream,subscriptionRequest);
                success=true;
            }
        }
    }
    if (success) {
        std::tr1::shared_ptr<Stream> strongStream(*stream);
        if(strongStream) {
            strongStream->close();
        }
        *stream=std::tr1::shared_ptr<Stream>();
    }
}
void Server::subscriberConnectionCallback(const std::tr1::shared_ptr<std::tr1::shared_ptr<Network::Stream> >&stream,Network::Stream::ConnectionStatus status,const std::string&reason){
    if (status!=Stream::Connected) {
        std::tr1::shared_ptr<Stream> strongStream(*stream);
        if(strongStream) {
            strongStream->close();
        }
        *stream=std::tr1::shared_ptr<Stream>();
    }
}
void Server::broadcastConnectionCallback(SubscriptionState*subscription,Network::Stream::ConnectionStatus status,const std::string&reason){
    if (status!=Stream::Connected) {
        boost::lock_guard<boost::mutex>lok(*mBroadcastersSubscriptionsLock);
        std::tr1::unordered_map<UUID,SubscriptionState*,UUID::Hasher>::iterator where;
        std::tr1::unordered_map<SubscriptionState*,UUID>::iterator whichuuid;
        whichuuid=mBroadcasters.find(subscription);
        if (whichuuid!=mBroadcasters.end()) {
            where=mSubscriptions.find(whichuuid->second);
            mBroadcasters.erase(whichuuid);
            if (where!=mSubscriptions.end()) {
                mSubscriptions.erase(where);
            }
        }
        delete subscription;
    }
}
void Server::broadcastBytesReceivedCallback(SubscriptionState*state, const Network::Chunk&chunk) {
    if (state->mEpoch==SubscriptionState::ReservedEpoch) {
        Protocol::Broadcast broadcastRegistration;
        if (!chunk.empty()&&broadcastRegistration.ParseFromArray(&chunk[0],chunk.size())&&broadcastRegistration.has_broadcast_name()) {
            ++state->mEpoch;
            boost::lock_guard<boost::mutex>lok(*mBroadcastersSubscriptionsLock);
            std::tr1::unordered_map<SubscriptionState*,UUID>::iterator whichuuid;
            whichuuid=mBroadcasters.find(state);
            if (whichuuid==mBroadcasters.end()) {
                if (mSubscriptions.find(broadcastRegistration.broadcast_name())==mSubscriptions.end()) {
                    mSubscriptions[broadcastRegistration.broadcast_name()]=state;
                    mBroadcasters[state]=broadcastRegistration.broadcast_name();
                    state->setUUID(broadcastRegistration.broadcast_name());
                }else {
                    SILOG(subscription,warning,"Duplicate UUID for broadcast "<<broadcastRegistration.broadcast_name().toString()<<" Already in map");
                }
            }else {
                SILOG(subscription,error,"UUID "<<whichuuid->second.toString()<<" Already in map");
            }
        }
    }else {
        state->broadcast(this,MemoryReference(chunk));
    }
}
void Server::broadcastStreamCallback(Network::Stream* stream,Network::Stream::SetCallbacks&cb) {
    SubscriptionState*state=new SubscriptionState(stream);
    cb(std::tr1::bind(&Server::broadcastConnectionCallback,this,state,_1,_2),
       std::tr1::bind(&Server::broadcastBytesReceivedCallback,this,state,_1));
}

void Server::initiatePolling(const UUID&name, const Duration&waitTime) {
    std::tr1::weak_ptr<Server>thus=shared_from_this();
    Network::IOServiceFactory::
        dispatchServiceMessage(mBroadcastIOService,
                               waitTime,
                               std::tr1::bind(&poll,
                                              thus,
                                              name));
                                           
}
void Server::poll(const std::tr1::weak_ptr<Server> &weak_thus, const UUID&name) {
    std::tr1::shared_ptr<Server>thus=weak_thus.lock();
    if (thus) {
        boost::lock_guard<boost::mutex>lok(*thus->mBroadcastersSubscriptionsLock);
        std::tr1::unordered_map<UUID,SubscriptionState*,UUID::Hasher>::iterator where=thus->mSubscriptions.find(name);

        if (where!=thus->mSubscriptions.end()) {
            where->second->poll(&*thus);
        }
    }
}


} }

