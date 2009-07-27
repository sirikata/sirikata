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

namespace CBR {

float64 MaxDistUpdatePredicate::maxDist = 0.0;

Object::Object(const OriginID& origin_id, const UUID& id, ObjectMessageQueue* obj_msg_q, MotionPath* motion, float prox_radius)
 : mID(id),
   mGlobalIntroductions(false),
   mMotion(motion),
   mLocation(mMotion->initial()),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mOriginID(origin_id),
   mObjectMessageQueue(obj_msg_q),
   mProximityRadius(prox_radius)
{
}

Object::Object(const OriginID& origin_id, const UUID& id, ObjectMessageQueue* obj_msg_q, MotionPath* motion, float prox_radius, const std::set<UUID>& objects)
 : mID(id),
   mGlobalIntroductions(true),
   mMotion(motion),
   mLocation(mMotion->initial()),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mOriginID(origin_id),
   mObjectMessageQueue(obj_msg_q),
   mProximityRadius(prox_radius)
{
    mSubscribers = objects;
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

void Object::checkPositionUpdate(const Time& t) {
    const TimedMotionVector3f* update = mMotion->nextUpdate(mLocation.time());
    while(update != NULL && update->time() <= t) {
        mLocation = *update;
        update = mMotion->nextUpdate(mLocation.time());
    }

    if (mLocationExtrapolator.needsUpdate(t, mLocation.extrapolate(t))) {
        mLocationExtrapolator.updateValue(mLocation.time(), mLocation.value());
        for(ObjectSet::iterator it = mSubscribers.begin(); it != mSubscribers.end(); it++) {
            LocationMessage* loc_msg =
                new LocationMessage(
                    mOriginID,
                    mID,
                    *it,
                    mLocation
                );
            bool success = mObjectMessageQueue->send(loc_msg);
            // XXX FIXME do something on failure
            delete loc_msg;
        }
    }
}

void Object::locationMessage(LocationMessage* loc_msg) {
    // FIXME do something with the data

    delete loc_msg;
}

void Object::proximityMessage(ProximityMessage* prox_msg) {
    if (mGlobalIntroductions) { // we already know about everybody, so we don't need to handle this
        delete prox_msg;
        return;
    }

    SubscriptionMessage* subs_msg = NULL;
    if (prox_msg->event() == ProximityMessage::Entered)
        subs_msg = new SubscriptionMessage(mOriginID, uuid(), prox_msg->neighbor(), SubscriptionMessage::Subscribe);
    else
        subs_msg = new SubscriptionMessage(mOriginID, uuid(), prox_msg->neighbor(), SubscriptionMessage::Unsubscribe);

    mObjectMessageQueue->send(subs_msg);
    delete subs_msg;

    delete prox_msg;
}

void Object::subscriptionMessage(SubscriptionMessage* subs_msg) {
    if (subs_msg->action() == SubscriptionMessage::Subscribe)
        addSubscriber(subs_msg->sourceObject());
    else
        removeSubscriber(subs_msg->sourceObject());

    delete subs_msg;
}

void Object::migrateMessage(MigrateMessage* migrate_msg) {
    mID = migrate_msg->object();
    mProximityRadius = migrate_msg->proximityRadius();

    for (int i = 0; i < migrate_msg->subscriberCount(); i++) {
      addSubscriber(migrate_msg->subscriberList()[i]);

      //printf("recvd migrateMsg->msubscribers[i] = %s\n",
      //     migrate_msg->subscriberList()[i].readableHexData().c_str()  );
    }
}

} // namespace CBR
