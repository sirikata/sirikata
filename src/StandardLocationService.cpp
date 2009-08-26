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
#include "ObjectFactory.hpp"

namespace CBR {

StandardLocationService::StandardLocationService(ServerID sid, MessageRouter* router, MessageDispatcher* dispatcher, Trace* trace)
 : LocationService(sid, router, dispatcher, trace),
   mCurrentTime(Time::null()),
   mLocalObjects(),
   mReplicaObjects()
{
}

bool StandardLocationService::contains(const UUID& uuid) const {
    return ( mLocalObjects.find(uuid) != mLocalObjects.end() ||
        mReplicaObjects.find(uuid) != mReplicaObjects.end() );
}

LocationService::TrackingType StandardLocationService::type(const UUID& uuid) const {
    if (mLocalObjects.find(uuid) != mLocalObjects.end())
        return Local;
    else if (mReplicaObjects.find(uuid) != mReplicaObjects.end())
        return Replica;
    else
        return NotTracking;
}


void StandardLocationService::tick(const Time& t) {
    mCurrentTime = t;
    mUpdatePolicy->tick(t);
}

TimedMotionVector3f StandardLocationService::location(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.location;
}

Vector3f StandardLocationService::currentPosition(const UUID& uuid) {
    TimedMotionVector3f loc = location(uuid);
    return loc.extrapolate(mCurrentTime).position();
}

BoundingSphere3f StandardLocationService::bounds(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.bounds;
}

void StandardLocationService::addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bnds) {
    // Add to the information to the cache
    LocationInfo info;
    info.location = loc;
    info.bounds = bnds;
    mLocations[uuid] = info;

    // Remove from replicated objects if its founds
    if (mReplicaObjects.find(uuid) != mReplicaObjects.end()) {
        mTrace->serverObjectEvent(mCurrentTime, 0, mID, uuid, false, TimedMotionVector3f()); // FIXME remote server ID
        mReplicaObjects.erase(uuid);
        notifyReplicaObjectRemoved(uuid);
    }

    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    // Add to the list of local objects
    mTrace->serverObjectEvent(mCurrentTime, mID, mID, uuid, true, loc);
    mLocalObjects.insert(uuid);
    notifyLocalObjectAdded(uuid, location(uuid), bounds(uuid));
}

void StandardLocationService::removeLocalObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    LocationInfo cachedInfo = mLocations[uuid];
    mLocations.erase(uuid);

    // Remove from the list of local objects
    if (mLocalObjects.find(uuid) != mLocalObjects.end()) {
        mTrace->serverObjectEvent(mCurrentTime, mID, mID, uuid, false, TimedMotionVector3f());
        mLocalObjects.erase(uuid);
        notifyLocalObjectRemoved(uuid);
    }

    // FIXME we might want to give a grace period where we generate a replica if one isn't already there,
    // instead of immediately removing all traces of the object.
    // However, this needs to be handled carefully, prefer updates from another server, and expire
    // automatically.
}

void StandardLocationService::addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bnds) {
    // FIXME we should do checks on timestamps to decide which setting is "more" sane

    bool is_local = (mLocalObjects.find(uuid) != mLocalObjects.end());

    // If its not local we need to insert the loc info
    if (!is_local) {
        LocationInfo locinfo;
        locinfo.location = loc;
        locinfo.bounds = bnds;
        mLocations[uuid] = locinfo;
    }
    else { // Otherwise we just maintain the existing state since we trust the local info more, although we might want to tcheck timestamps instead of just believing the local number
        assert( mLocations.find(uuid) != mLocations.end() );
    }

    // Regardless of who's state is authoritative, we need to add to the list of replicas a let people know
    mTrace->serverObjectEvent(mCurrentTime, 0, mID, uuid, true, loc); // FIXME add remote server ID
    mReplicaObjects.insert(uuid);
    notifyReplicaObjectAdded(uuid, location(uuid), bounds(uuid));
}

void StandardLocationService::removeReplicaObject(const Time& t, const UUID& uuid) {
    // FIXME we should maintain some time information and check t against it to make sure this is sane

    bool is_local = (mLocalObjects.find(uuid) != mLocalObjects.end());

    // If the object isn't already marked as local, its going away for good so delete its loc info
    if (!is_local)
        mLocations.erase(uuid);

    // And no matter what, we need to erase it from the replica object list and tell people about it
    mTrace->serverObjectEvent(mCurrentTime, 0, mID, uuid, false, TimedMotionVector3f()); // FIXME add remote server ID
    mReplicaObjects.erase(uuid);
    notifyReplicaObjectRemoved(uuid);
}


void StandardLocationService::receiveMessage(Message* msg) {
    BulkLocationMessage* bulk_loc = dynamic_cast<BulkLocationMessage*>(msg);
    if (bulk_loc != NULL) {
        for(uint32 idx = 0; idx < bulk_loc->contents.update_size(); idx++) {
            CBR::Protocol::Loc::LocationUpdate update = bulk_loc->contents.update(idx);

            // Its possible we'll get an out of date update. We only use this update
            // if (a) we have this object marked as a replica object and (b) we don't
            // have this object marked as a local object
            if (mLocalObjects.find(update.object()) != mLocalObjects.end())
                continue;
            if (mReplicaObjects.find(update.object()) == mReplicaObjects.end())
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

                mTrace->serverLoc( mCurrentTime, GetUniqueIDServerID(msg->id()), mID, update.object(), newloc );
            }

            if (update.has_bounds()) {
                BoundingSphere3f newbounds = update.bounds();
                loc_it->second.bounds = newbounds;
                notifyReplicaBoundsUpdated( update.object(), newbounds );
            }

        }

        delete msg;
    }
}

void StandardLocationService::receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    assert(msg.dest_object() == UUID::null());
    assert(msg.dest_port() == OBJECT_PORT_LOCATION);

    CBR::Protocol::Loc::Container loc_container;
    bool parse_success = loc_container.ParseFromString(msg.payload());
    assert(parse_success);

    if (loc_container.has_update_request()) {
        CBR::Protocol::Loc::LocationUpdateRequest request = loc_container.update_request();

        if (mLocalObjects.find( msg.source_object() ) == mLocalObjects.end()) {
            // FIXME warn about update to non-local object
        }
        else {
            LocationMap::iterator loc_it = mLocations.find( msg.source_object() );
            assert(loc_it != mLocations.end());

            if (request.has_location()) {
                TimedMotionVector3f newloc(
                    request.location().t(),
                    MotionVector3f( request.location().position(), request.location().velocity() )
                );
                loc_it->second.location = newloc;
                notifyLocalLocationUpdated( msg.source_object(), newloc );

                mTrace->serverLoc( mCurrentTime, mID, mID, msg.source_object(), newloc );
            }

            if (request.has_bounds()) {
                BoundingSphere3f newbounds = request.bounds();
                loc_it->second.bounds = newbounds;
                notifyLocalBoundsUpdated( msg.source_object(), newbounds );
            }
        }
    }
}


} // namespace CBR
