/*  Sirikata libspace -- Subscription services
 *  Subscription.hpp
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

#ifndef _SIRIKATA_SUBSCRIPTION_HPP_
#define _SIRIKATA_SUBSCRIPTION_HPP_

#include <space/Platform.hpp>
#include <util/ObjectReference.hpp>
#include <util/Time.hpp>
#include <network/IOTimer.hpp>

namespace Sirikata {

namespace SubscriptionPBJ {
namespace Protocol {
class Subscribe;
class Broadcast;
}
}

namespace Protocol {
using namespace ::Sirikata::SubscriptionPBJ::Protocol;
}

class Subscriber;
class Broadcast;
class Subscription;

typedef int32 subint;

typedef std::pair<ObjectReference, subint> SubscriptionID;
class SubscriptionIDHasher {
    ObjectReference::Hasher orHash;
public:
    size_t operator() (const SubscriptionID &toHash) const {
        return orHash(toHash.first) ^ std::tr1::hash<subint>()(toHash.second);
    }
};

class Subscriber {
    const ObjectReference mObject;
    Broadcast *const mBroadcast;
    Duration mDuration;
    Time mNextUpdate;
    Network::IOTimerPtr mTimer;
    bool mTimerIsActive;
private:
    void timerCallback();
    void doSend(MemoryReference messageContents);
public:
    Subscriber(const ObjectReference &objref, Broadcast *bcast);
    ~Subscriber();
    void updateDuration(Time now, Duration newDuration);
    void cancelTimer();
    void send(Time now, MemoryReference messageContents);
    bool canSendImmediately(Time now) const {
        return mNextUpdate < now;
    }
    bool isTimerActive() const {
        return mTimerIsActive;
    }
};

class Broadcast {
    typedef std::tr1::unordered_map<ObjectReference, Subscriber*, ObjectReference::Hasher> SubscriberMap;

    SubscriberMap mSubscribers;
    SubscriptionID mID;
    std::string mMessage;
    Subscription *mSubscriptionService;
public:
    Broadcast(Subscription *subService, const SubscriptionID &id);
    ~Broadcast();
    void subscribe(const ObjectReference &subscriber, Duration period);
    void broadcast(const std::string &message);
    void makeBroadcastMessage(std::string *serializedMsg);
    const ObjectReference &owner() {return mID.first;}
    subint getNumber() const { return mID.second; }
    Subscription *getSubscriptionService() {
        return mSubscriptionService;
    }
};

class SIRIKATA_SPACE_EXPORT Subscription {
    class SubscriptionService : public MessageService {
        Subscription *mParent;
    public:
        bool forwardMessagesTo(MessageService*) { return false; }
        bool endForwardingMessagesTo(MessageService*) { return false; }
        void processMessage(const RoutableMessageHeader&header,
                            MemoryReference message_body);
        SubscriptionService (Subscription *parent) {
            mParent = parent;
        }
    } mSubscription;
    class BroadcastService : public MessageService {
        Subscription *mParent;
        bool mTrusted;
    public:
        bool forwardMessagesTo(MessageService*) { return false; }
        bool endForwardingMessagesTo(MessageService*) { return false; }
        void processMessage(const RoutableMessageHeader&header,
                            MemoryReference message_body);
        BroadcastService (Subscription *parent, bool trusted) {
            mParent = parent;
            mTrusted = trusted;
        }
    } mBroadcastFromObject, mBroadcastFromSpace;
    class SendService : public MessageService {
        std::vector<MessageService*> mServices;
    public:
        bool forwardMessagesTo(MessageService*);
        bool endForwardingMessagesTo(MessageService*);
        void processMessage(const RoutableMessageHeader&header,
                            MemoryReference message_body);
    } mSendService;
    typedef std::tr1::unordered_map<SubscriptionID, Broadcast*, SubscriptionIDHasher> BroadcastMap;

    BroadcastMap mBroadcasts;
    Network::IOService *mIOService;
private:
    void processMessage(const ObjectReference&object_reference,const Protocol::Subscribe&loc);
    void processMessage(const ObjectReference&object_reference,bool isTrusted,const Protocol::Broadcast&loc);
public:
    Subscription(Network::IOService *ioServ);
    ~Subscription();

    inline void sendMessage(
            const RoutableMessageHeader&header,
            MemoryReference message_body) {
        getSendService()->processMessage(header, message_body);
    }

    Network::IOService *getIOService() {
        return mIOService;
    }

    MessageService *getSubscriptionService() {
        return &mSubscription;
    }
    MessageService *getBroadcastService() {
        return &mBroadcastFromObject;
    }
    MessageService *getSpaceBroadcastService() {
        return &mBroadcastFromSpace;
    }
    MessageService *getSendService() {
        return &mSendService;
    }

}; // class Space

} // namespace Sirikata

#endif //_SIRIKATA_LOC_HPP
