/*  cbr
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

#ifndef _CBR_CBR_LOCATION_SERVICE_CACHE_HPP_
#define _CBR_CBR_LOCATION_SERVICE_CACHE_HPP_

#include "ProxSimulationTraits.hpp"
#include "LocationService.hpp"
#include <prox/LocationServiceCache.hpp>

namespace CBR {

/* Implementation of LocationServiceCache which serves Prox libraries;
 * works by listening for updates from our LocationService.
 */
class CBRLocationServiceCache : public Prox::LocationServiceCache<ProxSimulationTraits>, public LocationServiceListener {
public:
    typedef Prox::LocationUpdateListener<ProxSimulationTraits> LocationUpdateListener;

    CBRLocationServiceCache(LocationService* locservice);
    virtual ~CBRLocationServiceCache();

    /* LocationServiceCache members. */
    virtual void startTracking(const ObjectID& id);
    virtual void stopTracking(const ObjectID& id);

    virtual const TimedMotionVector3f& location(const ObjectID& id) const;
    virtual const BoundingSphere3f& bounds(const ObjectID& id) const;

    virtual void addUpdateListener(LocationUpdateListener* listener);
    virtual void removeUpdateListener(LocationUpdateListener* listener);

    /* LocationServiceListener members. */
    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void localObjectRemoved(const UUID& uuid);
    virtual void locationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void boundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
private:
    CBRLocationServiceCache();

    LocationService* mLoc;

    typedef std::set<LocationUpdateListener*> ListenerSet;
    ListenerSet mListeners;

    struct ObjectData {
        TimedMotionVector3f location;
        BoundingSphere3f bounds;
    };
    typedef std::map<UUID, ObjectData> ObjectDataMap;
    ObjectDataMap mObjects;
};

} // namespace CBR

#endif //_CBR_CBR_LOCATION_SERVICE_CACHE_HPP_
