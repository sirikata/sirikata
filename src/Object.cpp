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

Object::Object(const ServerID& origin_id, ObjectFactory* parent, const UUID& id, ObjectMessageQueue* obj_msg_q, MotionPath* motion, SolidAngle queryAngle)
 : mID(id),
   mGlobalIntroductions(false),
   mMotion(motion),
   mLocation(mMotion->initial()),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mOriginID(origin_id),
   mObjectMessageQueue(obj_msg_q),
   mQueryAngle(queryAngle),
   mParent(parent)
{
}

Object::Object(const ServerID& origin_id, ObjectFactory* parent, const UUID& id, ObjectMessageQueue* obj_msg_q, MotionPath* motion, SolidAngle queryAngle, const std::set<UUID>& objects)
 : mID(id),
   mGlobalIntroductions(true),
   mMotion(motion),
   mLocation(mMotion->initial()),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mOriginID(origin_id),
   mObjectMessageQueue(obj_msg_q),
   mQueryAngle(queryAngle),
   mParent(parent)
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

void Object::locationMessage(const UUID& src, const TimedMotionVector3f& loc) {
    // FIXME do something with the data
}

void Object::proximityMessage(const CBR::Protocol::Object::ObjectMessage& msg, const CBR::Protocol::Prox::ProximityResults& prox_results) {
    if (mGlobalIntroductions) { // we already know about everybody, so we don't need to handle this
        return;
    }

    for(uint32 idx = 0; idx < prox_results.addition_size(); idx++) {
        CBR::Protocol::Prox::IObjectAddition addition = prox_results.addition(idx);

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
    for(uint32 idx = 0; idx < prox_results.removal_size(); idx++) {
        CBR::Protocol::Prox::IObjectRemoval removal = prox_results.removal(idx);

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

void Object::subscriptionMessage(const UUID& subscriber, bool subscribing) {
    if (subscribing)
        addSubscriber(subscriber);
    else
        removeSubscriber(subscriber);
}

void Object::migrateMessage(const UUID& oid, const SolidAngle& sa, const std::vector<UUID> subs) {
    assert(mID == oid);
    assert(mQueryAngle == sa);

    for (uint32 i = 0; i < subs.size(); i++) {
      addSubscriber(subs[i]);
    }
}

} // namespace CBR
