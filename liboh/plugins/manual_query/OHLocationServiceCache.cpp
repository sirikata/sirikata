// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "OHLocationServiceCache.hpp"
#include <sirikata/core/network/IOStrand.hpp>

namespace Sirikata {

namespace {
BoundingSphere3f regionFromBounds(const BoundingSphere3f& bnds) {
    return BoundingSphere3f(bnds.center(), 0.f);
}
float32 maxSizeFromBounds(const BoundingSphere3f& bnds) {
    return bnds.radius();
}
} // namespace

typedef Prox::LocationServiceCache<ObjectProxSimulationTraits> LocationServiceCache;

OHLocationServiceCache::OHLocationServiceCache(Network::IOStrand* strand)
 : LocationServiceCache(),
   mStrand(strand),
   mListeners(),
   mObjects()
{
}

OHLocationServiceCache::~OHLocationServiceCache() {
    mListeners.clear();
    mObjects.clear();
}

LocationServiceCache::Iterator OHLocationServiceCache::startTracking(const ObjectReference& id) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(id);
    assert(it != mObjects.end());

    it->second.tracking++;

    return Iterator( new IteratorData(id, it) );
}

void OHLocationServiceCache::stopTracking(const Iterator& id) {
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

bool OHLocationServiceCache::tracking(const ObjectReference& id) {
    Lock lck(mMutex);

    return (mObjects.find(id) != mObjects.end());
}


TimedMotionVector3f OHLocationServiceCache::location(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.props.location();
}

BoundingSphere3f OHLocationServiceCache::region(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    // "Region" for individual objects is the degenerate bounding sphere about
    // their center.
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return regionFromBounds(it->second.props.bounds());
}

float32 OHLocationServiceCache::maxSize(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    // Max size is just the size of the object.
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return maxSizeFromBounds(it->second.props.bounds());
}

bool OHLocationServiceCache::isLocal(const Iterator& id) {
    return true;
}

String OHLocationServiceCache::mesh(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    assert(it != mObjects.end());
    return it->second.props.mesh().toString();
}

Prox::ZernikeDescriptor& OHLocationServiceCache::zernikeDescriptor(const Iterator& id) {
  return Prox::ZernikeDescriptor::null();
}

const ObjectReference& OHLocationServiceCache::iteratorID(const Iterator& id) {
    // NOTE: Only accesses via iterator, shouldn't need a lock
    IteratorData* itdat = (IteratorData*)id.data;
    ObjectDataMap::iterator it = itdat->it;
    return it->first;
}

void OHLocationServiceCache::addUpdateListener(LocationUpdateListener* listener) {
    Lock lck(mMutex);

    assert( mListeners.find(listener) == mListeners.end() );
    mListeners.insert(listener);
}

void OHLocationServiceCache::removeUpdateListener(LocationUpdateListener* listener) {
    Lock lck(mMutex);

    ListenerSet::iterator it = mListeners.find(listener);
    assert( it != mListeners.end() );
    mListeners.erase(it);
}

#define GET_OBJ_ENTRY(objid) \
    Lock lck(mMutex); \
    ObjectDataMap::const_iterator it = mObjects.find(id);       \
    assert(it != mObjects.end())

TimedMotionVector3f OHLocationServiceCache::location(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.location();
}

TimedMotionQuaternion OHLocationServiceCache::orientation(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.orientation();
}

BoundingSphere3f OHLocationServiceCache::bounds(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.bounds();
}

float32 OHLocationServiceCache::radius(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.bounds().radius();
}

Transfer::URI OHLocationServiceCache::mesh(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.mesh();
}

String OHLocationServiceCache::physics(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props.physics();
}

const SequencedPresenceProperties& OHLocationServiceCache::properties(const ObjectID& id) {
    GET_OBJ_ENTRY(id);
    return it->second.props;
}

void OHLocationServiceCache::objectAdded(
    const ObjectReference& uuid, bool agg,
    const TimedMotionVector3f& loc, uint64 loc_seqno,
    const TimedMotionQuaternion& orient, uint64 orient_seqno,
    const BoundingSphere3f& bounds, uint64 bounds_seqno,
    const Transfer::URI& mesh, uint64 mesh_seqno,
    const String& physics, uint64 physics_seqno
) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it != mObjects.end()) return;

    it = mObjects.insert( ObjectDataMap::value_type(uuid, ObjectData()) ).first;
    it->second.props.setLocation(loc, loc_seqno);
    it->second.props.setOrientation(orient, orient_seqno);
    it->second.props.setBounds(bounds, bounds_seqno);
    it->second.props.setMesh(mesh, mesh_seqno);
    it->second.props.setPhysics(physics, physics_seqno);
    it->second.aggregate = agg;

    if (!agg) {
        it->second.tracking++;
        mStrand->post(
            std::tr1::bind(
                &OHLocationServiceCache::notifyObjectAdded, this,
                uuid, loc, bounds
            ),
            "OHLocationServiceCache::notifyObjectAdded"
        );
    }
}

void OHLocationServiceCache::notifyObjectAdded(
    const ObjectReference& uuid, const TimedMotionVector3f& loc,
    const BoundingSphere3f& bounds
) {
    Lock lck(mMutex);
    for(ListenerSet::iterator listener_it = mListeners.begin(); listener_it != mListeners.end(); listener_it++)
        (*listener_it)->locationConnected(uuid, true, loc, regionFromBounds(bounds), maxSizeFromBounds(bounds));
    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void OHLocationServiceCache::objectRemoved(const ObjectReference& uuid) {
    Lock lck(mMutex);

    ObjectDataMap::iterator data_it = mObjects.find(uuid);
    if (data_it == mObjects.end()) return;

    assert(data_it->second.exists);
    data_it->second.exists = false;
    bool agg = data_it->second.aggregate;

    tryRemoveObject(data_it);

    if (!agg) {
        data_it->second.tracking++;
        mStrand->post(
            std::tr1::bind(
                &OHLocationServiceCache::notifyObjectRemoved, this,
                uuid
            ),
            "OHLocationServiceCache::notifyObjectRemoved"
        );
    }
}

void OHLocationServiceCache::notifyObjectRemoved(const ObjectReference& uuid) {
    Lock lck(mMutex);
    for(ListenerSet::iterator listener_it = mListeners.begin(); listener_it != mListeners.end(); listener_it++)
        (*listener_it)->locationDisconnected(uuid);
    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void OHLocationServiceCache::locationUpdated(const ObjectReference& uuid, const TimedMotionVector3f& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    TimedMotionVector3f oldval = it->second.props.location();
    it->second.props.setLocation(newval, seqno);

    bool agg = it->second.aggregate;
    if (!agg) {
        it->second.tracking++;
        mStrand->post(
            std::tr1::bind(
                &OHLocationServiceCache::notifyLocationUpdated, this,
                uuid, oldval, newval
            ),
            "OHLocationServiceCache::notifyLocationUpdated"
        );
    }
}

void OHLocationServiceCache::notifyLocationUpdated(const ObjectReference& uuid, const TimedMotionVector3f& oldval, const TimedMotionVector3f& newval) {
    Lock lck(mMutex);
    for(ListenerSet::iterator listener_it = mListeners.begin(); listener_it != mListeners.end(); listener_it++)
        (*listener_it)->locationPositionUpdated(uuid, oldval, newval);
    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void OHLocationServiceCache::orientationUpdated(const ObjectReference& uuid, const TimedMotionQuaternion& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    it->second.props.setOrientation(newval, seqno);
}

void OHLocationServiceCache::boundsUpdated(const ObjectReference& uuid, const BoundingSphere3f& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    BoundingSphere3f oldval = it->second.props.bounds();
    it->second.props.setBounds(newval, seqno);

    bool agg = it->second.aggregate;
    if (!agg) {
        it->second.tracking++;
        mStrand->post(
            std::tr1::bind(
                &OHLocationServiceCache::notifyBoundsUpdated, this,
                uuid, oldval, newval
            ),
            "OHLocationServiceCache::notifyBoundsUpdated"
        );
    }
}

void OHLocationServiceCache::notifyBoundsUpdated(const ObjectReference& uuid, const BoundingSphere3f& oldval, const BoundingSphere3f& newval) {
    Lock lck(mMutex);
    for(ListenerSet::iterator listen_it = mListeners.begin(); listen_it != mListeners.end(); listen_it++) {
        (*listen_it)->locationRegionUpdated(uuid, regionFromBounds(oldval), regionFromBounds(newval));
        (*listen_it)->locationMaxSizeUpdated(uuid, maxSizeFromBounds(oldval), maxSizeFromBounds(newval));
    }
    ObjectDataMap::iterator obj_it = mObjects.find(uuid);
    obj_it->second.tracking--;
    tryRemoveObject(obj_it);
}

void OHLocationServiceCache::meshUpdated(const ObjectReference& uuid, const Transfer::URI& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;
    Transfer::URI oldval = it->second.props.mesh();
    it->second.props.setMesh(newval, seqno);
}

void OHLocationServiceCache::physicsUpdated(const ObjectReference& uuid, const String& newval, uint64 seqno) {
    Lock lck(mMutex);

    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;
    String oldval = it->second.props.physics();
    it->second.props.setPhysics(newval, seqno);
}

bool OHLocationServiceCache::tryRemoveObject(ObjectDataMap::iterator& obj_it) {
    if (obj_it->second.tracking > 0  || obj_it->second.exists)
        return false;

    mObjects.erase(obj_it);
    return true;
}

} // namespace Sirikata
