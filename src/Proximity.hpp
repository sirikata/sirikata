/*  cbr
 *  Proximity.hpp
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

#ifndef _CBR_PROXIMITY_HPP_
#define _CBR_PROXIMITY_HPP_

#include "ProxSimulationTraits.hpp"
#include "CBRLocationServiceCache.hpp"
#include "ServerNetwork.hpp"
#include "CoordinateSegmentation.hpp"
#include <prox/QueryHandler.hpp>
#include <prox/LocationUpdateListener.hpp>

namespace CBR {

class ProximityEventInfo {
public:
    enum Type {
        Entered = 1,
        Exited = 2
    };

    ProximityEventInfo(const UUID& q, const UUID& obj, TimedMotionVector3f location,Type t)
      : mQuery(q), mObject(obj), mLocation(location), mType(t) {}

    ProximityEventInfo(const UUID& q, const UUID& obj, Type t)
        : mQuery(q), mObject(obj), mType(t) {assert(t==Exited);}

    const UUID& query() const {
        return mQuery;
    }
    const TimedMotionVector3f& location() const {
        return mLocation;
    }

    const UUID& object() const {
        return mObject;
    }

    Type type() const {
        return mType;
    }
private:
    ProximityEventInfo();

    UUID mQuery;
    UUID mObject;
    TimedMotionVector3f mLocation;
    Type mType;
}; // class ProximityEvent

class LocationService;

class Proximity : Prox::QueryEventListener<ProxSimulationTraits>, LocationServiceListener, CoordinateSegmentation::Listener, MessageRecipient {
public:
    typedef Prox::Query<ProxSimulationTraits> Query;
    typedef Prox::QueryEvent<ProxSimulationTraits> QueryEvent;

    Proximity(SpaceContext* ctx, LocationService* locservice);
    ~Proximity();

    // Initialize prox.  Must be called after everything else (specifically message router) is set up since it
    // needs to send messages.
    void initialize(CoordinateSegmentation* cseg);

    // Server queries
    void updateQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa);
    void removeQuery(ServerID sid);

    // Objects
    void addQuery(UUID obj, SolidAngle sa);
    void removeQuery(UUID obj);

    // Update queries based on current state.  FIXME add event output
    void service(std::queue<ProximityEventInfo>& events);

    // QueryEventListener Interface
    void queryHasEvents(Query* query);

    // LocationServiceListener Interface
    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void localObjectRemoved(const UUID& uuid);
    virtual void localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);

    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg);

    // MessageRecipient Interface
    virtual void receiveMessage(Message* msg);
private:
    // Send a query add/update request to all the other servers
    void sendQueryRequests();

    typedef std::set<UUID> ObjectSet;
    typedef std::map<ServerID, Query*> ServerQueryMap;
    typedef std::map<UUID, Query*> ObjectQueryMap;

    SpaceContext* mContext;

    LocationService* mLocService;
    CoordinateSegmentation* mCSeg;

    // These track local objects and answer queries from other
    // servers.
    ServerQueryMap mServerQueries;
    CBRLocationServiceCache* mLocalLocCache;
    Prox::QueryHandler<ProxSimulationTraits>* mServerQueryHandler;

    // These track all objects being reported to this server and
    // answer queries for objects connected to this server.
    ObjectQueryMap mObjectQueries;
    CBRLocationServiceCache* mGlobalLocCache;
    Prox::QueryHandler<ProxSimulationTraits>* mObjectQueryHandler;

    // This tracks the minimum object query size, which is used
    // as the angle for queries to other servers.
    SolidAngle mMinObjectQueryAngle;

}; //class Proximity

} // namespace CBR

#endif //_CBR_PROXIMITY_HPP_
