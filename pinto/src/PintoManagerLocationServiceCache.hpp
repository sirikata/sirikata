/*  Sirikata
 *  PintoManagerLocationServiceCache.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_PINTO_PINTO_MANAGER_LOCATION_SERVICE_CACHE_HPP_
#define _SIRIKATA_PINTO_PINTO_MANAGER_LOCATION_SERVICE_CACHE_HPP_

#include <prox/base/LocationServiceCache.hpp>
#include "ProxSimulationTraits.hpp"
#include <boost/thread.hpp>
#include <sirikata/core/util/AggregateBoundingInfo.hpp>

namespace Sirikata {

/* Implementation of LocationServiceCache which tracks data reported by space
 * servers to Pinto master server.
 */
class PintoManagerLocationServiceCache : public Prox::LocationServiceCache<ServerProxSimulationTraits> {
public:
    PintoManagerLocationServiceCache();
    virtual ~PintoManagerLocationServiceCache();

    // Leaf inputs -- real space servers
    void addSpaceServer(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& region, float32 maxSize);
    void updateSpaceServerLocation(ServerID sid, const TimedMotionVector3f& loc);
    void updateSpaceServerRegion(ServerID sid, const BoundingSphere3f& region);
    void updateSpaceServerMaxSize(ServerID sid, float32 ms);
    void removeSpaceServer(ServerID sid);
    // Aggregates -- generated for aggregates
    void addAggregate(ServerID sid);
    void updateAggregateLocation(ServerID sid, const TimedMotionVector3f& loc);
    void updateAggregateBounds(ServerID sid, const AggregateBoundingInfo& bnds);
    void removeAggregate(ServerID sid);

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
    virtual bool startRefcountTracking(const ObjectID& id);
    virtual void stopRefcountTracking(const ObjectID& id);

    bool tracking(const ObjectID& id);

    virtual TimedMotionVector3f location(const Iterator& id);
    virtual Vector3 centerOffset(const Iterator& id);
    virtual float32 centerBoundsRadius(const Iterator& id);
    virtual float32 maxSize(const Iterator& id);
    virtual bool isLocal(const Iterator& id);
    Prox::ZernikeDescriptor& zernikeDescriptor(const Iterator& id);
    String mesh(const Iterator& id);
    bool aggregate(const Iterator& id);

    virtual const ObjectID& iteratorID(const Iterator& id);

    virtual void addUpdateListener(LocationUpdateListenerType* listener);
    virtual void removeUpdateListener(LocationUpdateListenerType* listener);

private:
    struct SpaceServerData {
        TimedMotionVector3f location;
        BoundingSphere3f region;
        float32 maxSize;

        bool aggregate;

        int16 tracking; // Refcount for multiple users -- query
                        // handlers and generating results
        bool removable;
    };

    typedef std::tr1::unordered_map<ServerID, SpaceServerData> ServerMap;
    typedef std::tr1::unordered_set<LocationUpdateListenerType*> ListenerSet;

    ServerMap mServers;
    ListenerSet mListeners;

    typedef boost::recursive_mutex Mutex;
    typedef boost::lock_guard<Mutex> Lock;
    Mutex mMutex;
};

} // namespace Sirikata

#endif //_SIRIKATA_PINTO_PINTO_MANAGER_LOCATION_SERVICE_CACHE_HPP_
