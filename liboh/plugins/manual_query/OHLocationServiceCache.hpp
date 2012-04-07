// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_OH_LOCATION_SERVICE_CACHE_HPP_
#define _SIRIKATA_OH_OH_LOCATION_SERVICE_CACHE_HPP_

#include "ProxSimulationTraits.hpp"
#include <prox/base/LocationServiceCache.hpp>
#include <prox/base/ZernikeDescriptor.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/core/util/PresenceProperties.hpp>
#include <boost/thread.hpp>

namespace Sirikata {
namespace Network {
class IOStrand;
}

class OHLocationServiceCache;
typedef std::tr1::shared_ptr<OHLocationServiceCache> OHLocationServiceCachePtr;

/** Implementation of LocationServiceCache used on the object host. Tracks state
 *  using SequencedPresenceProperties and avoids duplicating the state -- it
 *  simply holds onto it until it is safe to release it.
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
class OHLocationServiceCache :
        public Prox::LocationServiceCache<ObjectProxSimulationTraits> {
public:
    typedef Prox::LocationUpdateListener<ObjectProxSimulationTraits> LocationUpdateListener;

    OHLocationServiceCache(Network::IOStrand* strand);
    virtual ~OHLocationServiceCache();

    // External data input.
    void objectAdded(const ObjectReference& uuid, bool agg,
        const TimedMotionVector3f& loc, uint64 loc_seqno,
        const TimedMotionQuaternion& orient, uint64 orient_seqno,
        const BoundingSphere3f& bounds, uint64 bounds_seqno,
        const Transfer::URI& mesh, uint64 mesh_seqno,
        const String& physics, uint64 physics_seqno);
    void objectRemoved(const ObjectReference& uuid);
    void locationUpdated(const ObjectReference& uuid, const TimedMotionVector3f& newval, uint64 seqno);
    void orientationUpdated(const ObjectReference& uuid, const TimedMotionQuaternion& newval, uint64 seqno);
    void boundsUpdated(const ObjectReference& uuid, const BoundingSphere3f& newval, uint64 seqno);
    void meshUpdated(const ObjectReference& uuid, const Transfer::URI& newval, uint64 seqno);
    void physicsUpdated(const ObjectReference& uuid, const String& newval, uint64 seqno);

    /* LocationServiceCache members. */
    virtual Iterator startTracking(const ObjectID& id);
    virtual void stopTracking(const Iterator& id);

    bool tracking(const ObjectID& id);

    virtual TimedMotionVector3f location(const Iterator& id);
    virtual BoundingSphere3f region(const Iterator& id);
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
    TimedMotionVector3f location(const ObjectID& id);
    TimedMotionQuaternion orientation(const ObjectID& id);
    BoundingSphere3f bounds(const ObjectID& id);
    float32 radius(const ObjectID& id);
    Transfer::URI mesh(const ObjectID& id);
    String physics(const ObjectID& id);
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
    void notifyObjectAdded(const ObjectReference& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    void notifyObjectRemoved(const ObjectReference& uuid);
    void notifyLocationUpdated(const ObjectReference& uuid, const TimedMotionVector3f& oldval, const TimedMotionVector3f& newval);
    void notifyBoundsUpdated(const ObjectReference& uuid, const BoundingSphere3f& oldval, const BoundingSphere3f& newval);


    OHLocationServiceCache();

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
         : exists(true),
           tracking(0)
        {}

        SequencedPresenceProperties props;
        bool aggregate;

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

#endif //_SIRIKATA_OH_OH_LOCATION_SERVICE_CACHE_HPP_
