/*  Sirikata Subscription Library
 *  SubscriptionState.cpp
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

#include <subscription/Platform.hpp>

#include "network/Stream.hpp"
#include "network/StreamListener.hpp"
#include "util/UUID.hpp"
#include "Subscription_Subscription.pbj.hpp"
#include "subscription/Server.hpp"
#include "subscription/SubscriptionState.hpp"



namespace Sirikata { namespace Subscription {
SubscriptionState::SubscriptionState(Network::Stream*broadcaster):mName(UUID::null()),mLatestSentTime(Task::LocalTime::null()),mLatestUnsentTime(Task::LocalTime::null()){
    mEpoch=ReservedEpoch;
    mBroadcaster=broadcaster;
    mEverReceivedMessage=false;
    mPolling=false;
}
void SubscriptionState::clearLastSentMessage() {
	mEverReceivedMessage=true;
	mLastSentMessage.resize(0);
}

void SubscriptionState::setLastSentMessage(Network::Chunk&chunk) {
	mEverReceivedMessage=true;
	mLastSentMessage.swap(chunk);
}


Task::LocalTime SubscriptionState::pushSubscriber(Subscriber*subscriber,std::vector<SubscriberTimePair>*heap) {
    Task::LocalTime retval=subscriber->computeNextUpdateFromNow();
    heap->push_back(SubscriberTimePair(retval,subscriber));
    std::push_heap(heap->begin(),heap->end());
    return retval;
}

void SubscriptionState::pushNewSubscriber(Subscriber*s,const Task::LocalTime&oldTime) {
    mUnsentSubscribersHeap.push_back(SubscriberTimePair(oldTime,s));
    std::push_heap(mUnsentSubscribersHeap.begin(),mUnsentSubscribersHeap.end());
    if (mLatestUnsentTime<oldTime) {
        mLatestUnsentTime=oldTime;
    }
}
void SubscriptionState::pushJustReceivedSubscriber(Subscriber*s) {
    Task::LocalTime retval=pushSubscriber(s,&mSentSubscribersHeap);
    if (mLatestSentTime<retval) {
        mLatestSentTime=retval;
    }
}
void SubscriptionState::registerSubscriber(const std::tr1::shared_ptr<Network::Stream>&stream, const Protocol::Subscribe&subscriptionMessage){
    Subscriber*subscriber =new Subscriber(stream,subscriptionMessage);
    if (mEverReceivedMessage)
        subscriber->broadcast(MemoryReference(mLastSentMessage));
    pushJustReceivedSubscriber(subscriber);
}


void SubscriptionState::broadcast(Server*poll,const MemoryReference&data){
    Task::LocalTime now=Task::LocalTime::now();
    std::vector<SubscriberTimePair> newUnsenders;
    if (mLatestSentTime<now||mLatestUnsentTime<now) {//there exists a completing queue
        Task::LocalTime latestSentTime=mLatestSentTime;
        bool unsentSwap=false;
        if (!(mLatestSentTime<now)) {//if the queue of those items that were marked as having sent the last packet isn't fully ready to send
            mLatestSentTime=now;//make sure to reset the time for things in the sent queue
            mSentSubscribersHeap.swap(mUnsentSubscribersHeap);//swap it with that queue that is... since by the end of this operation all items that haven't sent this latest packet won't
                                                              //have sent the latest packet and can be placed in the unsent heap
            unsentSwap=true;//swap the latest times
        }
        if (mLatestSentTime<now&&mLatestUnsentTime<now) {//if both queues are going to empty, combine them as the sent queue
            mSentSubscribersHeap.insert(mSentSubscribersHeap.end(),mUnsentSubscribersHeap.begin(),mUnsentSubscribersHeap.end());
            mUnsentSubscribersHeap.clear();
        }else if (!mUnsentSubscribersHeap.empty()) {//otherwise pop items off the Unsent queue onto the sent queue for mass processing
            if (unsentSwap)
                mLatestUnsentTime=latestSentTime;
            while(mUnsentSubscribersHeap.front().mNextUpdateTime<now) {
                mSentSubscribersHeap.push_back(mUnsentSubscribersHeap.front());
                std::pop_heap(mUnsentSubscribersHeap.begin(),mUnsentSubscribersHeap.end());
                mUnsentSubscribersHeap.pop_back();
            }
        }
        std::vector<SubscriberTimePair>::iterator i,ie=mSentSubscribersHeap.end();
        for (i=mSentSubscribersHeap.begin(); i!=ie;++i) {
            i->mSubscriber->broadcast(data);
            i->mNextUpdateTime=i->mSubscriber->computeNextUpdateFromNow();
            if (mLatestSentTime<i->mNextUpdateTime) {
                mLatestSentTime=i->mNextUpdateTime;
            }
        }
        std::make_heap(mSentSubscribersHeap.begin(),mSentSubscribersHeap.end());//since the sent queue's heap property was violated, reheapify
    }else {
        //neither queue completes, meaning both will have partial entries that will need to be merged into a heap
        std::vector<SubscriberTimePair> temp,sent;
        sent.swap(mSentSubscribersHeap);
        mLatestSentTime=now;
        std::vector<SubscriberTimePair>::iterator unsentHeapEnd=mUnsentSubscribersHeap.end(),sentEnd=sent.end(),iter;
        size_t unsentSize=mUnsentSubscribersHeap.size();
        for (size_t i=0;i<unsentSize;++i) {
            if (mUnsentSubscribersHeap.front().mNextUpdateTime<now) {
                mUnsentSubscribersHeap.front().mSubscriber->broadcast(data);
                std::pop_heap(mUnsentSubscribersHeap.begin(),unsentHeapEnd);
                --unsentHeapEnd;
                pushJustReceivedSubscriber(unsentHeapEnd->mSubscriber);
            }else {
                if (unsentHeapEnd!=mUnsentSubscribersHeap.end())
                    mUnsentSubscribersHeap.erase(unsentHeapEnd,mUnsentSubscribersHeap.end());
                break;
            }
        }
        size_t sentSize=sent.size();
        for (size_t i=0;i<sentSize;++i) {
            if (sent.front().mNextUpdateTime<now) {
                sent.front().mSubscriber->broadcast(data);
                std::pop_heap(sent.begin(),sentEnd);
                --sentEnd;
                pushJustReceivedSubscriber(sentEnd->mSubscriber);
            }else {
                for (iter=sent.begin();iter!=sentEnd;++iter) {
                    pushNewSubscriber(iter->mSubscriber,iter->mNextUpdateTime);
                }
                break;
            }
        }
    }
    if (!mUnsentSubscribersHeap.empty()) {
        poll->initiatePolling(mName,mUnsentSubscribersHeap.front().mNextUpdateTime-now);
    }

}

SubscriptionState::~SubscriptionState(){
    std::vector<SubscriberTimePair>::iterator i=mUnsentSubscribersHeap.begin(),ie=mUnsentSubscribersHeap.end();
    for (;i!=ie;++i) {
        delete i->mSubscriber;
    }
    i=mSentSubscribersHeap.begin();
    ie=mSentSubscribersHeap.end();
    for (;i!=ie;++i) {
        delete i->mSubscriber;
    }
    delete mBroadcaster;
}

SubscriptionState::Subscriber::Subscriber(const std::tr1::shared_ptr<Network::Stream>&sender,const Protocol::Subscribe&msg):mSender(sender),mPeriod(msg.has_update_period()?msg.update_period():Duration::microseconds(0)) {
    mSentEpoch=ReservedEpoch;
}
void SubscriptionState::Subscriber::broadcast(const MemoryReference&data){
    std::tr1::shared_ptr<Network::Stream> sender=mSender.lock();
    if (sender) {
        sender->send(data,Network::ReliableOrdered);
    }
}
void SubscriptionState::poll(Server*parent) {
    Task::LocalTime now=Task::LocalTime::now();
    while (!mUnsentSubscribersHeap.empty()) {
        SubscriberTimePair* iter=&mUnsentSubscribersHeap.front();
        if (mUnsentSubscribersHeap.front().mNextUpdateTime<now) {
            iter->mSubscriber->broadcast(MemoryReference(mLastSentMessage));
            pushJustReceivedSubscriber(iter->mSubscriber);
            std::pop_heap(mUnsentSubscribersHeap.begin(),mUnsentSubscribersHeap.end());
            mUnsentSubscribersHeap.pop_back();
        }else {
            assert(mName!=UUID::null());
            parent->initiatePolling(mName,mUnsentSubscribersHeap.front().mNextUpdateTime-now);
            break;
        }
    }
    if (mUnsentSubscribersHeap.empty()) {
        mPolling=false;
    }
}

Task::LocalTime SubscriptionState::Subscriber::computeNextUpdateFromNow(){
    return Task::LocalTime::now()+mPeriod;
}




} }
