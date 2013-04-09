/*  Sirikata
 *  StandardLocationService.cpp
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
 *  * Neither the name of Sirikata nor the names of its contributors may
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

#include "StandardLocationService.hpp"
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/command/Commander.hpp>

#include "Protocol_Loc.pbj.hpp"

namespace Sirikata {

StandardLocationService::StandardLocationService(SpaceContext* ctx, LocationUpdatePolicy* update_policy)
 : LocationService(ctx, update_policy)
{
}

bool StandardLocationService::contains(const UUID& uuid) const {
    return (mLocations.find(uuid) != mLocations.end());
}

bool StandardLocationService::isLocal(const UUID& uuid) const {
    LocationMap::const_iterator it = mLocations.find(uuid);
    if (it == mLocations.end())
        return false;
    return it->second.local;
}

void StandardLocationService::service() {
    mUpdatePolicy->service();
}

uint64 StandardLocationService::epoch(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.props.maxSeqNo();
}

TimedMotionVector3f StandardLocationService::location(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.props.location();
}

Vector3f StandardLocationService::currentPosition(const UUID& uuid) {
    TimedMotionVector3f loc = location(uuid);
    Vector3f returner = loc.extrapolate(mContext->simTime()).position();
    return returner;
}

TimedMotionQuaternion StandardLocationService::orientation(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.props.orientation();
}

Quaternion StandardLocationService::currentOrientation(const UUID& uuid) {
    TimedMotionQuaternion orient = orientation(uuid);
    return orient.extrapolate(mContext->simTime()).position();
}

AggregateBoundingInfo StandardLocationService::bounds(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.props.bounds();
}

const String& StandardLocationService::mesh(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    locinfo.mesh_copied_str = locinfo.props.mesh().toString();
    return locinfo.mesh_copied_str;
}

const String& StandardLocationService::physics(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    locinfo.physics_copied_str = locinfo.props.physics();
    return locinfo.physics_copied_str;
}

const String& StandardLocationService::queryData(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    locinfo.query_data_copied_str = locinfo.props.queryData();
    return locinfo.query_data_copied_str;
}

  void StandardLocationService::addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bnds, const String& msh, const String& phy, const String& query_data) {
    LocationMap::iterator it = mLocations.find(uuid);

    // Add or update the information to the cache
    if (it == mLocations.end()) {
        mLocations[uuid] = LocationInfo();
        it = mLocations.find(uuid);
    } else {
        // It was already in there as a replica, notify its removal
        assert(it->second.local == false);
        CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, false, TimedMotionVector3f()); // FIXME remote server ID
        notifyReplicaObjectRemoved(uuid);
    }

    LocationInfo& locinfo = it->second;
    locinfo.props.reset();
    locinfo.props.setLocation(loc, 0);
    locinfo.props.setOrientation(orient, 0);
    locinfo.props.setBounds(bnds, 0);
    locinfo.props.setMesh(Transfer::URI(msh), 0);
    locinfo.props.setPhysics(phy, 0);
    locinfo.props.setQueryData(query_data, 0);
    locinfo.local = true;
    locinfo.aggregate = false;

    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    // Add to the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, true, loc);
    notifyLocalObjectAdded(uuid, false, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid), query_data);
}

void StandardLocationService::removeLocalObject(const UUID& uuid, const std::tr1::function<void()>&completeCallback) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == false );
    mLocations.erase(uuid);

    // Remove from the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, false, TimedMotionVector3f());
    notifyLocalObjectRemoved(uuid, false);
    completeCallback();
    // FIXME we might want to give a grace period where we generate a replica if one isn't already there,
    // instead of immediately removing all traces of the object.
    // However, this needs to be handled carefully, prefer updates from another server, and expire
    // automatically.
}

void StandardLocationService::addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bnds, const String& msh, const String& phy, const String& query_data) {
    // Aggregates get randomly assigned IDs -- if there's a conflict either we
    // got a true conflict (incredibly unlikely) or somebody (prox/query
    // handler) screwed up.
    assert(mLocations.find(uuid) == mLocations.end());

    mLocations[uuid] = LocationInfo();
    LocationMap::iterator it = mLocations.find(uuid);

    LocationInfo& locinfo = it->second;
    locinfo.props.reset();
    locinfo.props.setLocation(loc, 0);
    locinfo.props.setOrientation(orient, 0);
    locinfo.props.setBounds(bnds, 0);
    locinfo.props.setMesh(Transfer::URI(msh), 0);
    locinfo.props.setPhysics(phy, 0);
    locinfo.props.setQueryData(query_data, 0);

    locinfo.local = true;
    locinfo.aggregate = true;

    // Add to the list of local objects
    notifyLocalObjectAdded(uuid, true, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid), "");
}

void StandardLocationService::removeLocalAggregateObject(const UUID& uuid, const std::tr1::function<void()>&completeCallback) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == true );
    mLocations.erase(uuid);

    notifyLocalObjectRemoved(uuid, true);
    completeCallback();
}

void StandardLocationService::updateLocalAggregateLocation(const UUID& uuid, const TimedMotionVector3f& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.props.setLocation(newval, 0);
    notifyLocalLocationUpdated( uuid, true, newval );
}
void StandardLocationService::updateLocalAggregateOrientation(const UUID& uuid, const TimedMotionQuaternion& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.props.setOrientation(newval, 0);
    notifyLocalOrientationUpdated( uuid, true, newval );
}
void StandardLocationService::updateLocalAggregateBounds(const UUID& uuid, const AggregateBoundingInfo& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.props.setBounds(newval);
    notifyLocalBoundsUpdated( uuid, true, newval );
}
void StandardLocationService::updateLocalAggregateMesh(const UUID& uuid, const String& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.props.setMesh(Transfer::URI(newval));
    notifyLocalMeshUpdated( uuid, true, newval );
}
void StandardLocationService::updateLocalAggregatePhysics(const UUID& uuid, const String& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.props.setPhysics(newval, 0);
    notifyLocalPhysicsUpdated( uuid, true, newval );
}
void StandardLocationService::updateLocalAggregateQueryData(const UUID& uuid, const String& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.props.setQueryData(newval);
    notifyLocalQueryDataUpdated( uuid, true, newval );
}

void StandardLocationService::addReplicaObject(const Time& t, const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bnds, const String& msh, const String& phy, const String& query_data) {
    // FIXME we should do checks on timestamps to decide which setting is "more" sane
    LocationMap::iterator it = mLocations.find(uuid);

    if (it != mLocations.end()) {
        // It already exists. If its local, ignore the update. If its another replica, somethings out of sync, but perform the update anyway
        LocationInfo& locinfo = it->second;
        if (!locinfo.local) {
            locinfo.props.reset();
            locinfo.props.setLocation(loc, 0);
            locinfo.props.setOrientation(orient, 0);
            locinfo.props.setBounds(bnds, 0);
            locinfo.props.setMesh(Transfer::URI(msh), 0);
            locinfo.props.setPhysics(phy, 0);
            locinfo.props.setQueryData(query_data, 0);

            //local = false
            // FIXME should we notify location and bounds updated info?
        }
        // else ignore
    }
    else {
        // Its a new replica, just insert it
        LocationInfo locinfo;
        locinfo.props.reset();
        locinfo.props.setLocation(loc, 0);
        locinfo.props.setOrientation(orient, 0);
        locinfo.props.setBounds(bnds, 0);
        locinfo.props.setMesh(Transfer::URI(msh), 0);
        locinfo.props.setPhysics(phy, 0);
        locinfo.props.setQueryData(query_data, 0);
        locinfo.local = false;
        locinfo.aggregate = agg;
        mLocations[uuid] = locinfo;

        // We only run this notification when the object actually is new
        CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, true, loc); // FIXME add remote server ID
        notifyReplicaObjectAdded(uuid, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid), query_data);
    }
}

void StandardLocationService::removeReplicaObject(const Time& t, const UUID& uuid) {
    // FIXME we should maintain some time information and check t against it to make sure this is sane

    LocationMap::iterator it = mLocations.find(uuid);
    if (it == mLocations.end())
        return;

    // If the object is marked as local, this is out of date information.  Just ignore it.
    LocationInfo& locinfo = it->second;
    if (locinfo.local)
        return;

    // Otherwise, remove and notify
    mLocations.erase(uuid);
    CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, false, TimedMotionVector3f()); // FIXME add remote server ID
    notifyReplicaObjectRemoved(uuid);
}


void StandardLocationService::receiveMessage(Message* msg) {
    assert(msg->dest_port() == SERVER_PORT_LOCATION);
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    bool parsed = parsePBJMessage(&contents, msg->payload());

    if (parsed) {
        for(int32 idx = 0; idx < contents.update_size(); idx++) {
            Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

            notifyOnLocationUpdateFromServer(msg->source_server(), update);

            // Its possible we'll get an out of date update. We only use this update
            // if (a) we have this object marked as a replica object and (b) we don't
            // have this object marked as a local object
            if (!contains(update.object()) || isLocal(update.object()))
                continue;

            LocationMap::iterator loc_it = mLocations.find( update.object() );
            // We can safely make this assertion right now because space server
            // to space server loc and prox are on the same reliable channel. If
            // this goes away then a) we can't make this assertion and b) we
            // need an OrphanLocUpdateManager to save updates where this
            // condition is false so they can be applied once the prox update
            // arrives.
            assert(loc_it != mLocations.end());

            uint64 epoch = 0;
            if (update.has_epoch())
                epoch = update.epoch();

            if (update.has_location()) {
                TimedMotionVector3f newloc(
                    update.location().t(),
                    MotionVector3f( update.location().position(), update.location().velocity() )
                );
                loc_it->second.props.setLocation(newloc, epoch);
                notifyReplicaLocationUpdated( update.object(), loc_it->second.props.location() );

                CONTEXT_SPACETRACE(serverLoc, msg->source_server(), mContext->id(), update.object(), loc_it->second.props.location() );
            }

            if (update.has_orientation()) {
                TimedMotionQuaternion neworient(
                    update.orientation().t(),
                    MotionQuaternion( update.orientation().position(), update.orientation().velocity() )
                );
                loc_it->second.props.setOrientation(neworient, epoch);
                notifyReplicaOrientationUpdated( update.object(), loc_it->second.props.orientation() );
            }

            if (update.has_aggregate_bounds()) {
                Vector3f center = update.aggregate_bounds().has_center_offset() ? update.aggregate_bounds().center_offset() : Vector3f(0,0,0);
                float32 center_rad = update.aggregate_bounds().has_center_bounds_radius() ? update.aggregate_bounds().center_bounds_radius() : 0.f;
                float32 max_object_size = update.aggregate_bounds().has_max_object_size() ? update.aggregate_bounds().max_object_size() : 0.f;

                AggregateBoundingInfo newbounds(center, center_rad, max_object_size);
                loc_it->second.props.setBounds(newbounds, epoch);
                notifyReplicaBoundsUpdated( update.object(), loc_it->second.props.bounds() );
            }

            if (update.has_mesh()) {
                String newmesh = update.mesh();
                loc_it->second.props.setMesh(Transfer::URI(newmesh), epoch);
                notifyReplicaMeshUpdated( update.object(), loc_it->second.props.mesh().toString() );
            }

            if (update.has_physics()) {
                String newphy = update.physics();
                loc_it->second.props.setPhysics(newphy, epoch);
                notifyReplicaPhysicsUpdated( update.object(), loc_it->second.props.physics() );
            }

            if (update.has_query_data()) {
                String newqd = update.query_data();
                loc_it->second.props.setQueryData(newqd, epoch);
                notifyReplicaQueryDataUpdated( update.object(), loc_it->second.props.queryData() );
            }

        }
    }

    delete msg;
}

bool StandardLocationService::locationUpdate(UUID source, void* buffer, uint32 length) {
    Sirikata::Protocol::Loc::Container loc_container;
    bool parse_success = loc_container.ParseFromString( String((char*) buffer, length) );
    if (!parse_success) {
        // Since we don't have framing on these, we can't really log this since
        // some packets will require more than SST's 1000 byte default packet
        // size and it would spam the console.
        //LOG_INVALID_MESSAGE_BUFFER(standardloc, error, ((char*)buffer), length);
        return false;
    }

    if (loc_container.has_update_request()) {
        Sirikata::Protocol::Loc::LocationUpdateRequest request = loc_container.update_request();

        if (isLocal(source)) {
            LocationMap::iterator loc_it = mLocations.find( source );
            assert(loc_it != mLocations.end());

            uint64 epoch = 0;
            if (request.has_epoch())
                epoch = request.epoch();

            if (request.has_location()) {
                TimedMotionVector3f newloc(
                    request.location().t(),
                    MotionVector3f( request.location().position(), request.location().velocity() )
                );
                loc_it->second.props.setLocation(newloc, epoch);
                notifyLocalLocationUpdated( source, loc_it->second.aggregate, loc_it->second.props.location() );

                CONTEXT_SPACETRACE(serverLoc, mContext->id(), mContext->id(), source, loc_it->second.props.location() );
            }

            if (request.has_orientation()) {
                TimedMotionQuaternion neworient(
                    request.orientation().t(),
                    MotionQuaternion( request.orientation().position(), request.orientation().velocity() )
                );
                loc_it->second.props.setOrientation(neworient, epoch);
                notifyLocalOrientationUpdated( source, loc_it->second.aggregate, loc_it->second.props.orientation() );
            }

            if (request.has_bounds()) {
                AggregateBoundingInfo newbounds(request.bounds());
                loc_it->second.props.setBounds(newbounds, epoch);
                notifyLocalBoundsUpdated( source, loc_it->second.aggregate, loc_it->second.props.bounds() );
            }

            if (request.has_mesh()) {
                String newmesh = request.mesh();
                loc_it->second.props.setMesh(Transfer::URI(newmesh), epoch);
                notifyLocalMeshUpdated( source, loc_it->second.aggregate, loc_it->second.props.mesh().toString() );
            }

            if (request.has_physics()) {
                String newphy = request.physics();
                loc_it->second.props.setPhysics(newphy, epoch);
                notifyLocalPhysicsUpdated( source, loc_it->second.aggregate, loc_it->second.props.physics() );
            }

            if (request.has_query_data()) {
                String newqd = request.query_data();
                loc_it->second.props.setQueryData(newqd, epoch);
                notifyLocalQueryDataUpdated( source, loc_it->second.aggregate, loc_it->second.props.queryData() );
            }

        }
        else {
            // Warn about update to non-local object
        }
    }
    return true;
}




// Command handlers

void StandardLocationService::commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    result.put("name", "standard");

    // FIXME we could track these as we add/remove objects so they'd be cheaper
    // to compute
    uint32 local_count = 0, aggregate_count = 0, local_aggregate_count = 0;
    for(LocationMap::iterator it = mLocations.begin(); it != mLocations.end(); it++) {
        if (it->second.local) local_count++;
        if (it->second.aggregate) aggregate_count++;
        if (it->second.local && it->second.aggregate) local_aggregate_count++;
    }
    result.put("objects.count", mLocations.size());
    result.put("objects.local_count", local_count);
    result.put("objects.aggregate_count", aggregate_count);
    result.put("objects.local_aggregate_count", local_aggregate_count);

    cmdr->result(cmdid, result);
}

void StandardLocationService::commandObjectProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    String obj_string = cmd.getString("object", "");
    if (obj_string.empty()) { // not specified
        result.put("error", "Ill-formatted request: no object specified.");
        cmdr->result(cmdid, result);
        return;
    }
    UUID objid(obj_string, UUID::HumanReadable());

    LocationMap::iterator it = mLocations.find(objid);
    if (it == mLocations.end()) {
        result.put("error", "Object not found.");
        cmdr->result(cmdid, result);
        return;
    }

    Time t = mContext->recentSimTime();
    TimedMotionVector3f pos(t, it->second.props.location().extrapolate(t));
    result.put("properties.location.position.x", pos.position().x);
    result.put("properties.location.position.y", pos.position().y);
    result.put("properties.location.position.z", pos.position().z);
    result.put("properties.location.velocity.x", pos.velocity().x);
    result.put("properties.location.velocity.y", pos.velocity().y);
    result.put("properties.location.velocity.z", pos.velocity().z);
    result.put("properties.location.time", pos.updateTime().raw());

    TimedMotionQuaternion orient(t, it->second.props.orientation().extrapolate(t));
    result.put("properties.orientation.position.x", orient.position().x);
    result.put("properties.orientation.position.y", orient.position().y);
    result.put("properties.orientation.position.z", orient.position().z);
    result.put("properties.orientation.position.w", orient.position().w);
    result.put("properties.orientation.velocity.x", orient.velocity().x);
    result.put("properties.orientation.velocity.y", orient.velocity().y);
    result.put("properties.orientation.velocity.z", orient.velocity().z);
    result.put("properties.orientation.velocity.w", orient.velocity().w);
    result.put("properties.orientation.time", orient.updateTime().raw());

    // Present the bounds info in a way the consumer can easily draw a bounding
    // sphere, especially for single objects
    result.put("properties.bounds.center.x", it->second.props.bounds().centerOffset.x);
    result.put("properties.bounds.center.y", it->second.props.bounds().centerOffset.y);
    result.put("properties.bounds.center.z", it->second.props.bounds().centerOffset.z);
    result.put("properties.bounds.radius", it->second.props.bounds().fullRadius());
    // But also provide the other info so a correct view of
    // aggregates. Technically we only need one of these since fullRadius is the
    // sum of the two, but this is clearer.
    result.put("properties.bounds.centerBoundsRadius", it->second.props.bounds().centerBoundsRadius);
    result.put("properties.bounds.maxObjectRadius", it->second.props.bounds().maxObjectRadius);

    result.put("properties.mesh", it->second.props.mesh().toString());
    result.put("properties.physics", it->second.props.physics());

    result.put("properties.local", it->second.local);
    result.put("properties.aggregate", it->second.aggregate);

    result.put("success", true);
    cmdr->result(cmdid, result);
}


} // namespace Sirikata
