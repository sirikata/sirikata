/*  Sirikata Subscription and Broadcasting System -- Subscription Services
 *  SubscriptionState.hpp
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
#ifndef _SIRIKATA_SUBSCRIPTION_STATE_HPP_
#define _SIRIKATA_SUBSCRIPTION_STATE_HPP_
#include "util/Time.hpp"
namespace Sirikata { namespace Subscription {
class Server;

class SubscriptionState :public Noncopyable{
    friend class Server;
    typedef uint16 EpochType;
public:
    enum {
        LastEpoch=65535,
        ///On a Subscriber this epoch means that the subscriber has not received any recent updates, on a Broadcaster it means uninitialized
        ReservedEpoch=0
    };
private:
    class Subscriber {
        friend class SubscriptionState;
        std::tr1::weak_ptr<Network::Stream>mSender;
        Duration mPeriod;
        EpochType mSentEpoch;
    public:
        ///registers self with parent and finds appropriate parentOffset from array size
        Subscriber(const std::tr1::shared_ptr<Network::Stream>&sender,const Protocol::Subscribe&);
        ///broadcasts the message to the mSender
        void broadcast(const MemoryReference&);
        Time computeNextUpdateFromNow();
    };
    class SubscriberTimePair {
        friend class SubscriptionState;
      public:
        Time mNextUpdateTime;
        Subscriber* mSubscriber;
        SubscriberTimePair(const Time&nextUpdate, Subscriber*subscriber):mNextUpdateTime(nextUpdate) {
            mSubscriber=subscriber;
        }
        bool operator <(const SubscriberTimePair&other)const {
            return mNextUpdateTime<other.mNextUpdateTime;
        }
        bool operator ==(const SubscriberTimePair&other) const{
            return mNextUpdateTime==other.mNextUpdateTime;
        }
        
    };
    UUID mName;
    Network::Stream*mBroadcaster;
    Time mLatestSentTime;
    Time mLatestUnsentTime;
    EpochType mEpoch;
    bool mEverReceivedMessage;
    bool mPolling;
    Network::Chunk mLastSentMessage;
    ///this is the heap of subscribers who opted out of the last message
    std::vector<SubscriberTimePair>mUnsentSubscribersHeap;
    ///this is the heap of subscribers that received the last message
    std::vector<SubscriberTimePair>mSentSubscribersHeap;
    ///does the heap management for sent or unsent subscribers based on the passed vector
    Time pushSubscriber(Subscriber*,std::vector<SubscriberTimePair>*);
    ///Push a subscriber who missed a broadcast into the mUnsentSubscribersHeap
    void pushNewSubscriber(Subscriber*,const Time&);
    ///Push a Subscriber who has just heard a broadcast into the mSentSubscribersHeap
    void pushJustReceivedSubscriber(Subscriber*);
public:
    void setUUID(const UUID&name){mName=name;}
    ///Creates a new subscription state class for a given named subscription associated with a given network stream
    SubscriptionState(Network::Stream*broadcaster);
    ///register a new Stream to get updates who subscribed with the given protocol message. Must be called from IOServiceThread of *broadcaster*, instead of new subscriber
    void registerSubscriber(const std::tr1::shared_ptr<Network::Stream>&, const Protocol::Subscribe&);
    ///Take a memory reference and broadcast it to all interested parties who are within a receive window. Also schedules a poll if no other polls are present to retry for unsent subscribers
    void broadcast(Server*parent, const MemoryReference&);
    ///Check if any of the subscribers who had not received the last message when it was broadcast are able to receive it by now. If there are still unsent subscribers then ask the server to schedule a second polling
    void poll(Server*parent);
    ~SubscriptionState();
};
} }

#endif
