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

OracleLocationService::OracleLocationService(ServerID sid, MessageRouter* router, MessageDispatcher* dispatcher, ObjectFactory* objfactory)
 : LocationService(sid, router, dispatcher),
   mCurrentTime(Time::null()),
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

void OracleLocationService::tick(const Time& t) {
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

    mCurrentTime = t;
    mUpdatePolicy->tick(t);
}

TimedMotionVector3f OracleLocationService::location(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.location;
}

Vector3f OracleLocationService::currentPosition(const UUID& uuid) {
    TimedMotionVector3f loc = location(uuid);
    return loc.extrapolate(mCurrentTime).position();
}

BoundingSphere3f OracleLocationService::bounds(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.bounds;
}

void OracleLocationService::addLocalObject(const UUID& uuid) {
    // This is an oracle, so we don't need to track these.
    if (mReplicaObjects.find(uuid) != mReplicaObjects.end()) {
        mReplicaObjects.erase(uuid);
        notifyReplicaObjectRemoved(uuid);
    }

    mLocalObjects.insert(uuid);
    notifyLocalObjectAdded(uuid, location(uuid), bounds(uuid));
}

void OracleLocationService::removeLocalObject(const UUID& uuid) {
    // This is an oracle, so we don't need to track these.
    if (mLocalObjects.find(uuid) != mLocalObjects.end()) {
        mLocalObjects.erase(uuid);
        notifyLocalObjectRemoved(uuid);
    }

    mReplicaObjects.insert(uuid);
    notifyReplicaObjectAdded(uuid, location(uuid), bounds(uuid));
}

void OracleLocationService::receiveMessage(Message* msg) {
    BulkLocationMessage* bulk_loc = dynamic_cast<BulkLocationMessage*>(msg);
    if (bulk_loc != NULL) {
        // We don't actually use these updates here because we are an Oracle --
        // we already know all the positions. Normally these would generate
        // replica events.
        delete msg;
    }
}


} // namespace CBR
