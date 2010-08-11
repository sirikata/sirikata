/*  Sirikata
 *  CBRLocationServiceCache.cpp
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
 *  * Neither the name of libprox nor the names of its contributors may
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

#include "CBRLocationServiceCache.hpp"

namespace Sirikata {

typedef Prox::LocationServiceCache<ObjectProxSimulationTraits> LocationServiceCache;

CBRLocationServiceCache::CBRLocationServiceCache(Network::IOStrand* strand, LocationService* locservice, bool replicas)
 : LocationServiceCache(),
   LocationServiceListener(),
   mStrand(strand),
   mLoc(locservice),
   mListeners(),
   mObjects(),
   mWithReplicas(replicas)
{
    assert(mLoc != NULL);
    mLoc->addListener(this);
}

CBRLocationServiceCache::~CBRLocationServiceCache() {
    mLoc->removeListener(this);
    mLoc = NULL;
    mListeners.clear();
    mObjects.clear();
}

LocationServiceCache::Iterator CBRLocationServiceCache::startTracking(const UUID& id) {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    ObjectDataMap::iterator it = mObjects.find(id);
    assert(it != mObjects.end());

    it->second.tracking = true;

    return Iterator( new IteratorData(id, it) );
}

void CBRLocationServiceCache::stopTracking(const Iterator& id) {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    // In this special case, we ignore the true iterator and do the lookup.
    // This is necessary because ordering problems can cause the iterator to
    // become invalidated.
    IteratorData* itdat = (IteratorData*)id.data;

    ObjectDataMap::iterator it = mObjects.find(itdat->objid);
    if (it == mObjects.end() || it->second.tracking == false) {
        printf("Warning: stopped tracking unknown object\n");
        return;
    }
    mObjects.erase(it);
}

bool CBRLocationServiceCache::tracking(const UUID& id) const {
    return (mObjects.find(id) != mObjects.end());
}


const TimedMotionVector3f& CBRLocationServiceCache::location(const Iterator& id) const {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.location;
}

const BoundingSphere3f& CBRLocationServiceCache::bounds(const Iterator& id) const {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.bounds;
}

float32 CBRLocationServiceCache::radius(const Iterator& id) const {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.bounds.radius();
}

const UUID& CBRLocationServiceCache::iteratorID(const Iterator& id) const {
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    return it->first;
}

void CBRLocationServiceCache::addUpdateListener(LocationUpdateListener* listener) {
    assert( mListeners.find(listener) == mListeners.end() );
    mListeners.insert(listener);
}

void CBRLocationServiceCache::removeUpdateListener(LocationUpdateListener* listener) {
    ListenerSet::iterator it = mListeners.find(listener);
    assert( it != mListeners.end() );
    mListeners.erase(it);
}

#define GET_OBJ_ENTRY(objid) \
    ObjectDataMap::const_iterator it = mObjects.find(id);       \
    assert(it != mObjects.end())

const TimedMotionVector3f& CBRLocationServiceCache::location(const ObjectID& id) const {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.location;
}

const TimedMotionQuaternion& CBRLocationServiceCache::orientation(const ObjectID& id) const {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.orientation;
}

const BoundingSphere3f& CBRLocationServiceCache::bounds(const ObjectID& id) const {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.bounds;
}

float32 CBRLocationServiceCache::radius(const ObjectID& id) const {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.bounds.radius();
}

const String& CBRLocationServiceCache::mesh(const ObjectID& id) const {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.mesh;
}

void CBRLocationServiceCache::localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh) {
    objectAdded(uuid, loc, orient, bounds, mesh);
}

void CBRLocationServiceCache::localObjectRemoved(const UUID& uuid) {
    objectRemoved(uuid);
}

void CBRLocationServiceCache::localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    locationUpdated(uuid, newval);
}

void CBRLocationServiceCache::localOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
    orientationUpdated(uuid, newval);
}

void CBRLocationServiceCache::localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    boundsUpdated(uuid, newval);
}

void CBRLocationServiceCache::localMeshUpdated(const UUID& uuid, const String& newval) {
    meshUpdated(uuid, newval);
}

void CBRLocationServiceCache::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh) {
    if (mWithReplicas)
        objectAdded(uuid, loc, orient, bounds, mesh);
}

void CBRLocationServiceCache::replicaObjectRemoved(const UUID& uuid) {
    if (mWithReplicas)
        objectRemoved(uuid);
}

void CBRLocationServiceCache::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    if (mWithReplicas)
        locationUpdated(uuid, newval);
}

void CBRLocationServiceCache::replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
    if (mWithReplicas)
        orientationUpdated(uuid, newval);
}

void CBRLocationServiceCache::replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    if (mWithReplicas)
        boundsUpdated(uuid, newval);
}

void CBRLocationServiceCache::replicaMeshUpdated(const UUID& uuid, const String& newval) {
    if (mWithReplicas)
        meshUpdated(uuid, newval);
}


void CBRLocationServiceCache::objectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processObjectAdded, this,
            uuid, loc, orient, bounds, mesh
        )
    );
}

void CBRLocationServiceCache::processObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh) {
    if (mObjects.find(uuid) != mObjects.end())
        return;

    ObjectData data;
    data.location = loc;
    data.orientation = orient;
    data.bounds = bounds;
    data.mesh = mesh;
    data.tracking = false;
    mObjects[uuid] = data;

    for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->locationConnected(uuid, loc, bounds);
}

void CBRLocationServiceCache::objectRemoved(const UUID& uuid) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processObjectRemoved, this,
            uuid
        )
    );
}

void CBRLocationServiceCache::processObjectRemoved(const UUID& uuid) {
    ObjectDataMap::iterator data_it = mObjects.find(uuid);
    if (data_it == mObjects.end()) return;

    if (data_it->second.tracking == false)
        mObjects.erase(data_it);

    for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->locationDisconnected(uuid);
}

void CBRLocationServiceCache::locationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processLocationUpdated, this,
            uuid, newval
        )
    );
}

void CBRLocationServiceCache::processLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    TimedMotionVector3f oldval = it->second.location;
    it->second.location = newval;

    for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->locationPositionUpdated(uuid, oldval, newval);
}

void CBRLocationServiceCache::orientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processOrientationUpdated, this,
            uuid, newval
        )
    );
}

void CBRLocationServiceCache::processOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    TimedMotionQuaternion oldval = it->second.orientation;
    it->second.orientation = newval;
}

void CBRLocationServiceCache::boundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processBoundsUpdated, this,
            uuid, newval
        )
    );
}

void CBRLocationServiceCache::processBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    BoundingSphere3f oldval = it->second.bounds;
    it->second.bounds = newval;

    for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->locationBoundsUpdated(uuid, oldval, newval);
}

void CBRLocationServiceCache::meshUpdated(const UUID& uuid, const String& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processMeshUpdated, this,
            uuid, newval
        )
    );
}

void CBRLocationServiceCache::processMeshUpdated(const UUID& uuid, const String& newval) {
    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;
    String oldval = it->second.mesh;
    it->second.mesh = newval;
}

} // namespace Sirikata
