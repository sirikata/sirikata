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

namespace CBR {

ObjectFactory::ObjectFactory(uint32 count, const BoundingBox3f& region, const Duration& duration) {
    Time start(0);
    Time end = start + duration;
    Vector3f region_extents = region.extents();

    for(uint32 i = 0; i < count; i++) {
        UUID id = UUID::random(); // FIXME

        ObjectInputs* inputs = new ObjectInputs;

        Vector3f startpos = region.min() + Vector3f(randFloat()*region_extents.x, randFloat()*region_extents.y, randFloat()*region_extents.z);
        inputs->motion = new RandomMotionPath(start, end, startpos, 1, Duration::milliseconds((uint32)1000)); // FIXME

        inputs->proximityRadius = randFloat() * 50 + 50; // FIXME

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

float ObjectFactory::proximityRadius(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mInputs.find(id) != mInputs.end() );
    return mInputs[id]->proximityRadius;
}

Object* ObjectFactory::object(const UUID& id, Server* server) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );

    ObjectMap::iterator it = mObjects.find(id);
    if (it != mObjects.end()) return it->second;

    Object* new_obj = new Object(server, id, motion(id), proximityRadius(id));
    mObjects[id] = new_obj;
    return new_obj;
}

void ObjectFactory::destroyObject(const UUID& id) {
    assert( mObjectIDs.find(id) != mObjectIDs.end() );
    assert( mObjects.find(id) != mObjects.end() );

    Object* obj = mObjects[id];
    delete obj;
    mObjects.erase(id);
}

} // namespace CBR
