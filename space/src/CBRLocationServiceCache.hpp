/*  Sirikata
 *  CBRLocationServiceCache.hpp
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

#ifndef _SIRIKATA_CBR_LOCATION_SERVICE_CACHE_HPP_
#define _SIRIKATA_CBR_LOCATION_SERVICE_CACHE_HPP_

#include <sirikata/space/ProxSimulationTraits.hpp>
#include <sirikata/space/LocationService.hpp>
#include <prox/LocationServiceCache.hpp>

namespace Sirikata {

/* Implementation of LocationServiceCache which serves Prox libraries;
 * works by listening for updates from our LocationService.  Note that
 * CBR should only be using the LocationServiceListener methods in normal
 * operation -- all other threads are to be used by libprox classes and
 * will only be accessed in the proximity thread. Therefore, most of the
 * work happens in the proximity thread, with the callbacks just storing
 * information to be picked up in the next iteration.
 */
class CBRLocationServiceCache : public Prox::LocationServiceCache<ObjectProxSimulationTraits>, public LocationServiceListener {
public:
    typedef Prox::LocationUpdateListener<ObjectProxSimulationTraits> LocationUpdateListener;

    /** Constructs a CBRLocationServiceCache which caches entries from locservice.  If
     *  replicas is true, then it caches replica entries from locservice, in addition
     *  to the local entries it always caches.
     */
    CBRLocationServiceCache(Network::IOStrand* strand, LocationService* locservice, bool replicas);
    virtual ~CBRLocationServiceCache();

    /* LocationServiceCache members. */
    virtual Iterator startTracking(const ObjectID& id);
    virtual void stopTracking(const Iterator& id);

    bool tracking(const ObjectID& id) const;

    virtual const TimedMotionVector3f& location(const Iterator& id) const;
    virtual const BoundingSphere3f& region(const Iterator& id) const;
    virtual float32 maxSize(const Iterator& id) const;

    virtual const UUID& iteratorID(const Iterator& id) const;

    virtual void addUpdateListener(LocationUpdateListener* listener);
    virtual void removeUpdateListener(LocationUpdateListener* listener);

    // We also provide accessors by ID for Proximity generate results.
    const TimedMotionVector3f& location(const ObjectID& id) const;
    const TimedMotionQuaternion& orientation(const ObjectID& id) const;
    const BoundingSphere3f& bounds(const ObjectID& id) const;
    float32 radius(const ObjectID& id) const;
    const String& mesh(const ObjectID& id) const;

    /* LocationServiceListener members. */
    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void localObjectRemoved(const UUID& uuid);
    virtual void localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void localOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void localMeshUpdated(const UUID& uuid, const String& newval);
    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void replicaMeshUpdated(const UUID& uuid, const String& newval);

private:

    // These generate and queue up updates from the main thread
    void objectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    void objectRemoved(const UUID& uuid);
    void locationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    void orientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    void boundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    void meshUpdated(const UUID& uuid, const String& newval);

    // These do the actual work for the LocationServiceListener methods.  Local versions always
    // call these, replica versions only call them if replica tracking is on
    void processObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    void processObjectRemoved(const UUID& uuid);
    void processLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    void processOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    void processBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    void processMeshUpdated(const UUID& uuid, const String& newval);

    CBRLocationServiceCache();

    Network::IOStrand* mStrand;
    LocationService* mLoc;

    typedef std::set<LocationUpdateListener*> ListenerSet;
    ListenerSet mListeners;

    // Object data is only accessed in the prox thread (by libprox
    // and by this class when updates are passed by the main thread).
    // Therefore, this data does *NOT* need to be locked for access.
    struct ObjectData {
        TimedMotionVector3f location;
        TimedMotionQuaternion orientation;
        // The raw bounding volume.
        BoundingSphere3f bounds;
        // "Region" is the center of the object's bounding region, with 0 size
        // for the bounding sphere.
        BoundingSphere3f region;
        // MaxSize is the size of the object, stored upon bounding region updates.
        float32 maxSize;
        String mesh;
        bool tracking;
    };
    typedef std::tr1::unordered_map<UUID, ObjectData, UUID::Hasher> ObjectDataMap;
    ObjectDataMap mObjects;
    bool mWithReplicas;

    // Data contained in our Iterators. We maintain both the UUID and the
    // iterator because the iterator can become invalidated due to ordering of
    // events in the prox thread.
    struct IteratorData {
        IteratorData(const UUID& _objid, ObjectDataMap::iterator _it)
         : objid(_objid), it(_it) {}

        const UUID objid;
        ObjectDataMap::iterator it;
    };

};

} // namespace Sirikata

#endif //_SIRIKATA_CBR_LOCATION_SERVICE_CACHE_HPP_
