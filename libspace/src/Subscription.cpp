/*  Sirikata libspace -- Suscription/Broadcast Services
 *  Subscription.cpp
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

#include "Protocol_Subscription.pbj.hpp"
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/space/Subscription.hpp>
namespace Sirikata {


Subscriber::Subscriber(const ObjectReference &objref, Broadcast *bcast)
    : mObject(objref),
      mBroadcast(bcast),
      mDuration(Duration::zero()),
      mNextUpdate(Time::null()),
      mTimerIsActive(false)
{
    mTimer = Network::IOTimer::create(
        mBroadcast->getSubscriptionService()->getIOService(),
        std::tr1::bind(&Subscriber::timerCallback, this));
}

void Subscriber::timerCallback() {
    mTimerIsActive = false;
    Time now = Time::local();

    std::string serializedMessage;
    mBroadcast->makeBroadcastMessage(&serializedMessage);
    MemoryReference memref(serializedMessage.data(), serializedMessage.length());

    send(now, memref);
}

void Subscriber::doSend(MemoryReference body) {
    RoutableMessageHeader destination_header;
    destination_header.set_source_object(ObjectReference::spaceServiceID());
    destination_header.set_source_port(Services::BROADCAST);
    destination_header.set_destination_object(mObject);

    mBroadcast->getSubscriptionService()->sendMessage(destination_header,body);
}

Subscriber::~Subscriber() {
}

void Subscriber::updateDuration(Time now, Duration newDuration) {
    mDuration = newDuration;
    if (!canSendImmediately(now)) {
        mNextUpdate = now + mDuration;
    }
    if (isTimerActive()) {
        mTimer->wait(mDuration);
    }
}

void Subscriber::send(Time now, MemoryReference body) {
    if (canSendImmediately(now)) {
        doSend(body);
        mNextUpdate = now + mDuration;
    } else if (!isTimerActive()) {
        mTimer->wait(mNextUpdate-now);
        mTimerIsActive = true;
    }
}

void Subscriber::cancelTimer() {
    mTimer->cancel();
    mTimerIsActive = false;
}

Broadcast::Broadcast(Subscription *sub, const SubscriptionID &id)
    : mID(id), mSubscriptionService(sub) {
}

Broadcast::~Broadcast() {
}

void Broadcast::subscribe(const ObjectReference &subscriber, Duration period) {
    mSubscribers.insert(SubscriberMap::value_type(
        subscriber, new Subscriber(subscriber, this)));
}

void Broadcast::makeBroadcastMessage(std::string *serializedMsg) {
    Protocol::Broadcast msg;
    msg.set_broadcast_name(getNumber());
    msg.set_data(mMessage);
    msg.SerializeToString(serializedMsg);
}

void Broadcast::broadcast(const std::string &message) {
    mMessage = message;
    Time now = Time::local();
    std::string serializedMessage;
    makeBroadcastMessage(&serializedMessage);
    MemoryReference memref(serializedMessage.data(), serializedMessage.length());
    for (SubscriberMap::const_iterator iter = mSubscribers.begin();
         iter != mSubscribers.end();
         ++iter)
    {
        Subscriber *sub = iter->second;
        sub->send(now, memref);
    }
}

Subscription::Subscription(Network::IOService *ioServ)
    : mSubscription(this),
      mBroadcastFromObject(this, false),
      mBroadcastFromSpace(this, true) {
    mIOService = ioServ;
}

Subscription::~Subscription() {
}

void Subscription::processMessage(const ObjectReference&src, const Protocol::Subscribe&sub){
    ObjectReference object_reference(sub.object());
    SubscriptionID broadcast (object_reference, sub.broadcast_name());
    Duration period = sub.update_period();
    BroadcastMap::iterator iter = mBroadcasts.find(broadcast);
    if (iter == mBroadcasts.end()) {
        iter = mBroadcasts.insert(BroadcastMap::value_type(
            broadcast,
            new Broadcast(this, broadcast))).first;
    }
    iter->second->subscribe(object_reference, period);
}

void Subscription::processMessage(const ObjectReference&src, bool isTrusted, const Protocol::Broadcast&sub){
    ObjectReference object_reference(sub.object());
    if (ObjectReference(sub.object()) != src && !isTrusted) {
        return; // Src object and broadcast object do not match
    }
    SubscriptionID broadcast (ObjectReference(sub.object()), sub.broadcast_name());
    BroadcastMap::iterator iter = mBroadcasts.find(broadcast);
    if (iter == mBroadcasts.end() &&
            (sub.broadcast_name()>=0 || isTrusted)) {
        iter = mBroadcasts.insert(BroadcastMap::value_type(
            broadcast,
            new Broadcast(this, broadcast))).first;
    }
    if (sub.has_data()) {
        iter->second->broadcast(sub.data());
    }
}
void Subscription::SubscriptionService::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    Protocol::Subscribe subMsg;
    subMsg.ParseFromArray(message_body.data(),message_body.size());
    mParent->processMessage(header.source_object(), subMsg);
}

void Subscription::BroadcastService::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    if (header.reply_id()) {
        // ACK'ed
        return;
    }
    Protocol::Broadcast broadMsg;
    broadMsg.ParseFromArray(message_body.data(),message_body.size());
    mParent->processMessage(header.source_object(), mTrusted, broadMsg);
}
void Subscription::SendService::processMessage(const RoutableMessageHeader&header, MemoryReference message_body) {
    for (size_t i = 0; i < mServices.size(); ++i) {
        mServices[i]->processMessage(header, message_body);
    }
}
bool Subscription::SendService::forwardMessagesTo(MessageService*ms) {
    mServices.push_back(ms);
    return true;
}
bool Subscription::SendService::endForwardingMessagesTo(MessageService*ms) {
    std::vector<MessageService*>::iterator where=std::find(mServices.begin(),mServices.end(),ms);
    if (where==mServices.end())
        return false;
    mServices.erase(where);
    return true;
}

}
