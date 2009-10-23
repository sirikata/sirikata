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
#include "Random.hpp"
#include "ObjectFactory.hpp"
#include "Statistics.hpp"
#include <boost/bind.hpp>

#define OBJ_LOG(level,msg) SILOG(object,level,"[OBJ] " << msg)

namespace CBR {

float64 MaxDistUpdatePredicate::maxDist = 3.0;

Object::Object(const UUID& id, MotionPath* motion, const BoundingSphere3f& bnds, bool regQuery, SolidAngle queryAngle, const ObjectHostContext* ctx)
 : mID(id),
   mContext(ctx),
   mGlobalIntroductions(false),
   mMotion(motion),
   mBounds(bnds),
   mLocation(mMotion->initial()),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mRegisterQuery(regQuery),
   mQueryAngle(queryAngle),
   mConnectedTo(0),
   mMigrating(false)
{
}

Object::~Object() {
    mContext->objectFactory->notifyDestroyed(mID);
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

void Object::tick() {
    if (connected() && !mMigrating)
        checkPositionUpdate();
}

const TimedMotionVector3f Object::location() const {
    return mLocation;
}

void Object::connect() {
    assert(!connected());

    TimedMotionVector3f curMotion = mMotion->at(mContext->time);

    if (mRegisterQuery)
        mContext->objectHost->openConnection(
            this,
            curMotion,
            mBounds,
            mQueryAngle,
            boost::bind(&Object::handleSpaceConnection, this, _1)
        );
    else
        mContext->objectHost->openConnection(
            this,
            curMotion,
            mBounds,
            boost::bind(&Object::handleSpaceConnection, this, _1)
        );
}

void Object::handleSpaceConnection(ServerID sid) {
    if (sid == 0) {
        OBJ_LOG(error,"Failed to open connection for object " << mID.toString());
        return;
    }

    OBJ_LOG(insane,"Got space connection callback");
    mConnectedTo = sid;

    TimedMotionVector3f curMotion = mMotion->at(mContext->time);
}

bool Object::connected() {
    return (mConnectedTo != NullServerID);
}

void Object::checkPositionUpdate() {
    const Time& t = mContext->time;

    const TimedMotionVector3f* update = mMotion->nextUpdate(mLocation.time());
    while(update != NULL && update->time() <= t) {
        mLocation = *update;
        update = mMotion->nextUpdate(mLocation.time());
    }

    if (mLocationExtrapolator.needsUpdate(t, mLocation.extrapolate(t))) {
        mLocationExtrapolator.updateValue(mLocation.time(), mLocation.value());

        // Generate and send an update to Loc
        CBR::Protocol::Loc::Container container;
        CBR::Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
        CBR::Protocol::Loc::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t(mLocation.updateTime());
        requested_loc.set_position(mLocation.position());
        requested_loc.set_velocity(mLocation.velocity());
        bool success = mContext->objectHost->send(
            t,
            this, OBJECT_PORT_LOCATION,
            UUID::null(), OBJECT_PORT_LOCATION,
            serializePBJMessage(container)
        );
        // XXX FIXME do something on failure
        mContext->trace->objectGenLoc(t, mID, mLocation);
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
    assert(msg.source_object() == UUID::null()); // Should come from space

    CBR::Protocol::Loc::BulkLocationUpdate contents;
    bool parse_success = contents.ParseFromString(msg.payload());
    assert(parse_success);

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        CBR::Protocol::Loc::LocationUpdate update = contents.update(idx);

        CBR::Protocol::Loc::TimedMotionVector update_loc = update.location();
        TimedMotionVector3f loc(update_loc.t(), MotionVector3f(update_loc.position(), update_loc.velocity()));

        mContext->trace->objectLoc(
            mContext->time,
            msg.dest_object(),
            update.object(),
            loc
        );

        // FIXME do something with the data
    }
}

void Object::proximityMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    assert(msg.source_object() == UUID::null()); // Should originate at space server
    CBR::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(msg.payload());
    assert(parse_success);

    for(int32 idx = 0; idx < contents.addition_size(); idx++) {
        CBR::Protocol::Prox::ObjectAddition addition = contents.addition(idx);

        TimedMotionVector3f loc(addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()));

        mContext->trace->prox(
            mContext->time,
            msg.dest_object(),
            addition.object(),
            true,
            loc
        );

        if (!mGlobalIntroductions) {
            CBR::Protocol::Subscription::SubscriptionMessage subs;
            subs.set_action(CBR::Protocol::Subscription::SubscriptionMessage::Subscribe);

            bool success = mContext->objectHost->send(
                mContext->time,
                this, OBJECT_PORT_SUBSCRIPTION,
                addition.object(), OBJECT_PORT_SUBSCRIPTION,
                serializePBJMessage(subs)
            );
            // FIXME do something on failure
        }
    }
    for(int32 idx = 0; idx < contents.removal_size(); idx++) {
        CBR::Protocol::Prox::ObjectRemoval removal = contents.removal(idx);

        mContext->trace->prox(
            mContext->time,
            msg.dest_object(),
            removal.object(),
            false,
            TimedMotionVector3f()
        );

        if (!mGlobalIntroductions) {
            CBR::Protocol::Subscription::SubscriptionMessage subs;
            subs.set_action(CBR::Protocol::Subscription::SubscriptionMessage::Unsubscribe);

            bool success = mContext->objectHost->send(
                mContext->time,
                this, OBJECT_PORT_SUBSCRIPTION,
                removal.object(), OBJECT_PORT_SUBSCRIPTION,
                serializePBJMessage(subs)
            );
            // FIXME do something on failure
        }
    }
}

void Object::subscriptionMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    CBR::Protocol::Subscription::SubscriptionMessage contents;
    bool parse_success = contents.ParseFromString(msg.payload());
    assert(parse_success);

    mContext->trace->subscription(
        mContext->time,
        msg.dest_object(),
        msg.source_object(),
        (contents.action() == CBR::Protocol::Subscription::SubscriptionMessage::Subscribe) ? true : false
    );

    if ( contents.action() == CBR::Protocol::Subscription::SubscriptionMessage::Subscribe )
        addSubscriber(msg.source_object());
    else
        removeSubscriber(msg.source_object());
}

} // namespace CBR
