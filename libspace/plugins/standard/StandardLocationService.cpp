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

#include "Protocol_Loc.pbj.hpp"

namespace Sirikata {

StandardLocationService::StandardLocationService(SpaceContext* ctx, LocationUpdatePolicy* update_policy)
 : LocationService(ctx, update_policy)
{
}

bool StandardLocationService::contains(const UUID& uuid) const {
    return (mLocations.find(uuid) != mLocations.end());
}

LocationService::TrackingType StandardLocationService::type(const UUID& uuid) const {
    LocationMap::const_iterator it = mLocations.find(uuid);
    if (it == mLocations.end())
        return NotTracking;
    if (it->second.aggregate)
        return Aggregate;
    if (it->second.local)
        return Local;
    return Replica;
}


void StandardLocationService::service() {
    mUpdatePolicy->service();
}

TimedMotionVector3f StandardLocationService::location(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.location;
}

Vector3f StandardLocationService::currentPosition(const UUID& uuid) {
    TimedMotionVector3f loc = location(uuid);
    return loc.extrapolate(mContext->simTime()).position();
}

TimedMotionQuaternion StandardLocationService::orientation(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.orientation;
}

Quaternion StandardLocationService::currentOrientation(const UUID& uuid) {
    TimedMotionQuaternion orient = orientation(uuid);
    return orient.extrapolate(mContext->simTime()).position();
}

BoundingSphere3f StandardLocationService::bounds(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.bounds;
}

const String& StandardLocationService::mesh(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    const LocationInfo& locinfo = it->second;
    return locinfo.mesh;
}

void StandardLocationService::addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh) {
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
    locinfo.location = loc;
    locinfo.orientation = orient;
    locinfo.bounds = bnds;
    locinfo.mesh = msh;
    locinfo.local = true;
    locinfo.aggregate = false;

    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    // Add to the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, true, loc);
    notifyLocalObjectAdded(uuid, false, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid));
}

void StandardLocationService::removeLocalObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == false );
    mLocations.erase(uuid);

    // Remove from the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, false, TimedMotionVector3f());
    notifyLocalObjectRemoved(uuid, false);

    // FIXME we might want to give a grace period where we generate a replica if one isn't already there,
    // instead of immediately removing all traces of the object.
    // However, this needs to be handled carefully, prefer updates from another server, and expire
    // automatically.
}

void StandardLocationService::addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh) {
    // Aggregates get randomly assigned IDs -- if there's a conflict either we
    // got a true conflict (incredibly unlikely) or somebody (prox/query
    // handler) screwed up.
    assert(mLocations.find(uuid) == mLocations.end());

    mLocations[uuid] = LocationInfo();
    LocationMap::iterator it = mLocations.find(uuid);

    LocationInfo& locinfo = it->second;
    locinfo.location = loc;
    locinfo.orientation = orient;
    locinfo.bounds = bnds;
    locinfo.mesh = msh;
    locinfo.local = true;
    locinfo.aggregate = true;

    // Add to the list of local objects
    notifyLocalObjectAdded(uuid, true, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid));
}

void StandardLocationService::removeLocalAggregateObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == true );
    mLocations.erase(uuid);

    notifyLocalObjectRemoved(uuid, true);
}

void StandardLocationService::updateLocalAggregateLocation(const UUID& uuid, const TimedMotionVector3f& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.location = newval;
    notifyLocalLocationUpdated( uuid, true, newval );
}
void StandardLocationService::updateLocalAggregateOrientation(const UUID& uuid, const TimedMotionQuaternion& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.orientation = newval;
    notifyLocalOrientationUpdated( uuid, true, newval );
}
void StandardLocationService::updateLocalAggregateBounds(const UUID& uuid, const BoundingSphere3f& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.bounds = newval;
    notifyLocalBoundsUpdated( uuid, true, newval );
}
void StandardLocationService::updateLocalAggregateMesh(const UUID& uuid, const String& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.mesh = newval;
    notifyLocalMeshUpdated( uuid, true, newval );
}

void StandardLocationService::addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh) {
    // FIXME we should do checks on timestamps to decide which setting is "more" sane
    LocationMap::iterator it = mLocations.find(uuid);

    if (it != mLocations.end()) {
        // It already exists. If its local, ignore the update. If its another replica, somethings out of sync, but perform the update anyway
        LocationInfo& locinfo = it->second;
        if (!locinfo.local) {
            locinfo.location = loc;
            locinfo.orientation = orient;
            locinfo.bounds = bnds;
            locinfo.mesh = msh;
            //local = false
            // FIXME should we notify location and bounds updated info?
        }
        // else ignore
    }
    else {
        // Its a new replica, just insert it
        LocationInfo locinfo;
        locinfo.location = loc;
        locinfo.orientation = orient;
        locinfo.bounds = bnds;
        locinfo.mesh = msh;
        locinfo.local = false;
        locinfo.aggregate = false;
        mLocations[uuid] = locinfo;

        // We only run this notification when the object actually is new
        CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, true, loc); // FIXME add remote server ID
        notifyReplicaObjectAdded(uuid, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid));
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

            // Its possible we'll get an out of date update. We only use this update
            // if (a) we have this object marked as a replica object and (b) we don't
            // have this object marked as a local object
            if (type(update.object()) != Replica)
                continue;

            LocationMap::iterator loc_it = mLocations.find( update.object() );
            // We can safely make this assertion right now because space server
            // to space server loc and prox are on the same reliable channel. If
            // this goes away then a) we can't make this assertion and b) we
            // need an OrphanLocUpdateManager to save updates where this
            // condition is false so they can be applied once the prox update
            // arrives.
            assert(loc_it != mLocations.end());

            if (update.has_location()) {
                TimedMotionVector3f newloc(
                    update.location().t(),
                    MotionVector3f( update.location().position(), update.location().velocity() )
                );
                loc_it->second.location = newloc;
                notifyReplicaLocationUpdated( update.object(), newloc );

                CONTEXT_SPACETRACE(serverLoc, msg->source_server(), mContext->id(), update.object(), newloc );
            }

            if (update.has_orientation()) {
                TimedMotionQuaternion neworient(
                    update.orientation().t(),
                    MotionQuaternion( update.orientation().position(), update.orientation().velocity() )
                );
                loc_it->second.orientation = neworient;
                notifyReplicaOrientationUpdated( update.object(), neworient );
            }

            if (update.has_bounds()) {
                BoundingSphere3f newbounds = update.bounds();
                loc_it->second.bounds = newbounds;
                notifyReplicaBoundsUpdated( update.object(), newbounds );
            }

            if (update.has_mesh()) {
                String newmesh = update.mesh();
                loc_it->second.mesh = newmesh;
                notifyReplicaMeshUpdated( update.object(), newmesh );
            }
        }
    }

    delete msg;
}

void StandardLocationService::locationUpdate(UUID source, void* buffer, uint32 length) {
    Sirikata::Protocol::Loc::Container loc_container;
    bool parse_success = loc_container.ParseFromString( String((char*) buffer, length) );
    if (!parse_success) {
        LOG_INVALID_MESSAGE_BUFFER(standardloc, error, ((char*)buffer), length);
        return;
    }

    if (loc_container.has_update_request()) {
        Sirikata::Protocol::Loc::LocationUpdateRequest request = loc_container.update_request();

        TrackingType obj_type = type(source);
        if (obj_type == Local) {
            LocationMap::iterator loc_it = mLocations.find( source );
            assert(loc_it != mLocations.end());

            if (request.has_location()) {
                TimedMotionVector3f newloc(
                    request.location().t(),
                    MotionVector3f( request.location().position(), request.location().velocity() )
                );
                loc_it->second.location = newloc;
                notifyLocalLocationUpdated( source, loc_it->second.aggregate, newloc );

                CONTEXT_SPACETRACE(serverLoc, mContext->id(), mContext->id(), source, newloc );

            }

            if (request.has_bounds()) {
                BoundingSphere3f newbounds = request.bounds();
                loc_it->second.bounds = newbounds;
                notifyLocalBoundsUpdated( source, loc_it->second.aggregate, newbounds );
            }

            if (request.has_mesh()) {
                String newmesh = request.mesh();
                loc_it->second.mesh = newmesh;
                notifyLocalMeshUpdated( source, loc_it->second.aggregate, newmesh );
            }

            if (request.has_orientation()) {
                TimedMotionQuaternion neworient(
                    request.orientation().t(),
                    MotionQuaternion( request.orientation().position(), request.orientation().velocity() )
                );
                loc_it->second.orientation = neworient;
                notifyLocalOrientationUpdated( source, loc_it->second.aggregate, neworient );
            }

        }
        else {
            // Warn about update to non-local object
        }
    }
}



} // namespace Sirikata
