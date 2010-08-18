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

#include <prox/LocationServiceCache.hpp>
#include <sirikata/space/ProxSimulationTraits.hpp>

namespace Sirikata {

/* Implementation of LocationServiceCache which tracks data reported by space
 * servers to Pinto master server.
 */
class PintoManagerLocationServiceCache : public Prox::LocationServiceCache<ServerProxSimulationTraits> {
public:
    PintoManagerLocationServiceCache();
    virtual ~PintoManagerLocationServiceCache();

    void addSpaceServer(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& region, float32 maxSize);
    void updateSpaceServerLocation(ServerID sid, const TimedMotionVector3f& loc);
    void updateSpaceServerRegion(ServerID sid, const BoundingSphere3f& region);
    void updateSpaceServerMaxSize(ServerID sid, float32 ms);
    void removeSpaceServer(ServerID sid);

    virtual Iterator startTracking(const ObjectID& id);
    virtual void stopTracking(const Iterator& id);

    virtual const TimedMotionVector3f& location(const Iterator& id) const;
    virtual const BoundingSphere3f& region(const Iterator& id) const;
    virtual float32 maxSize(const Iterator& id) const;

    virtual const ObjectID& iteratorID(const Iterator& id) const;

    virtual void addUpdateListener(LocationUpdateListenerType* listener);
    virtual void removeUpdateListener(LocationUpdateListenerType* listener);

private:
    struct SpaceServerData {
        TimedMotionVector3f location;
        BoundingSphere3f region;
        float32 maxSize;

        bool tracking;
        bool removable;
    };

    typedef std::tr1::unordered_map<ServerID, SpaceServerData> ServerMap;
    typedef std::tr1::unordered_set<LocationUpdateListenerType*> ListenerSet;

    ServerMap mServers;
    ListenerSet mListeners;
};

} // namespace Sirikata

#endif //_SIRIKATA_PINTO_PINTO_MANAGER_LOCATION_SERVICE_CACHE_HPP_
