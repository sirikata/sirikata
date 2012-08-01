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
 : ExtendedLocationServiceCache(),
   LocationServiceListener(),
   mStrand(strand),
   mLoc(locservice),
   mListeners(),
   mObjects(),
   mWithReplicas(replicas)
{
    assert(mLoc != NULL);
    mLoc->addListener(this, true);
}

CBRLocationServiceCache::~CBRLocationServiceCache() {
    mLoc->removeListener(this);
    mLoc = NULL;
    mListeners.clear();
    mObjects.clear();
}

void CBRLocationServiceCache::addPlaceholderImposter(
    const ObjectID& uuid,
    const Vector3f& center_offset,
    const float32 center_bounds_radius,
    const float32 max_size,
    const String& zernike,
    const String& mesh
) {
    TimedMotionVector3f loc(mLoc->context()->simTime(), MotionVector3f(center_offset, Vector3f(0,0,0)));
    TimedMotionQuaternion orient;
    AggregateBoundingInfo bounds(Vector3f(0, 0, 0), center_bounds_radius, max_size);
    String phy;
    objectAdded(
        uuid.getAsUUID(), true, true, loc, orient, bounds, mesh, phy, zernike
    );
}


LocationServiceCache::Iterator CBRLocationServiceCache::startTracking(const ObjectReference& id) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(id);
    assert(it != mObjects.end());

    it->second.tracking++;

    return Iterator( new IteratorData(id, it) );
}

bool CBRLocationServiceCache::startRefcountTracking(const ObjectID& id) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(id);
    assert(it != mObjects.end());

    it->second.tracking++;

    return true;
}

void CBRLocationServiceCache::stopTracking(const Iterator& id) {
    Lock lck(mMutex);

    // In this special case, we ignore the true iterator and do the lookup.
    // This is necessary because ordering problems can cause the iterator to
    // become invalidated.
    IteratorData* itdat = (IteratorData*)id.data;

    stopRefcountTracking(itdat->objid);
}

void CBRLocationServiceCache::stopRefcountTracking(const ObjectID& objid) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(objid);
    if (it == mObjects.end()) {
        printf("Warning: stopped tracking unknown object\n");
        return;
    }
    if (it->second.tracking <= 0) {
        printf("Warning: stopped tracking untracked object\n");
    }
    it->second.tracking--;
    tryRemoveObject(it);
}

bool CBRLocationServiceCache::tracking(const ObjectReference& id) {
    Lock lck(mMutex);

    return (mObjects.find(id) != mObjects.end());
}

TimedMotionVector3f CBRLocationServiceCache::location(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.location;
}

Prox::ZernikeDescriptor& CBRLocationServiceCache::zernikeDescriptor(const Iterator& id)  {
  // NOTE: Only accesses via iterator, shouldn't need a lock
  IteratorData* itdat = (IteratorData*)id.data;
  ObjectDataMap::iterator it = itdat->it;
  assert(it != mObjects.end());

  return it->second.zernike;
}

String CBRLocationServiceCache::mesh(const Iterator& id)  {
  // NOTE: Only accesses via iterator, shouldn't need a lock
  IteratorData* itdat = (IteratorData*)id.data;
  ObjectDataMap::iterator it = itdat->it;
  assert(it != mObjects.end());

  return it->second.mesh;
}

Vector3f CBRLocationServiceCache::centerOffset(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    // Max size is just the size of the object.
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.bounds.centerOffset;
}

float32 CBRLocationServiceCache::centerBoundsRadius(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    // Max size is just the size of the object.
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.bounds.centerBoundsRadius;
}

float32 CBRLocationServiceCache::maxSize(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    // Max size is just the size of the object.
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.bounds.maxObjectRadius;
}

bool CBRLocationServiceCache::isLocal(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.isLocal;
}


const ObjectReference& CBRLocationServiceCache::iteratorID(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    return it->first;
}

void CBRLocationServiceCache::addUpdateListener(LocationUpdateListener* listener) {
    Lock lck(mMutex);

    assert( mListeners.find(listener) == mListeners.end() );
    mListeners.insert(listener);
}

void CBRLocationServiceCache::removeUpdateListener(LocationUpdateListener* listener) {
    Lock lck(mMutex);

    ListenerSet::iterator it = mListeners.find(listener);
    assert( it != mListeners.end() );
    mListeners.erase(it);
}

#define GET_OBJ_ENTRY(objid) \
    ObjectDataMap::const_iterator it = mObjects.find(id);       \
    assert(it != mObjects.end())

TimedMotionVector3f CBRLocationServiceCache::location(const ObjectID& id) {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.location;
}

TimedMotionQuaternion CBRLocationServiceCache::orientation(const ObjectID& id) {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.orientation;
}

AggregateBoundingInfo CBRLocationServiceCache::bounds(const ObjectID& id) {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.bounds;
}

Transfer::URI CBRLocationServiceCache::mesh(const ObjectID& id) {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return Transfer::URI(it->second.mesh);
}

String CBRLocationServiceCache::physics(const ObjectID& id) {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.physics;
}


const bool CBRLocationServiceCache::isAggregate(const ObjectID& id) const {
    GET_OBJ_ENTRY(id); // NOTE: should only be accessed by prox thread, shouldn't need lock
    return it->second.isAggregate;
}


void CBRLocationServiceCache::localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& phy, const String& zernike) {
  objectAdded(uuid, true, agg, loc, orient, bounds, mesh, phy, zernike);
}

void CBRLocationServiceCache::localObjectRemoved(const UUID& uuid, bool agg) {
    objectRemoved(uuid, agg);
}

void CBRLocationServiceCache::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
    locationUpdated(uuid, agg, newval);
}

void CBRLocationServiceCache::localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {
    orientationUpdated(uuid, agg, newval);
}

void CBRLocationServiceCache::localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) {
    boundsUpdated(uuid, agg, newval);
}

void CBRLocationServiceCache::localMeshUpdated(const UUID& uuid, bool agg, const String& newval) {
    meshUpdated(uuid, agg, newval);
}

void CBRLocationServiceCache::localPhysicsUpdated(const UUID& uuid, bool agg, const String& newval) {
    physicsUpdated(uuid, agg, newval);
}

void CBRLocationServiceCache::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& phy, const String& zernike) {
    if (mWithReplicas)
      objectAdded(uuid, false, false, loc, orient, bounds, mesh, phy, zernike);
}

void CBRLocationServiceCache::replicaObjectRemoved(const UUID& uuid) {
    if (mWithReplicas)
        objectRemoved(uuid, false);
}

void CBRLocationServiceCache::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    if (mWithReplicas)
        locationUpdated(uuid, false, newval);
}

void CBRLocationServiceCache::replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
    if (mWithReplicas)
        orientationUpdated(uuid, false, newval);
}

void CBRLocationServiceCache::replicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval) {
    if (mWithReplicas)
        boundsUpdated(uuid, false, newval);
}

void CBRLocationServiceCache::replicaMeshUpdated(const UUID& uuid, const String& newval) {
    if (mWithReplicas)
        meshUpdated(uuid, false, newval);
}

void CBRLocationServiceCache::replicaPhysicsUpdated(const UUID& uuid, const String& newval) {
    if (mWithReplicas)
        physicsUpdated(uuid, false, newval);
}


void CBRLocationServiceCache::objectAdded(const UUID& uuid, bool islocal, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& phy, const String& zernike) {
    // This looks a bit odd compared to some other similar methods. We check for
    // and insert the object immediately because addPlaceholderImposter needs it
    // to occur immediately (because imposters need to be added so they can be
    // refcounted by the query events to ensure they stay alive in this cache as
    // long as they are needed). TODO(ewencp) We should probably just make this
    // look more like the other implementations of LocationServiceCache (using
    // posting only to get notifications in the right strand) but who knows what
    // kind of fallout that might have...
    Lock lck(mMutex);

    if (mObjects.find(ObjectReference(uuid)) != mObjects.end())
        return;

    ObjectData data;
    data.location = loc;
    data.orientation = orient;
    data.bounds = bounds;
    data.mesh = mesh;
    data.physics = phy;
    data.zernike = zernike;
    data.isLocal = islocal;
    data.exists = true;
    data.tracking = 0;
    data.isAggregate = agg;

    mObjects[ObjectReference(uuid)] = data;

    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processObjectAdded, this,
            ObjectReference(uuid), data
        ),
        "CBRLocationServiceCache::processObjectAdded"
    );
}

void CBRLocationServiceCache::processObjectAdded(const ObjectReference& uuid, ObjectData data) {
    // TODO(ewencp) at some point, we might want to (optionally) use aggregates
    // here, especially if we're reconstructing entire trees.
    if (!data.isAggregate)
        for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
            (*it)->locationConnected(uuid, false, data.isLocal, data.location, data.bounds.centerBounds(), data.bounds.maxObjectRadius);
}

void CBRLocationServiceCache::objectRemoved(const UUID& uuid, bool agg) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processObjectRemoved, this,
            ObjectReference(uuid), agg
        ),
        "CBRLocationServiceCache::processObjectRemoved"
    );
}

void CBRLocationServiceCache::processObjectRemoved(const ObjectReference& uuid, bool agg) {
    Lock lck(mMutex);

    ObjectDataMap::iterator data_it = mObjects.find(uuid);
    if (data_it == mObjects.end()) return;

    assert(data_it->second.exists);
    data_it->second.exists = false;

    tryRemoveObject(data_it);

    if (!agg)
        for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
            (*it)->locationDisconnected(uuid);
}

void CBRLocationServiceCache::locationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processLocationUpdated, this,
            ObjectReference(uuid), agg, newval
        ),
        "CBRLocationServiceCache::processLocationUpdated"
    );
}

void CBRLocationServiceCache::processLocationUpdated(const ObjectReference& uuid, bool agg, const TimedMotionVector3f& newval) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    TimedMotionVector3f oldval = it->second.location;
    it->second.location = newval;

    if (!agg)
        for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
            (*it)->locationPositionUpdated(uuid, oldval, newval);
}

void CBRLocationServiceCache::orientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processOrientationUpdated, this,
            ObjectReference(uuid), agg, newval
        ),
        "CBRLocationServiceCache::processOrientationUpdated"
    );
}

void CBRLocationServiceCache::processOrientationUpdated(const ObjectReference& uuid, bool agg, const TimedMotionQuaternion& newval) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    it->second.orientation = newval;
}

void CBRLocationServiceCache::boundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processBoundsUpdated, this,
            ObjectReference(uuid), agg, newval
        ),
        "CBRLocationServiceCache::processBoundsUpdated"
    );
}

void CBRLocationServiceCache::processBoundsUpdated(const ObjectReference& uuid, bool agg, const AggregateBoundingInfo& newval) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    AggregateBoundingInfo oldval = it->second.bounds;
    it->second.bounds = newval;

    if (!agg) {
        for(ListenerSet::iterator listen_it = mListeners.begin(); listen_it != mListeners.end(); listen_it++) {
            (*listen_it)->locationRegionUpdated(uuid, oldval.centerBounds(), it->second.bounds.centerBounds());
            (*listen_it)->locationMaxSizeUpdated(uuid, oldval.maxObjectRadius, it->second.bounds.maxObjectRadius);
        }
    }
}

void CBRLocationServiceCache::meshUpdated(const UUID& uuid, bool agg, const String& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processMeshUpdated, this,
            ObjectReference(uuid), agg, newval
        ),
        "CBRLocationServiceCache::processMeshUpdated"
    );
}

void CBRLocationServiceCache::processMeshUpdated(const ObjectReference& uuid, bool agg, const String& newval) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;
    String oldval = it->second.mesh;
    it->second.mesh = newval;
}

void CBRLocationServiceCache::physicsUpdated(const UUID& uuid, bool agg, const String& newval) {
    mStrand->post(
        std::tr1::bind(
            &CBRLocationServiceCache::processPhysicsUpdated, this,
            ObjectReference(uuid), agg, newval
        ),
        "CBRLocationServiceCache::processPhysicsUpdated"
    );
}

void CBRLocationServiceCache::processPhysicsUpdated(const ObjectReference& uuid, bool agg, const String& newval) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;
    String oldval = it->second.physics;
    it->second.physics = newval;
}

bool CBRLocationServiceCache::tryRemoveObject(ObjectDataMap::iterator& obj_it) {
    if (obj_it->second.tracking > 0  || obj_it->second.exists)
        return false;

    mObjects.erase(obj_it);
    return true;
}

} // namespace Sirikata
