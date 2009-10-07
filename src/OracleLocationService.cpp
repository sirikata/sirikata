/*  cbr
 *  OracleLocationService.cpp
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

#include "OracleLocationService.hpp"
#include "ObjectFactory.hpp"

namespace CBR {

OracleLocationService::OracleLocationService(SpaceContext* ctx, ObjectFactory* objfactory)
 : LocationService(ctx),
   mLocalObjects(),
   mReplicaObjects(),
   mInitialNotification(false)
{
    assert(objfactory);

    for(ObjectFactory::iterator it = objfactory->begin(); it != objfactory->end(); it++) {
        UUID objid = *it;
        MotionPath* objpath = objfactory->motion(objid);

        LocationInfo objinfo;
        objinfo.path = objpath;
        objinfo.location = objpath->initial();
        const TimedMotionVector3f* next = objpath->nextUpdate( objinfo.location.time() );
        if (next == NULL) {
            objinfo.has_next = false;
        }
        else {
            objinfo.has_next = true;
            objinfo.next = *next;
        }
        objinfo.bounds = objfactory->bounds(objid);

        mLocations[objid] = objinfo;
    }
}

bool OracleLocationService::contains(const UUID& uuid) const {
    assert( mLocalObjects.find(uuid) != mLocalObjects.end() ||
        mReplicaObjects.find(uuid) != mReplicaObjects.end() );
    return true;
}

LocationService::TrackingType OracleLocationService::type(const UUID& uuid) const {
    if (mLocalObjects.find(uuid) != mLocalObjects.end())
        return Local;
    else {
        assert(mReplicaObjects.find(uuid) != mReplicaObjects.end());
        return Replica;
    }
}


void OracleLocationService::service() {
    Time t = mContext->time;

    // Add an non-local objects as replicas.  We do this here instead of in the constructor
    // since we need to notify listeners, who would not have been subscribed yet. This is how
    // the real system would do it since it wouldn't find out about replicas until it talked to
    // other servers.
    if (!mInitialNotification) {
        for(LocationMap::iterator it = mLocations.begin(); it != mLocations.end(); it++) {
            UUID obj_id = it->first;
            if (mLocalObjects.find(obj_id) == mLocalObjects.end()) {
                mReplicaObjects.insert(obj_id);
                notifyReplicaObjectAdded(obj_id, location(obj_id), bounds(obj_id));
            }
        }
        mInitialNotification = true;
    }

    // FIXME we could maintain a heap of event times instead of scanning through this list every time
    for(LocationMap::iterator it = mLocations.begin(); it != mLocations.end(); it++) {
        UUID objid = it->first;
        LocationInfo& locinfo = it->second;
        if(locinfo.has_next && locinfo.next.time() <= t) {
            locinfo.location = locinfo.next;
            if (mLocalObjects.find(objid) != mLocalObjects.end())
                notifyLocalLocationUpdated(objid, locinfo.location);
            else if (mReplicaObjects.find(objid) != mReplicaObjects.end())
                notifyReplicaLocationUpdated(objid, locinfo.location);
            const TimedMotionVector3f* next = locinfo.path->nextUpdate(t);
            if (next == NULL)
                locinfo.has_next = false;
            else
                locinfo.next = *next;
        }
    }
    // FIXME we should update bounds as well

    mUpdatePolicy->service();
}

TimedMotionVector3f OracleLocationService::location(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.location;
}

Vector3f OracleLocationService::currentPosition(const UUID& uuid) {
    TimedMotionVector3f loc = location(uuid);
    return loc.extrapolate(mContext->time).position();
}

BoundingSphere3f OracleLocationService::bounds(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.bounds;
}

void OracleLocationService::addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bnds) {
    // This is an oracle, so we don't need to track these.
    if (mReplicaObjects.find(uuid) != mReplicaObjects.end()) {
        mReplicaObjects.erase(uuid);
        notifyReplicaObjectRemoved(uuid);
    }

    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    mLocalObjects.insert(uuid);
    notifyLocalObjectAdded(uuid, location(uuid), bounds(uuid));
}

void OracleLocationService::removeLocalObject(const UUID& uuid) {
    // This is an oracle, so we don't need to track these.
    if (mLocalObjects.find(uuid) != mLocalObjects.end()) {
        mLocalObjects.erase(uuid);
        notifyLocalObjectRemoved(uuid);
    }

    // Removed in favor of listening to other servers for notifications about
    // replica creationg/destruction, but may be useful for debugging.
    mReplicaObjects.insert(uuid);
    notifyReplicaObjectAdded(uuid, location(uuid), bounds(uuid));
}

void OracleLocationService::addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    // Add to the list of replica objects
    //mReplicaObjects.insert(uuid);
    // notifyReplicaObjectAdded(uuid, location(uuid), bounds(uuid));
}

void OracleLocationService::removeReplicaObject(const Time& t, const UUID& uuid) {
    //if (mReplicaObjects.find(uuid) != mReplicaObjects.end()) {
    //   mReplicaObjects.erase(uuid);
    //  notifyReplicaObjectRemoved(uuid);
    //}
}

void OracleLocationService::receiveMessage(Message* msg) {
    delete msg;
}

void OracleLocationService::receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    // FIXME we should probably at least decode this, maybe verify it
}

} // namespace CBR
