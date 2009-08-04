/*  cbr
 *  AlwaysLocationUpdatePolicy.hpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _ALWAYS_LOCATION_UPDATE_POLICY_HPP_
#define _ALWAYS_LOCATION_UPDATE_POLICY_HPP_

#include "LocationService.hpp"

namespace CBR {

/** A LocationUpdatePolicy which always sends a location
 *  update message to all subscribers on any position update.
 */
class AlwaysLocationUpdatePolicy : public LocationUpdatePolicy {
    virtual ~AlwaysLocationUpdatePolicy();

    virtual void subscribe(ServerID remote, const UUID& uuid);
    virtual void unsubscribe(ServerID remote, const UUID& uuid);
    virtual void unsubscribe(ServerID remote);

    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void localObjectRemoved(const UUID& uuid);
    virtual void localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);

    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);

    virtual void tick(const Time& t);

private:
    typedef std::set<UUID> UUIDSet;
    typedef std::set<ServerID> ServerIDSet;

    struct UpdateInfo {
        UpdateInfo()
         : hasLocation(false), hasBounds(false)
        {}

        bool hasLocation;
        TimedMotionVector3f location;
        bool hasBounds;
        BoundingSphere3f bounds;
    };

    struct ServerSubscriberInfo {
        UUIDSet subscribedTo;
        std::map<UUID, UpdateInfo> outstandingUpdates;
    };

    typedef std::map<ServerID, ServerSubscriberInfo*> ServerSubscriptionMap;
    ServerSubscriptionMap mServerSubscriptions;

    typedef std::map<UUID, ServerIDSet*> ObjectSubscribersMap;
    ObjectSubscribersMap mObjectSubscribers;
}; // class AlwaysLocationUpdatePolicy

} // namespace CBR

#endif //_ALWAYS_LOCATION_UPDATE_POLICY_HPP_
