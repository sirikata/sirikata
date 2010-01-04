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
    UUID mID;
    const ObjectReference mOwner;
    std::string mMessage;
    Subscription *mSubscriptionService;
public:
    Broadcast(Subscription *subService, const UUID &uuid, const ObjectReference &owner);
    ~Broadcast();
    void subscribe(const ObjectReference &subscriber, Duration period);
    void broadcast(const std::string &message);
    void makeBroadcastMessage(std::string *serializedMsg);
    const ObjectReference &owner() {return mOwner;}
    Subscription *getSubscriptionService() {
        return mSubscriptionService;
    }
};

class SIRIKATA_SPACE_EXPORT Subscription {
    struct SubscriptionService : public MessageService {
        Subscription *mParent;
        bool forwardMessagesTo(MessageService*);
        bool endForwardingMessagesTo(MessageService*);
        void processMessage(const RoutableMessageHeader&header,
                            MemoryReference message_body);
        void init (Subscription *parent) { mParent = parent; }
    } mSubscription;
    struct BroadcastService : public MessageService {
        Subscription *mParent;
        std::vector<MessageService*> mServices;
        bool forwardMessagesTo(MessageService*);
        bool endForwardingMessagesTo(MessageService*);
        void processMessage(const RoutableMessageHeader&header,
                            MemoryReference message_body);
        void init (Subscription *parent) { mParent = parent; }
    } mBroadcast;
    typedef std::tr1::unordered_map<UUID, Broadcast*, UUID::Hasher> BroadcastMap;

    BroadcastMap mBroadcasts;
    Network::IOService *mIOService;
private:
    void processMessage(const ObjectReference&object_reference,const Protocol::Subscribe&loc);
    void processMessage(const ObjectReference&object_reference,const Protocol::Broadcast&loc);
public:
    Subscription(Network::IOService *ioServ);
    ~Subscription();
    void sendMessage(const RoutableMessageHeader&header,
                        MemoryReference message_body);

    Network::IOService *getIOService() {
        return mIOService;
    }

    MessageService *getSubscriptionService() {
        return &mSubscription;
    }
    MessageService *getBroadcastService() {
        return &mBroadcast;
    }

}; // class Space

} // namespace Sirikata

#endif //_SIRIKATA_LOC_HPP
