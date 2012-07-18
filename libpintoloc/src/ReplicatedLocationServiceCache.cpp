// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/pintoloc/ReplicatedLocationServiceCache.hpp>
#include <sirikata/core/network/IOStrand.hpp>

namespace Sirikata {

typedef Prox::LocationServiceCache<ObjectProxSimulationTraits> LocationServiceCache;

ReplicatedLocationServiceCache::ReplicatedLocationServiceCache(Network::IOStrandPtr strand)
 : ExtendedLocationServiceCache(),
   mStrand(strand.get()),
   mListeners(),
   mObjects()
{
}
ReplicatedLocationServiceCache::ReplicatedLocationServiceCache(Network::IOStrand* strand)
 : ExtendedLocationServiceCache(),
   mStrand(strand),
   mListeners(),
   mObjects()
{
}

ReplicatedLocationServiceCache::~ReplicatedLocationServiceCache() {
    mListeners.clear();
    mObjects.clear();
}

void ReplicatedLocationServiceCache::addPlaceholderImposter(
    const ObjectID& uuid,
    const Vector3f& center_offset,
    const float32 center_bounds_radius,
    const float32 max_size,
    const String& zernike,
    const String& mesh
) {
    // We only deal with replicated trees, so you might expect this not to pop
    // up. But for manual queries we're going to have an aggregate listener
    // registered, which triggers these calls. So we can't assert(false), but we
    // will just ignore the call. In fact, we can check that we don't get any
    // unexpected calls here since any objects getting nodes created for them
    // should *already* exist in here, making the call useless anyway.

    Lock lck(mMutex);
    ObjectDataMap::iterator it = mObjects.find(uuid);
    assert(it != mObjects.end());
}

LocationServiceCache::Iterator ReplicatedLocationServiceCache::startTracking(const ObjectReference& id) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(id);
    assert(it != mObjects.end());

    it->second.tracking++;

    return Iterator( new IteratorData(id, it) );
}

void ReplicatedLocationServiceCache::stopTracking(const Iterator& id) {
    Lock lck(mMutex);

    // In this special case, we ignore the true iterator and do the lookup.
    // This is necessary because ordering problems can cause the iterator to
    // become invalidated.
    IteratorData* itdat = (IteratorData*)id.data;

    ObjectDataMap::iterator it = mObjects.find(itdat->objid);
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

bool ReplicatedLocationServiceCache::startSimpleTracking(const ObjectID& id) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(id);
    if (it == mObjects.end()) return false;

    it->second.tracking++;
    return true;
}

void ReplicatedLocationServiceCache::stopSimpleTracking(const ObjectID& id) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(id);
    assert (it != mObjects.end());

    it->second.tracking--;
    tryRemoveObject(it);
}


bool ReplicatedLocationServiceCache::tracking(const ObjectReference& id) {
    Lock lck(mMutex);

    return (mObjects.find(id) != mObjects.end());
}


TimedMotionVector3f ReplicatedLocationServiceCache::location(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.props.location();
}

Vector3f ReplicatedLocationServiceCache::centerOffset(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    // Max size is just the size of the object.
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.props.bounds().centerOffset;
}

float32 ReplicatedLocationServiceCache::centerBoundsRadius(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    // Max size is just the size of the object.
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.props.bounds().centerBoundsRadius;
}

float32 ReplicatedLocationServiceCache::maxSize(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    // Max size is just the size of the object.
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.props.bounds().maxObjectRadius;
}

bool ReplicatedLocationServiceCache::isLocal(const Iterator& id) {
    return true;
}

String ReplicatedLocationServiceCache::mesh(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.props.mesh().toString();
}

Prox::ZernikeDescriptor& ReplicatedLocationServiceCache::zernikeDescriptor(const Iterator& id) {
  return Prox::ZernikeDescriptor::null();
}

const ObjectReference& ReplicatedLocationServiceCache::iteratorID(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    return it->first;
}

void ReplicatedLocationServiceCache::addUpdateListener(LocationUpdateListener* listener) {
    Lock lck(mMutex);

    assert( mListeners.find(listener) == mListeners.end() );
    mListeners.insert(listener);
}

void ReplicatedLocationServiceCache::removeUpdateListener(LocationUpdateListener* listener) {
    Lock lck(mMutex);

    ListenerSet::iterator it = mListeners.find(listener);
    assert( it != mListeners.end() );
    mListeners.erase(it);
}

#define GET_OBJ_ENTRY(objid) \
    Lock lck(mMutex); \
    ObjectDataMap::const_iterator it = mObjects.find(id);       \
    assert(it != mObjects.end())

uint64 ReplicatedLocationServiceCache::epoch(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.epoch;
}

TimedMotionVector3f ReplicatedLocationServiceCache::location(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.location();
}

TimedMotionQuaternion ReplicatedLocationServiceCache::orientation(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.orientation();
}

AggregateBoundingInfo ReplicatedLocationServiceCache::bounds(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.bounds();
}

Transfer::URI ReplicatedLocationServiceCache::mesh(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.mesh();
}

String ReplicatedLocationServiceCache::physics(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.physics();
}

ObjectReference ReplicatedLocationServiceCache::parent(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.parent;
}

bool ReplicatedLocationServiceCache::aggregate(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.aggregate;
}

const SequencedPresenceProperties& ReplicatedLocationServiceCache::properties(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props;
}

void ReplicatedLocationServiceCache::objectAdded(
    const ObjectReference& uuid, bool agg,
    const ObjectReference& parent,
    const TimedMotionVector3f& loc, uint64 loc_seqno,
    const TimedMotionQuaternion& orient, uint64 orient_seqno,
    const AggregateBoundingInfo& bounds, uint64 bounds_seqno,
    const Transfer::URI& mesh, uint64 mesh_seqno,
    const String& physics, uint64 physics_seqno
) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    assert(it == mObjects.end() || (it->second.exists == false));

    if (it == mObjects.end())
        it = mObjects.insert( ObjectDataMap::value_type(uuid, ObjectData()) ).first;
    else
        it->second.props = SequencedPresenceProperties(); // reset
    it->second.exists = true;
    it->second.props.setLocation(loc, loc_seqno);
    it->second.props.setOrientation(orient, orient_seqno);
    it->second.props.setBounds(bounds, bounds_seqno);
    it->second.props.setMesh(mesh, mesh_seqno);
    it->second.props.setPhysics(physics, physics_seqno);
    it->second.aggregate = agg;
    it->second.parent = parent;


    it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyObjectAdded, this,
            uuid, parent, agg, loc, bounds
        ),
        "ReplicatedLocationServiceCache::notifyObjectAdded"
    );
}

void ReplicatedLocationServiceCache::notifyObjectAdded(
    const ObjectReference& uuid,
    const ObjectReference& parent, bool agg,
    const TimedMotionVector3f& loc,
    const AggregateBoundingInfo& bounds
) {
    Lock lck(mMutex);

    for(ListenerSet::iterator listener_it = mListeners.begin(); listener_it != mListeners.end(); listener_it++)
        (*listener_it)->locationConnectedWithParent(uuid, parent, agg, true, loc, bounds.centerBounds(), bounds.maxObjectRadius);

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onObjectAdded, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void ReplicatedLocationServiceCache::objectRemoved(const ObjectReference& uuid, bool temporary) {
    Lock lck(mMutex);

    ObjectDataMap::iterator data_it = mObjects.find(uuid);
    if (data_it == mObjects.end()) return;

    assert(data_it->second.exists);
    data_it->second.exists = false;
    bool agg = data_it->second.aggregate;

    data_it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyObjectRemoved, this,
            uuid, temporary
        ),
        "ReplicatedLocationServiceCache::notifyObjectRemoved"
    );
}

void ReplicatedLocationServiceCache::notifyObjectRemoved(const ObjectReference& uuid, bool temporary) {
    Lock lck(mMutex);

    for(ListenerSet::iterator listener_it = mListeners.begin(); listener_it != mListeners.end(); listener_it++)
        (*listener_it)->locationDisconnected(uuid, temporary);

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onObjectRemoved, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void ReplicatedLocationServiceCache::epochUpdated(const ObjectReference& uuid, const uint64 ep) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    it->second.epoch = std::max(it->second.epoch, ep);

    bool agg = it->second.aggregate;

    it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyEpochUpdated, this,
            uuid, ep
        ),
        "ReplicatedLocationServiceCache::notifyEpochUpdated"
    );
}

void ReplicatedLocationServiceCache::notifyEpochUpdated(const ObjectReference& uuid, const uint64 val) {
    Lock lck(mMutex);

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onEpochUpdated, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void ReplicatedLocationServiceCache::locationUpdated(const ObjectReference& uuid, const TimedMotionVector3f& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    TimedMotionVector3f oldval = it->second.props.location();
    it->second.props.setLocation(newval, seqno);

    bool agg = it->second.aggregate;

    it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyLocationUpdated, this,
            uuid, oldval, newval
        ),
        "ReplicatedLocationServiceCache::notifyLocationUpdated"
    );
}

void ReplicatedLocationServiceCache::notifyLocationUpdated(const ObjectReference& uuid, const TimedMotionVector3f& oldval, const TimedMotionVector3f& newval) {
    Lock lck(mMutex);

    for(ListenerSet::iterator listener_it = mListeners.begin(); listener_it != mListeners.end(); listener_it++)
        (*listener_it)->locationPositionUpdated(uuid, oldval, newval);

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onLocationUpdated, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void ReplicatedLocationServiceCache::orientationUpdated(const ObjectReference& uuid, const TimedMotionQuaternion& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    it->second.props.setOrientation(newval, seqno);

    bool agg = it->second.aggregate;

    it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyOrientationUpdated, this, uuid
        )
    );
}

void ReplicatedLocationServiceCache::notifyOrientationUpdated(const ObjectReference& uuid) {
    Lock lck(mMutex);

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onOrientationUpdated, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void ReplicatedLocationServiceCache::boundsUpdated(const ObjectReference& uuid, const AggregateBoundingInfo& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    AggregateBoundingInfo oldval = it->second.props.bounds();
    it->second.props.setBounds(newval, seqno);

    bool agg = it->second.aggregate;

    it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyBoundsUpdated, this,
            uuid, oldval, newval
        ),
        "ReplicatedLocationServiceCache::notifyBoundsUpdated"
    );
}

void ReplicatedLocationServiceCache::notifyBoundsUpdated(const ObjectReference& uuid, const AggregateBoundingInfo& oldval, const AggregateBoundingInfo& newval) {
    Lock lck(mMutex);

    for(ListenerSet::iterator listen_it = mListeners.begin(); listen_it != mListeners.end(); listen_it++) {
        (*listen_it)->locationRegionUpdated(uuid, oldval.centerBounds(), newval.centerBounds());
        (*listen_it)->locationMaxSizeUpdated(uuid, oldval.maxObjectRadius, newval.maxObjectRadius);
    }

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onBoundsUpdated, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void ReplicatedLocationServiceCache::meshUpdated(const ObjectReference& uuid, const Transfer::URI& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;
    Transfer::URI oldval = it->second.props.mesh();
    it->second.props.setMesh(newval, seqno);

    bool agg = it->second.aggregate;

    it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyMeshUpdated, this, uuid
        )
    );
}

void ReplicatedLocationServiceCache::notifyMeshUpdated(const ObjectReference& uuid) {
    Lock lck(mMutex);

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onMeshUpdated, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void ReplicatedLocationServiceCache::physicsUpdated(const ObjectReference& uuid, const String& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;
    String oldval = it->second.props.physics();
    it->second.props.setPhysics(newval, seqno);

    bool agg = it->second.aggregate;

    it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyPhysicsUpdated, this, uuid
        )
    );
}

void ReplicatedLocationServiceCache::notifyPhysicsUpdated(const ObjectReference& uuid) {
    Lock lck(mMutex);

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onPhysicsUpdated, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void ReplicatedLocationServiceCache::parentUpdated(const ObjectReference& uuid, const ObjectReference& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;
    ObjectReference oldval = it->second.parent;
    it->second.parent = newval; // FIXME seqno?

    bool agg = it->second.aggregate;

    it->second.tracking++;
    mStrand->post(
        std::tr1::bind(
            &ReplicatedLocationServiceCache::notifyParentUpdated, this, uuid, oldval, newval
        )
    );
}

void ReplicatedLocationServiceCache::notifyParentUpdated(const ObjectReference& uuid, const ObjectReference& oldval, const ObjectReference& newval) {
    Lock lck(mMutex);

    for(ListenerSet::iterator listener_it = mListeners.begin(); listener_it != mListeners.end(); listener_it++)
        (*listener_it)->locationParentUpdated(uuid, oldval, newval);

    ReplicatedLocationUpdateProvider::notify(&ReplicatedLocationUpdateListener::onParentUpdated, this, uuid);

    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}


bool ReplicatedLocationServiceCache::tryRemoveObject(ObjectDataMap::iterator& obj_it) {
    if (obj_it->second.tracking > 0  || obj_it->second.exists)
        return false;

    mObjects.erase(obj_it);
    return true;
}

bool ReplicatedLocationServiceCache::empty() {
    // Indicates if we have any objects with exists == true, so we can just scan
    // through looking for them. If we need something faster we can keep track
    // of a count based on addObject/removeObject calls

    Lock lck(mMutex);

    for(ObjectDataMap::iterator obj_it = mObjects.begin(); obj_it != mObjects.end(); obj_it++)
        if (obj_it->second.exists) return false;

    return true;
}

bool ReplicatedLocationServiceCache::fullyEmpty() {
    Lock lck(mMutex);
    return mObjects.empty();
}

} // namespace Sirikata
