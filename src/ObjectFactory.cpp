/*  cbr
 *  ObjectFactory.cpp
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

#include "ObjectFactory.hpp"
#include "RandomMotionPath.hpp"
#include "Object.hpp"
#include "Random.hpp"

#include "QuakeMotionPath.hpp"
#include "StaticMotionPath.hpp"
#include "ObjectMessageQueue.hpp"
#include "ObjectConnection.hpp"
#include "Server.hpp"

#include "Options.hpp"
#include "Statistics.hpp"

namespace CBR {

ObjectFactory::ObjectFactory(uint32 count, const BoundingBox3f& region, const Duration& duration, Trace* trace)
 : mObjectMessageQueue(NULL),
   mServerID(0),
   mServer(NULL),
   mCSeg(NULL),
   mFirstTick(true),
   mTrace(trace)
{
    Time start(Time::null());
    Time end = start + duration;
    Vector3f region_extents = region.extents();

    for(uint32 i = 0; i < count; i++) {
        uint8 raw_uuid[UUID::static_size];
        for(uint32 ui = 0; ui < UUID::static_size; ui++)
            raw_uuid[ui] = (uint8)rand() % 256;
        UUID id(raw_uuid, UUID::static_size);

        ObjectInputs* inputs = new ObjectInputs;

        Vector3f startpos = region.min() + Vector3f(randFloat()*region_extents.x, randFloat()*region_extents.y, randFloat()*region_extents.z);
        if (GetOption(OBJECT_STATIC)->as<bool>() == true)
            inputs->motion = new StaticMotionPath(start, startpos);
        else
            inputs->motion = new RandomMotionPath(start, end, startpos, 10, Duration::milliseconds((int64)1000), region); // FIXME
        inputs->bounds = BoundingSphere3f( Vector3f(0, 0, 0), randFloat() * 20 );
        inputs->queryAngle = SolidAngle(SolidAngle::Max / 900.f); // FIXME how to set this? variability by objects?

        mObjectIDs.insert(id);
        mInputs[id] = inputs;
    }
}

ObjectFactory::~ObjectFactory() {
    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        Object* obj = it->second;
        delete obj;
    }

    for(ObjectInputsMap::iterator it = mInputs.begin(); it != mInputs.end(); it++) {
        ObjectInputs* inputs = it->second;
        delete inputs->motion;
        delete inputs;
    }
}

void ObjectFactory::initialize(ServerID sid, Server* server, CoordinateSegmentation* cseg) {
    mServerID = sid;
    mServer = server;
    mCSeg = cseg;
}

ObjectFactory::iterator ObjectFactory::begin() {
    return mObjectIDs.begin();
}

ObjectFactory::const_iterator ObjectFactory::begin() const {
    return mObjectIDs.begin();
}

ObjectFactory::iterator ObjectFactory::end() {
    return mObjectIDs.end();
}

ObjectFactory::const_iterator ObjectFactory::end() const {
    return mObjectIDs.end();
}

MotionPath* ObjectFactory::motion(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->motion;
}

BoundingSphere3f ObjectFactory::bounds(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->bounds;
}

SolidAngle ObjectFactory::queryAngle(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->queryAngle;
}

Object* ObjectFactory::object(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );

    ObjectMap::iterator it = mObjects.find(id);
    if (it != mObjects.end()) return it->second;

    assert( mObjectMessageQueue != NULL);

    Object* new_obj = NULL;
    if (GetOption(OBJECT_GLOBAL)->as<bool>() == true)
        new_obj = new Object(mServerID, this, id, mObjectMessageQueue, motion(id), queryAngle(id), mObjectIDs);
    else
        new_obj = new Object(mServerID, this, id, mObjectMessageQueue, motion(id), queryAngle(id));
    mObjects[id] = new_obj;
    return new_obj;
}

void ObjectFactory::setObjectMessageQueue(ObjectMessageQueue* omq) {
    mObjectMessageQueue = omq;
    for(ObjectSet::iterator it = mObjectIDs.begin(); it != mObjectIDs.end(); it++) {
        UUID id = *it;
        mObjectMessageQueue->registerClient(id, 1);
    }
}

bool ObjectFactory::isActive(const UUID& id) {
    ObjectMap::iterator it = mObjects.find(id);
    return (it != mObjects.end());
}

static bool isInServerRegion(const BoundingBoxList& bboxlist, Vector3f pos) {
    for(uint32 i = 0; i < bboxlist.size(); i++)
        if (bboxlist[i].contains(pos)) return true;
    return false;
}

void ObjectFactory::tick(const Time& t) {
    BoundingBoxList serverRegion = mCSeg->serverRegion(mServerID);
    for(iterator it = begin(); it != end(); it++) {
        // Active objects receive a tick
        if (isActive(*it)) {
            object(*it)->tick(t);
            continue;
        }
        // Inactive objects get checked to see if they have moved into this server region
        TimedMotionVector3f curMotion = motion(*it)->at(t);
        Vector3f curPos = curMotion.extrapolate(t).position();
        if (isInServerRegion(serverRegion, curPos)) {
            // The object has moved into the region, so start its connection process
            Object* obj = object(*it);
            ObjectConnection* conn = new ObjectConnection(obj, mTrace);

            if (mFirstTick) { // initial connection
                CBR::Protocol::Session::Connect connect_msg;
                connect_msg.set_object(*it);
                CBR::Protocol::Session::ITimedMotionVector loc = connect_msg.mutable_loc();
                loc.set_t(curMotion.updateTime());
                loc.set_position(curMotion.position());
                loc.set_velocity(curMotion.velocity());
                connect_msg.set_bounds(obj->bounds());
                connect_msg.set_query_angle(obj->queryAngle().asFloat());

                std::string connect_serialized = serializePBJMessage(connect_msg);
                mServer->connect(conn, connect_serialized);
            }
            else { // migration
                CBR::Protocol::Session::Migrate migrate_msg;
                migrate_msg.set_object(*it);
                std::string migrate_serialized = serializePBJMessage(migrate_msg);
                mServer->migrate(conn, migrate_serialized);
            }
        }
    }

    mFirstTick = false;
}

void ObjectFactory::notifyDestroyed(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mObjects.find(id) != mObjects.end() );

    mObjects.erase(id);
}

} // namespace CBR
