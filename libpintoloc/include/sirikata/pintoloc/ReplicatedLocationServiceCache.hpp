// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPINTO_REPLICATED_LOCATION_SERVICE_CACHE_HPP_
#define _SIRIKATA_LIBPINTO_REPLICATED_LOCATION_SERVICE_CACHE_HPP_

#include <sirikata/pintoloc/Platform.hpp>
#include <sirikata/pintoloc/ProxSimulationTraits.hpp>
#include <prox/base/LocationServiceCache.hpp>
#include <prox/base/ZernikeDescriptor.hpp>
#include <sirikata/core/util/PresenceProperties.hpp>
#include <sirikata/pintoloc/ReplicatedLocationUpdateListener.hpp>
#include <boost/thread.hpp>
#include <sirikata/core/network/IOStrand.hpp>

namespace Sirikata {

class ReplicatedLocationServiceCache;
typedef std::tr1::shared_ptr<ReplicatedLocationServiceCache> ReplicatedLocationServiceCachePtr;

/** Implementation of LocationServiceCache used for tracking replicated data
 *  from another node. Tracks state using SequencedPresenceProperties and avoids
 *  duplicating the state -- it simply holds onto it until it is safe to release
 *  it.
 *
 *  This version is thread-safe. It allows you to update (and access) the data
 *  from any thread. You provide a strand in which listeners are triggered,
 *  allowing you to control where events get triggered. This ensures the
 *  appropriate controls for libprox query handlers but allows synchronous
 *  access to the data.
 *
 *  To accomplish this, objects are refcounted and updates sent to the strand
 *  cause an increment in the refcount so that the object is guaranteed to
 *  remain available. The updated values are also passed through the post() to
 *  ensure the (non-thread-safe) data remains consistent when reported.
 */
class SIRIKATA_LIBPINTOLOC_EXPORT ReplicatedLocationServiceCache :
        public Prox::LocationServiceCache<ObjectProxSimulationTraits>,
        public ReplicatedLocationUpdateProvider
{
public:
    typedef Prox::LocationUpdateListener<ObjectProxSimulationTraits> LocationUpdateListener;

    ReplicatedLocationServiceCache(Network::IOStrandPtr strand);
    ReplicatedLocationServiceCache(Network::IOStrand* strand);
    virtual ~ReplicatedLocationServiceCache();

    // Returns true if this has no objects that still exist. Note that this
    // isn't quite like a regular empty() method. We might still be tracking
    // data, but only because something is asking for data to be tracked and
    // maintained -- according to the inputs, we've got no real, live data
    // left.
    bool empty();
    // Returns true if this is actually completely empty, i.e. it has no data in
    // it.
    bool fullyEmpty();

    // External data input.
    void objectAdded(const ObjectReference& uuid, bool agg,
        const ObjectReference& parent,
        const TimedMotionVector3f& loc, uint64 loc_seqno,
        const TimedMotionQuaternion& orient, uint64 orient_seqno,
        const AggregateBoundingInfo& bounds, uint64 bounds_seqno,
        const Transfer::URI& mesh, uint64 mesh_seqno,
        const String& physics, uint64 physics_seqno);
    void objectRemoved(const ObjectReference& uuid, bool temporary);
    void epochUpdated(const ObjectReference& uuid, const uint64 ep);
    void locationUpdated(const ObjectReference& uuid, const TimedMotionVector3f& newval, uint64 seqno);
    void orientationUpdated(const ObjectReference& uuid, const TimedMotionQuaternion& newval, uint64 seqno);
    void boundsUpdated(const ObjectReference& uuid, const AggregateBoundingInfo& newval, uint64 seqno);
    void meshUpdated(const ObjectReference& uuid, const Transfer::URI& newval, uint64 seqno);
    void physicsUpdated(const ObjectReference& uuid, const String& newval, uint64 seqno);
    void parentUpdated(const ObjectReference& uuid, const ObjectReference& newval, uint64 seqno);

    /* LocationServiceCache members. */

    virtual void addPlaceholderImposter(
        const ObjectID& uuid,
        const Vector3f& center_offset,
        const float32 center_bounds_radius,
        const float32 max_size,
        const String& zernike,
        const String& mesh
    );

    virtual Iterator startTracking(const ObjectID& id);
    virtual void stopTracking(const Iterator& id);
    // These are an alternative form of marking objects as
    // tracked. The do basically the same thing as above but are used
    // when you want to access properties by ObjectID rather than by
    // iterator.  startSimpleTracking is also safe to call even if
    // you're not sure the object is still available, and returns true
    // only if it is still available.
    bool startSimpleTracking(const ObjectID& id);
    void stopSimpleTracking(const ObjectID& id);

    bool tracking(const ObjectID& id);

    virtual TimedMotionVector3f location(const Iterator& id);
    virtual Vector3f centerOffset(const Iterator& id);
    virtual float32 centerBoundsRadius(const Iterator& id);
    virtual float32 maxSize(const Iterator& id);
    virtual bool isLocal(const Iterator& id);
    virtual String mesh(const Iterator& id);
    virtual Prox::ZernikeDescriptor& zernikeDescriptor(const Iterator& id);

    virtual const ObjectReference& iteratorID(const Iterator& id);

    virtual void addUpdateListener(LocationUpdateListener* listener);
    virtual void removeUpdateListener(LocationUpdateListener* listener);

    // We also provide accessors by ID for Proximity generate results.
    // Note: as with the LocationServiceCache interface, these return values to
    // allow for thread-safety.
    uint64 epoch(const ObjectID& id);
    TimedMotionVector3f location(const ObjectID& id);
    TimedMotionQuaternion orientation(const ObjectID& id);
    AggregateBoundingInfo bounds(const ObjectID& id);
    Transfer::URI mesh(const ObjectID& id);
    String physics(const ObjectID& id);
    ObjectReference parent(const ObjectID& id);
    bool aggregate(const ObjectID& id);
    // And raw access to the underlying SequencedPresenceProperties
    const SequencedPresenceProperties& properties(const ObjectID& id);

private:

    // These operate in the strand and do notification of events. They get
    // values passed through to ensure the right values are passed in the
    // notification (since additional updates might be handled before this
    // notification gets processed in the strand).
    // (Since these are only used for Prox LocationUpdateListener, we only need
    // to notify of a few of the events -- orientation, mesh, and physics are
    // all ignored.)
    void notifyObjectAdded(const ObjectReference& uuid, const ObjectReference& parent, bool agg, const TimedMotionVector3f& loc, const AggregateBoundingInfo& bounds);
    void notifyParentUpdated(const ObjectReference& uuid, const ObjectReference& oldval, const ObjectReference& newval);
    void notifyObjectRemoved(const ObjectReference& uuid, bool temporary);
    void notifyEpochUpdated(const ObjectReference& uuid, const uint64 val);
    void notifyLocationUpdated(const ObjectReference& uuid, const TimedMotionVector3f& oldval, const TimedMotionVector3f& newval);
    void notifyBoundsUpdated(const ObjectReference& uuid, const AggregateBoundingInfo& oldval, const AggregateBoundingInfo& newval);
    // These are only used by ReplicatedLocationUpdateListener so they don't need values
    // with them (see comments for that class)
    void notifyOrientationUpdated(const ObjectReference& uuid);
    void notifyMeshUpdated(const ObjectReference& uuid);
    void notifyPhysicsUpdated(const ObjectReference& uuid);


    ReplicatedLocationServiceCache();

    typedef boost::recursive_mutex Mutex;
    typedef boost::lock_guard<Mutex> Lock;
    Mutex mMutex;

    Network::IOStrand* mStrand;

    typedef std::set<LocationUpdateListener*> ListenerSet;
    ListenerSet mListeners;

    // Object data is only accessed in the prox thread (by libprox
    // and by this class when updates are passed by the main thread).
    // Therefore, this data does *NOT* need to be locked for access.
    struct ObjectData {
        ObjectData()
         : epoch(0),
           exists(true),
           tracking(0)
        {}

        SequencedPresenceProperties props;
        uint64 epoch; // Only valid for presences
        bool aggregate;
        ObjectReference parent;

        bool exists; // Exists, i.e. ObjectRemoved hasn't been called
        int16 tracking; // Ref count to support multiple users
    };
    typedef std::tr1::unordered_map<ObjectReference, ObjectData, ObjectReference::Hasher> ObjectDataMap;
    ObjectDataMap mObjects;

    bool tryRemoveObject(ObjectDataMap::iterator& obj_it);

    // Data contained in our Iterators. We maintain both the ObjectReference and the
    // iterator because the iterator can become invalidated due to ordering of
    // events in the prox thread.
    struct IteratorData {
        IteratorData(const ObjectReference& _objid, ObjectDataMap::iterator _it)
         : objid(_objid), it(_it) {}

        const ObjectReference objid;
        ObjectDataMap::iterator it;
    };

};

} // namespace Sirikata

#endif //_SIRIKATA_LIBPINTO_REPLICATED_LOCATION_SERVICE_CACHE_HPP_
