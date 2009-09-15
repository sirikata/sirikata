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

#include "Options.hpp"
#include "Statistics.hpp"

namespace CBR {

ObjectFactory::ObjectFactory(uint32 count, const BoundingBox3f& region, const Duration& duration)
 : mContext(NULL)
{
    Time start(Time::null());
    Time end = start + duration;
    Vector3f region_extents = region.extents();

    bool simple = GetOption(OBJECT_SIMPLE)->as<bool>();
    bool only_2d = GetOption(OBJECT_2D)->as<bool>();
    float zfactor = (only_2d ? 0.f : 1.f);

    for(uint32 i = 0; i < count; i++) {
        uint8 raw_uuid[UUID::static_size];
        for(uint32 ui = 0; ui < UUID::static_size; ui++)
            raw_uuid[ui] = (uint8)rand() % 256;
        UUID id(raw_uuid, UUID::static_size);

        ObjectInputs* inputs = new ObjectInputs;

        Vector3f startpos = region.min() + Vector3f(randFloat()*region_extents.x, randFloat()*region_extents.y, randFloat()*region_extents.z * zfactor);
        if (GetOption(OBJECT_STATIC)->as<bool>() == true)
            inputs->motion = new StaticMotionPath(start, startpos);
        else
            inputs->motion = new RandomMotionPath(start, end, startpos, 3, Duration::milliseconds((int64)1000), region, zfactor); // FIXME
        float bounds_radius = (simple ? 10.f : (randFloat()*20));
        inputs->bounds = BoundingSphere3f( Vector3f(0, 0, 0), bounds_radius );
        inputs->registerQuery = (randFloat() > 0.5f);
        inputs->queryAngle = SolidAngle(SolidAngle::Max / 900.f); // FIXME how to set this? variability by objects?

        mObjectIDs.insert(id);
        mInputs[id] = inputs;
    }
}

ObjectFactory::~ObjectFactory() {
#ifdef OH_BUILD // should only need to clean these up on object host
    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        Object* obj = it->second;
        delete obj;
    }
#endif //OH_BUILD

    for(ObjectInputsMap::iterator it = mInputs.begin(); it != mInputs.end(); it++) {
        ObjectInputs* inputs = it->second;
        delete inputs->motion;
        delete inputs;
    }
}

void ObjectFactory::initialize(const ObjectHostContext* ctx) {
    mContext = ctx;
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

bool ObjectFactory::registerQuery(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->registerQuery;
}

SolidAngle ObjectFactory::queryAngle(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->queryAngle;
}

#ifdef OH_BUILD

Object* ObjectFactory::object(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );

    ObjectMap::iterator it = mObjects.find(id);
    if (it != mObjects.end()) return it->second;

    Object* new_obj = NULL;
    if (GetOption(OBJECT_GLOBAL)->as<bool>() == true)
        new_obj = new Object(id, motion(id), bounds(id), registerQuery(id), queryAngle(id), mContext, mObjectIDs);
    else
        new_obj = new Object(id, motion(id), bounds(id), registerQuery(id), queryAngle(id), mContext);
    mObjects[id] = new_obj;
    return new_obj;
}
#endif //OH_BUILD

bool ObjectFactory::isActive(const UUID& id) {
    ObjectMap::iterator it = mObjects.find(id);
    return (it != mObjects.end());
}

#ifdef OH_BUILD
void ObjectFactory::tick() {
    Time t = mContext->time;
    for(iterator it = begin(); it != end(); it++) {
        // Active objects receive a tick
        if (isActive(*it)) {
            object(*it)->tick();
            continue;
        }

        // Inactive objects get checked to see if they have moved into this server region
        if (true) { // FIXME always start connection on first tick, should have some starting time or something
            // The object has moved into the region, so start its connection process
            Object* obj = object(*it);
            obj->connect();
        }
    }
}
#endif //OH_BUILD

void ObjectFactory::notifyDestroyed(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mObjects.find(id) != mObjects.end() );

    mObjects.erase(id);
}

} // namespace CBR
