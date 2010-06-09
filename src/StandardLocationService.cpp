/*  cbr
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

#include "StandardLocationService.hpp"
#include "Statistics.hpp"

#include "CBR_Loc.pbj.hpp"

namespace Sirikata {

StandardLocationService::StandardLocationService(SpaceContext* ctx)
 : LocationService(ctx)
{
}

bool StandardLocationService::contains(const UUID& uuid) const {
    return (mLocations.find(uuid) != mLocations.end());
}

LocationService::TrackingType StandardLocationService::type(const UUID& uuid) const {
    LocationMap::const_iterator it = mLocations.find(uuid);
    if (it == mLocations.end())
        return NotTracking;
    if (it->second.local)
        return Local;
    else
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

BoundingSphere3f StandardLocationService::bounds(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.bounds;
}

void StandardLocationService::addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bnds) {
    LocationMap::iterator it = mLocations.find(uuid);

    // Add or update the information to the cache
    if (it == mLocations.end()) {
        mLocations[uuid] = LocationInfo();
        it = mLocations.find(uuid);
    } else {
        // It was already in there as a replica, notify its removal

        assert(it->second.local == false);
        CONTEXT_TRACE(serverObjectEvent, 0, mContext->id(), uuid, false, TimedMotionVector3f()); // FIXME remote server ID
        notifyReplicaObjectRemoved(uuid);
    }

    LocationInfo& locinfo = it->second;
    locinfo.location = loc;
    locinfo.bounds = bnds;
    locinfo.local = true;

    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    // Add to the list of local objects
    CONTEXT_TRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, true, loc);
    notifyLocalObjectAdded(uuid, location(uuid), bounds(uuid));
}

void StandardLocationService::removeLocalObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state

  if (!(mLocations.find(uuid)!= mLocations.end()))
  {
    printf("\n\nDoes not meet first condition for object:  %s\n\n", uuid.toString().c_str());
    fflush(stdout);
  }
  if (! ( mLocations[uuid].local == true ))
  {
    printf("\n\nDoes not meet second condition for object:  %s\n\n", uuid.toString().c_str());
    fflush(stdout);
  }

  assert( mLocations.find(uuid) != mLocations.end() );
  assert( mLocations[uuid].local == true );
  mLocations.erase(uuid);




    // Remove from the list of local objects
  CONTEXT_TRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, false, TimedMotionVector3f());
    notifyLocalObjectRemoved(uuid);

    // FIXME we might want to give a grace period where we generate a replica if one isn't already there,
    // instead of immediately removing all traces of the object.
    // However, this needs to be handled carefully, prefer updates from another server, and expire
    // automatically.
}

void StandardLocationService::addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bnds) {
    // FIXME we should do checks on timestamps to decide which setting is "more" sane
    LocationMap::iterator it = mLocations.find(uuid);

    if (it != mLocations.end()) {
        // It already exists. If its local, ignore the update. If its another replica, somethings out of sync, but perform the update anyway
        LocationInfo& locinfo = it->second;
        if (!locinfo.local) {
            locinfo.location = loc;
            locinfo.bounds = bnds;
            //local = false
            // FIXME should we notify location and bounds updated info?
        }
        // else ignore
    }
    else {
        // Its a new replica, just insert it
        LocationInfo locinfo;
        locinfo.location = loc;
        locinfo.bounds = bnds;
        locinfo.local = false;
        mLocations[uuid] = locinfo;

        // We only run this notification when the object actually is new
        CONTEXT_TRACE(serverObjectEvent, 0, mContext->id(), uuid, true, loc); // FIXME add remote server ID
        notifyReplicaObjectAdded(uuid, location(uuid), bounds(uuid));
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
    CONTEXT_TRACE(serverObjectEvent, 0, mContext->id(), uuid, false, TimedMotionVector3f()); // FIXME add remote server ID
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
            assert(loc_it != mLocations.end());

            if (update.has_location()) {
                TimedMotionVector3f newloc(
                    update.location().t(),
                    MotionVector3f( update.location().position(), update.location().velocity() )
                );
                loc_it->second.location = newloc;
                notifyReplicaLocationUpdated( update.object(), newloc );

                CONTEXT_TRACE(serverLoc, msg->source_server(), mContext->id(), update.object(), newloc );
            }

            if (update.has_bounds()) {
                BoundingSphere3f newbounds = update.bounds();
                loc_it->second.bounds = newbounds;
                notifyReplicaBoundsUpdated( update.object(), newbounds );
            }
        }
    }

    delete msg;
}

void StandardLocationService::receiveMessage(const Sirikata::Protocol::Object::ObjectMessage& msg) {
    assert(msg.dest_object() == UUID::null());
    assert(msg.dest_port() == OBJECT_PORT_LOCATION);

    Sirikata::Protocol::Loc::Container loc_container;
    bool parse_success = loc_container.ParseFromString(msg.payload());
    assert(parse_success);

    if (loc_container.has_update_request()) {
        Sirikata::Protocol::Loc::LocationUpdateRequest request = loc_container.update_request();

        TrackingType obj_type = type(msg.source_object());
        if (obj_type == Local) {
            LocationMap::iterator loc_it = mLocations.find( msg.source_object() );
            assert(loc_it != mLocations.end());

            if (request.has_location()) {
                TimedMotionVector3f newloc(
                    request.location().t(),
                    MotionVector3f( request.location().position(), request.location().velocity() )
                );
                loc_it->second.location = newloc;
                notifyLocalLocationUpdated( msg.source_object(), newloc );

                CONTEXT_TRACE(serverLoc, mContext->id(), mContext->id(), msg.source_object(), newloc );
            }

            if (request.has_bounds()) {
                BoundingSphere3f newbounds = request.bounds();
                loc_it->second.bounds = newbounds;
                notifyLocalBoundsUpdated( msg.source_object(), newbounds );
            }
        }
        else {
            // Warn about update to non-local object
        }
    }
}

void StandardLocationService::locationUpdate(UUID source, void* buffer, uint length) {
    Sirikata::Protocol::Loc::Container loc_container;
    bool parse_success = loc_container.ParseFromString( String((char*) buffer, length) );
    assert(parse_success);

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
                notifyLocalLocationUpdated( source, newloc );

                CONTEXT_TRACE(serverLoc, mContext->id(), mContext->id(), source, newloc );

            }

            if (request.has_bounds()) {
                BoundingSphere3f newbounds = request.bounds();
                loc_it->second.bounds = newbounds;
                notifyLocalBoundsUpdated( source, newbounds );
            }
        }
        else {
            // Warn about update to non-local object
        }
    }
}


} // namespace Sirikata
