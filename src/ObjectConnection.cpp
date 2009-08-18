/*  cbr
 *  ObjectConnection.cpp
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

#include "ObjectConnection.hpp"
#include "Object.hpp"
#include "Message.hpp"
#include "Statistics.hpp"

namespace CBR {

ObjectConnection::ObjectConnection(Object* obj, Trace* trace)
 : mObject(obj),
   mTrace(trace),
   mReceiveQueue()
{
}

ObjectConnection::~ObjectConnection() {
    // FIXME is it really ok to just drop these messages
    for(MessageQueue::iterator it = mReceiveQueue.begin(); it != mReceiveQueue.end(); it++)
        delete *it;
    mReceiveQueue.clear();
}

UUID ObjectConnection::id() const {
    return mObject->uuid();
}


void ObjectConnection::deliver(const CBR::Protocol::Object::ObjectMessage& msg) {
    mReceiveQueue.push_back( new CBR::Protocol::Object::ObjectMessage(msg) );
}

void ObjectConnection::tick(const Time& t) {
    for(MessageQueue::iterator msg_it = mReceiveQueue.begin(); msg_it != mReceiveQueue.end(); msg_it++) {
        CBR::Protocol::Object::ObjectMessage* msg = (*msg_it);
        mObject->receiveMessage(msg);
    }

    mReceiveQueue.clear();
}

void ObjectConnection::handleMigrateMessage(const UUID& oid, const SolidAngle& sa, const std::vector<UUID> subs) {
    mObject->migrateMessage(oid, sa, subs);
}

void ObjectConnection::fillMigrateMessage(MigrateMessage* migrate_msg) const {
    const UUID& obj_id = mObject->uuid();

    migrate_msg->contents.set_object(obj_id);

    CBR::Protocol::Migration::ITimedMotionVector loc = migrate_msg->contents.mutable_loc();
    loc.set_t( mObject->location().updateTime());
    loc.set_position(mObject->location().position());
    loc.set_velocity(mObject->location().velocity());

    migrate_msg->contents.set_bounds(mObject->bounds());
    migrate_msg->contents.set_query_angle(mObject->queryAngle().asFloat());
    for (ObjectSet::iterator it = mObject->subscriberSet().begin(); it != mObject->subscriberSet().end(); it++)
        migrate_msg->contents.add_subscriber(*it);
}

} // namespace CBR
