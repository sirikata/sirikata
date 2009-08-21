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

StandardLocationService::StandardLocationService(ServerID sid, MessageRouter* router, MessageDispatcher* dispatcher)
 : LocationService(sid, router, dispatcher),
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
        mReplicaObjects.erase(uuid);
        notifyReplicaObjectRemoved(uuid);
    }

    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    // Add to the list of local objects
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
        mLocalObjects.erase(uuid);
        notifyLocalObjectRemoved(uuid);
    }

    // Add to list of replicated objects. FIXME should we actually do this?
    // shouldn't the indication of whether to do this come from another server?
    // what about the time in between
    //mReplicaObjects.insert(uuid);
    //notifyReplicaObjectAdded(uuid, cachedInfo.location, cachedInfo.bounds);
}

void StandardLocationService::receiveMessage(Message* msg) {
    BulkLocationMessage* bulk_loc = dynamic_cast<BulkLocationMessage*>(msg);
    if (bulk_loc != NULL) {
        // We don't actually use these updates here because we are an Oracle --
        // we already know all the positions. Normally these would generate
        // replica events.
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
