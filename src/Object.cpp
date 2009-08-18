/*  cbr
 *  Object.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "Object.hpp"
#include "Message.hpp"
#include "ObjectMessageQueue.hpp"
#include "Random.hpp"
#include "ObjectFactory.hpp"

namespace CBR {

float64 MaxDistUpdatePredicate::maxDist = 0.0;

Object::Object(const ServerID& origin_id, ObjectFactory* parent, const UUID& id, ObjectMessageQueue* obj_msg_q, MotionPath* motion, SolidAngle queryAngle, Trace* trace)
 : mID(id),
   mGlobalIntroductions(false),
   mMotion(motion),
   mLocation(mMotion->initial()),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mOriginID(origin_id),
   mObjectMessageQueue(obj_msg_q),
   mQueryAngle(queryAngle),
   mParent(parent),
   mTrace(trace),
   mLastTime(Time::null())
{
}

Object::Object(const ServerID& origin_id, ObjectFactory* parent, const UUID& id, ObjectMessageQueue* obj_msg_q, MotionPath* motion, SolidAngle queryAngle, Trace* trace, const std::set<UUID>& objects)
 : mID(id),
   mGlobalIntroductions(true),
   mMotion(motion),
   mLocation(mMotion->initial()),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mOriginID(origin_id),
   mObjectMessageQueue(obj_msg_q),
   mQueryAngle(queryAngle),
   mParent(parent),
   mTrace(trace),
   mLastTime(Time::null())
{
    mSubscribers = objects;
}

Object::~Object() {
    mParent->notifyDestroyed(mID);
}

void Object::addSubscriber(const UUID& sub) {
    if (mSubscribers.find(sub) == mSubscribers.end())
        mSubscribers.insert(sub);
}

void Object::removeSubscriber(const UUID& sub) {
    ObjectSet::iterator it = mSubscribers.find(sub);
    if (it != mSubscribers.end())
        mSubscribers.erase(it);
}

void Object::tick(const Time& t) {
    checkPositionUpdate(t);
    mLastTime = t;
}

const TimedMotionVector3f Object::location() const {
    return mLocation;
}

void Object::checkPositionUpdate(const Time& t) {
    const TimedMotionVector3f* update = mMotion->nextUpdate(mLocation.time());
    while(update != NULL && update->time() <= t) {
        mLocation = *update;
        update = mMotion->nextUpdate(mLocation.time());
    }

    if (mLocationExtrapolator.needsUpdate(t, mLocation.extrapolate(t))) {
        mLocationExtrapolator.updateValue(mLocation.time(), mLocation.value());
        for(ObjectSet::iterator it = mSubscribers.begin(); it != mSubscribers.end(); it++) {
            CBR::Protocol::Loc::TimedMotionVector loc;
            loc.set_t(mLocation.updateTime());
            loc.set_position(mLocation.position());
            loc.set_velocity(mLocation.velocity());

            CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
            obj_msg->set_source_object(uuid());
            obj_msg->set_source_port(OBJECT_PORT_LOCATION);
            obj_msg->set_dest_object(*it);
            obj_msg->set_dest_port(OBJECT_PORT_LOCATION);
            obj_msg->set_unique(GenerateUniqueID(mOriginID));
            obj_msg->set_payload( serializePBJMessage(loc) );

            bool success = mObjectMessageQueue->send(obj_msg);
            // XXX FIXME do something on failure
        }
    }
}

void Object::receiveMessage(const CBR::Protocol::Object::ObjectMessage* msg) {
    assert( msg->dest_object() == uuid() );

    switch( msg->dest_port() ) {
      case OBJECT_PORT_PROXIMITY:
        proximityMessage(*msg);
        break;
      case OBJECT_PORT_LOCATION:
        locationMessage(*msg);
        break;
      case OBJECT_PORT_SUBSCRIPTION:
        subscriptionMessage(*msg);
        break;
      default:
        printf("Warning: Tried to deliver unknown message type through ObjectConnection.\n");
        break;
    }

    delete msg;
}

void Object::locationMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    CBR::Protocol::Loc::TimedMotionVector contents;
    bool parse_success = contents.ParseFromString(msg.payload());
    assert(parse_success);

    TimedMotionVector3f loc(contents.t(), MotionVector3f(contents.position(), contents.velocity()));

    mTrace->loc(
        mLastTime,
        msg.dest_object(),
        msg.source_object(),
        loc
    );

    // FIXME do something with the data
}

void Object::proximityMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    assert(msg.source_object() == UUID::null()); // Should originate at space server
    CBR::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(msg.payload());
    assert(parse_success);

    for(uint32 idx = 0; idx < contents.addition_size(); idx++) {
        CBR::Protocol::Prox::IObjectAddition addition = contents.addition(idx);

        TimedMotionVector3f loc(addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()));

        mTrace->prox(
            mLastTime,
            msg.dest_object(),
            addition.object(),
            true,
            loc
        );

        if (!mGlobalIntroductions) {
            CBR::Protocol::Subscription::SubscriptionMessage subs;
            subs.set_action(CBR::Protocol::Subscription::SubscriptionMessage::Subscribe);

            CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
            obj_msg->set_source_object(uuid());
            obj_msg->set_source_port(OBJECT_PORT_SUBSCRIPTION);
            obj_msg->set_dest_object(addition.object());
            obj_msg->set_dest_port(OBJECT_PORT_SUBSCRIPTION);
            obj_msg->set_unique(GenerateUniqueID(mOriginID));
            obj_msg->set_payload( serializePBJMessage(subs) );

            mObjectMessageQueue->send(obj_msg);
        }
    }
    for(uint32 idx = 0; idx < contents.removal_size(); idx++) {
        CBR::Protocol::Prox::IObjectRemoval removal = contents.removal(idx);

        mTrace->prox(
            mLastTime,
            msg.dest_object(),
            removal.object(),
            false,
            TimedMotionVector3f()
        );

        if (!mGlobalIntroductions) {
            CBR::Protocol::Subscription::SubscriptionMessage subs;
            subs.set_action(CBR::Protocol::Subscription::SubscriptionMessage::Unsubscribe);

            CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
            obj_msg->set_source_object(uuid());
            obj_msg->set_source_port(OBJECT_PORT_SUBSCRIPTION);
            obj_msg->set_dest_object(removal.object());
            obj_msg->set_dest_port(OBJECT_PORT_SUBSCRIPTION);
            obj_msg->set_unique(GenerateUniqueID(mOriginID));
            obj_msg->set_payload( serializePBJMessage(subs) );

            mObjectMessageQueue->send(obj_msg);
        }
    }
}

void Object::subscriptionMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    CBR::Protocol::Subscription::SubscriptionMessage contents;
    bool parse_success = contents.ParseFromString(msg.payload());
    assert(parse_success);

    mTrace->subscription(
        mLastTime,
        msg.dest_object(),
        msg.source_object(),
        (contents.action() == CBR::Protocol::Subscription::SubscriptionMessage::Subscribe) ? true : false
    );

    if ( contents.action() == CBR::Protocol::Subscription::SubscriptionMessage::Subscribe )
        addSubscriber(msg.source_object());
    else
        removeSubscriber(msg.source_object());
}

void Object::migrateMessage(const UUID& oid, const SolidAngle& sa, const std::vector<UUID> subs) {
    assert(mID == oid);
    assert(mQueryAngle == sa);

    for (uint32 i = 0; i < subs.size(); i++) {
      addSubscriber(subs[i]);
    }
}

} // namespace CBR
