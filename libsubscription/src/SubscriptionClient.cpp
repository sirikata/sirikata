/*  Sirikata Subscription Library
 *  SubscriptionClient.cpp
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
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH package.
 */

#include "subscription/Platform.hpp"
#include "subscription/SubscriptionClient.hpp"
#include "network/IOServiceFactory.hpp"
#include "network/TCPStream.hpp"
#include "Subscription_Subscription.pbj.hpp"
#include <boost/thread.hpp>
namespace Sirikata { namespace Subscription {
class SubscriptionClient::UniqueLock : public boost::mutex {

};

void SubscriptionClient::upgradeFromIOThread(const std::tr1::weak_ptr<State>&weak_source,
                                             const std::tr1::weak_ptr<State>&weak_dest) {
    std::tr1::shared_ptr<State> dest=weak_dest.lock();
    std::tr1::shared_ptr<State> source=weak_source.lock();
    if (dest&&source) {//make sure both source and dest are available
        for (std::vector<std::tr1::weak_ptr<IndividualSubscription> >::iterator i=source->mSubscribers.begin(),ie=source->mSubscribers.end();
             i!=ie;
             ++i) {//for each item in the list
            std::tr1::shared_ptr<IndividualSubscription> individual=i->lock();//if that item is still alive
            if (individual) {
                individual->mSubscriptionState=dest;//reparent the item
                dest->mSubscribers.push_back(individual);//add the item to the parents queue
            }
        }
        source->mSubscribers.clear();//get it off this queue in case there are pending subscriptions for this defunct subscriptions.
    }
}


void SubscriptionClient::upgrade(const std::tr1::weak_ptr<State>&source,
                                 const std::tr1::weak_ptr<State>&dest) {
    Network::IOServiceFactory::
        dispatchServiceMessage(mService,
                               std::tr1::bind(&SubscriptionClient::upgradeFromIOThread,
                                              this,
                                              source,
                                              dest));
}


void SubscriptionClient::removeSubscriberHint(const Network::Address&address,
                                 const UUID&uuid) {
    Network::IOServiceFactory::
        dispatchServiceMessage(mService,
                               std::tr1::bind(&SubscriptionClient::removeSubscriberFromIOThreadHint,
                                              this,
                                              address,
                                              uuid));
}
void SubscriptionClient::addSubscriberFromIOThread(const std::tr1::weak_ptr<IndividualSubscription>&individual, bool sendIntroMessage){
    std::tr1::shared_ptr<IndividualSubscription>i=individual.lock();
    if (i) {
        i->mFunction(i->mSubscriptionState->mLastDeliveredMessage);//blank state indicates that the stream is connected and future updates will be received
        i->mSubscriptionState->mSubscribers.push_back(individual);//add the subscriber to the list of subscribers in the IO thread rather than the main thread to avoid locks
    }
}
void SubscriptionClient::removeSubscriberFromIOThreadHint(Network::Address address,
                                                          const UUID&uuid){
    boost::lock_guard<boost::mutex>lok(*static_cast<boost::mutex*>(mMapLock));
    BroadcastMap::iterator where=mBroadcasts.find(AddressUUID(address,uuid));
    if (where!=mBroadcasts.end()) {
        std::tr1::shared_ptr<State> thus=where->second.lock();
        if (thus) {
            thus->purgeSubscribersFromIOThread(where->second,this);
        }
    }
}
void SubscriptionClient::addSubscriber(const std::tr1::weak_ptr<IndividualSubscription>&individual, bool sendIntroMessage){
    Network::IOServiceFactory::
        dispatchServiceMessage(mService,
                               std::tr1::bind(&SubscriptionClient::addSubscriberFromIOThread,
                                              this,
                                              individual,
                                              sendIntroMessage));
}
SubscriptionClient::SubscriptionClient(Network::IOService*service):mService(service){
    mMapLock = new UniqueLock;
}
SubscriptionClient::~SubscriptionClient(){
    delete mMapLock;
}
SubscriptionClient::State::State(const Duration &period,
                                 const Network::Address&address,
                                 const UUID&uuid,
                                 SubscriptionClient *parent):mAddress(address),mUUID(uuid),mPeriod(period){
    mParent=parent;
}
void SubscriptionClient::State::setStream(const std::tr1::shared_ptr<State> thus,
                      const std::tr1::shared_ptr<Network::Stream>topLevelStream,
                      const String& serializedStream){
    //set the toplevel stream
    thus->mTopLevelStream=topLevelStream;

    std::tr1::weak_ptr<State> weak_thus(thus);
    //clone the toplevel stream and bind weak_ptr functions from the state to callback
    std::tr1::shared_ptr<Network::Stream> 
        clonedStream(topLevelStream->clone(std::tr1::bind(&State::connectionCallback,
                                                          weak_thus,
                                                          _1,
                                                          _2),
                                           std::tr1::bind(&State::bytesReceived,
                                                          weak_thus,
                                                          _1)));
    thus->mStream=clonedStream;
    if (serializedStream.length()) {//if the message has any substance, send it
        thus->mStream->send(MemoryReference(serializedStream),Network::ReliableUnordered);
    }
}

void SubscriptionClient::State::purgeSubscribersFromIOThread(const std::tr1::weak_ptr<State>&weak_thus, SubscriptionClient *parent) {
    //the minimum period in our set of live listeners
    Duration period(Duration::microseconds(0));
    // an example live listener, without which the above period is meaningless
    std::tr1::shared_ptr<IndividualSubscription> tooGoodForMe;
    
    //start at the end
    size_t i=mSubscribers.size();
    
    while(i>0) {
        --i;//and work down
        std::tr1::shared_ptr<IndividualSubscription> individual=mSubscribers[i].lock();
        if (!individual) {//if the individual is not alive
            if (i+1<mSubscribers.size())//and the individual is not at the back
                mSubscribers[i]=mSubscribers.back();//put the back item here
            mSubscribers.pop_back();//and pop the back item
        }else {
            if (!tooGoodForMe) {//if this individual is the first alive
                tooGoodForMe=individual;//mark it as first alive
                period=individual->mPeriod;//set the minimum recorded shortest period
            }else if (individual->mPeriod<period) {//if this individual wants a shorter period than the minimum recorded or it is the first
                period=individual->mPeriod;//set the minimum recorded shortest period
            }
        }
    }
    if (tooGoodForMe&&mPeriod<period) {//need to resubscribe to get less frequent updates
        while (mSubscribers.size()&&!(tooGoodForMe=mSubscribers.back().lock())) {//if the back one died in our sleep
            mSubscribers.pop_back();//well start popping until one is alive: this should not happen
            assert(0&&"Back subscriber died even though it was *locked*");
        }
        if (tooGoodForMe){
            mSubscribers.pop_back();//pop the back guy--he will be the guinea pig in our upgrade process
            {
                boost::lock_guard<boost::mutex>lok(*parent->mMapLock);//lock the map
                BroadcastMap::iterator where=parent->mBroadcasts.find(AddressUUID(mAddress,mUUID));
                if (where!=parent->mBroadcasts.end()) {
                    parent->mBroadcasts.erase(where);//purge the map of our entry
                }
            }
            Protocol::Subscribe subscription;//make a subscription packet
            subscription.set_broadcast_name(mUUID);//populate it with fields from this
            subscription.set_update_period(period);//and with the desired slower period
            //FIXME any more properties? we don't have a way to determine to edit the code if there are
            parent->subscribe(mAddress,//call subscribe
                              subscription,
                              tooGoodForMe->mFunction,
                              tooGoodForMe->mDisconFunction,
                              String(),
                              tooGoodForMe);//pass the example individual in, so a new one is not created
            parent->upgradeFromIOThread(weak_thus,//work an upgrade path to shift everyone into the new subscription state
                                        tooGoodForMe->mSubscriptionState);
        }
    }
}
namespace {
size_t sMaximumSubscriptionStateSize=1360;
}
void SubscriptionClient::State::bytesReceived(const std::tr1::weak_ptr<State>&weak_thus,
                          const Network::Chunk&data) {
    std::tr1::shared_ptr<State> thus=weak_thus.lock();
    if (thus) {
        bool eraseAny=false;
        for (std::vector<std::tr1::weak_ptr<IndividualSubscription> >::iterator i
                     =thus->mSubscribers.begin(),ie=thus->mSubscribers.end();
             i!=ie;
             ++i) {
            std::tr1::shared_ptr<IndividualSubscription> temp=i->lock();
            if (temp) {//lock each guy...if true send datae
                temp->mFunction(data);
            }else {//if false, get ready for a round of purge
                eraseAny=true;
            }
        }
        if (eraseAny) {
            thus->purgeSubscribersFromIOThread(weak_thus,thus->mParent);
        }
        if (data.size()<sMaximumSubscriptionStateSize) {
            thus->mLastDeliveredMessage=data;
        }else {
            thus->mLastDeliveredMessage.resize(0);
        }
    }
}

void SubscriptionClient::State::connectionCallback(const std::tr1::weak_ptr<State>&weak_thus,
                               const Network::Stream::ConnectionStatus status,
                               const String&reason) {
    if (status!=Network::Stream::Connected) {
        std::tr1::shared_ptr<State> thus=weak_thus.lock();
        if (thus) {
            for (std::vector<std::tr1::weak_ptr<IndividualSubscription> >::iterator i
                     =thus->mSubscribers.begin(),ie=thus->mSubscribers.end();
                 i!=ie;
                 ++i) {
                std::tr1::shared_ptr<IndividualSubscription> temp=i->lock();
                if (temp) {
                    temp->mDisconFunction();
                }
            }
            thus->mSubscribers.clear();
            thus->mStream=std::tr1::shared_ptr<Network::Stream>();
            thus->mTopLevelStream=std::tr1::shared_ptr<Network::Stream>();
        }
    }
}

std::tr1::shared_ptr<SubscriptionClient::IndividualSubscription>
   SubscriptionClient::subscribe(const Network::Address& address,
                                 const Protocol::Subscribe&subscription,
                                 const std::tr1::function<void(const Network::Chunk&)>&cb,
                                 const std::tr1::function<void()>&disconCb,
                                 const String&serializedSubscription,
                                 std::tr1::shared_ptr<IndividualSubscription> newSubscription){
    bool do_upgrade=false;
    std::tr1::weak_ptr<State> upgrade_source;
    std::tr1::weak_ptr<State> upgrade_dest;
    std::tr1::shared_ptr<IndividualSubscription> retval;
    String localSerializedSubscription;
    if (serializedSubscription.length()==0)//serialize out if necessary
        subscription.SerializeToString(&localSerializedSubscription);
    if (!newSubscription) {//if there was no example newSubscription passed in, make one using the data in the Protocol::Subscribe message
        std::tr1::shared_ptr<IndividualSubscription> newerSubscription(
            new IndividualSubscription(subscription.update_period(),cb,disconCb));
        newSubscription=newerSubscription;
    }
    {//lock guard
        boost::lock_guard<boost::mutex>lok(*mMapLock);
        AddressUUID key(address,subscription.broadcast_name());
        BroadcastMap::iterator where=mBroadcasts.find(key);
        TopLevelStreamMap::iterator topLevelStreamIter;
        if (where!=mBroadcasts.end()){//if the specific broadcast can be found
            std::tr1::shared_ptr<State>state=where->second.lock();
            if (state) {
                if (subscription.update_period()<state->mPeriod) {//if the period needs to be upgraded
                    do_upgrade=true;//set do update flag
                    upgrade_source=where->second;//set the source upgrade, but do not set retval--this will force us to fallthrough to other stream creation mechanisms
                }else{
                    newSubscription->mSubscriptionState=state;//otherwise use the existing stream even if it's at a higher rate
                    addSubscriber(newSubscription,true);//and add a subscriber to it, waiting for the appropriate thread to arrive
                    retval=newSubscription;//set retval: we're done
                }
            }
        }
        //fallback to existing toplevel stream
        if ((!retval)&&(topLevelStreamIter=mTopLevelStreams.find(address))!=mTopLevelStreams.end()) {
            std::tr1::shared_ptr<Network::Stream> topLevelStreamPtr;
            if ((topLevelStreamPtr=topLevelStreamIter->second.lock())) {//make sure toplevel stream is there
                std::tr1::shared_ptr<State> state(new State(
                                                      subscription.update_period(),
                                                      address,
                                                      subscription.broadcast_name(),this));//setup state

                state->setStream(state,topLevelStreamPtr,serializedSubscription.length()?serializedSubscription:localSerializedSubscription);//set state to use a given toplevel stream and serialize
                                                                                                                           //out a broadcast join request

                //put new broadcast into the broadcasts lists
                mBroadcasts.insert(BroadcastMap::value_type(key,state));
                newSubscription->mSubscriptionState=state;
                retval=newSubscription;
                addSubscriber(newSubscription,false);
            }else {
                mTopLevelStreams.erase(topLevelStreamIter);
            }
        }
        if (!retval) {//make a new connection
            std::tr1::shared_ptr<Network::Stream> topLevelStream (new Network::TCPStream(*mService));

            topLevelStream->connect(address,
                                    &Network::Stream::ignoreSubstreamCallback,
                                    &Network::Stream::ignoreConnectionStatus,
                                    &Network::Stream::ignoreBytesReceived);
            std::tr1::shared_ptr<State> state(new State(subscription.update_period(),
                                                                  address,
                                                                  subscription.broadcast_name(),
                                                                  this));
            if (do_upgrade) {
                upgrade_dest=state;
            }
            state->setStream(state,topLevelStream,serializedSubscription.length()?serializedSubscription:localSerializedSubscription);
            newSubscription->mSubscriptionState=state;
            state->mSubscribers.push_back(newSubscription);
            mTopLevelStreams.insert(TopLevelStreamMap::value_type(address,topLevelStream));
            mBroadcasts.insert(BroadcastMap::value_type(key,state));
            retval=newSubscription;
        }
    }//unlock map lock
    if (do_upgrade) {
        upgrade(upgrade_source,upgrade_dest);//coalesce those with lower retval into broadcast with higher retval
    }
    return retval;
}
std::tr1::shared_ptr<SubscriptionClient::IndividualSubscription>
    SubscriptionClient::subscribe(const Network::Address& address,
                                  const String&subscription,
                                  const std::tr1::function<void(const Network::Chunk&)>&cb,
                                  const std::tr1::function<void()>&disconCb){
    Protocol::Subscribe s;
    if (s.ParseFromString(subscription)) {
        return subscribe(address,
                         s,
                         cb,
                         disconCb,
                         subscription);
    }
    return std::tr1::shared_ptr<SubscriptionClient::IndividualSubscription>();
}


} }
