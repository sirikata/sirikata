/*  cbr
 *  MigrationMonitor.cpp
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

#include "MigrationMonitor.hpp"

namespace CBR {

MigrationMonitor::MigrationMonitor(LocationService* locservice, CoordinateSegmentation* cseg)
 : mLocService(locservice),
   mCSeg(cseg)
{
    mLocService->addListener(this);
    mCSeg->addListener(this);

    mBoundingRegions = mCSeg->serverRegion( mLocService->context()->id() );
}

MigrationMonitor::~MigrationMonitor() {
    mCSeg->removeListener(this);
    mLocService->removeListener(this);
}


std::set<UUID> MigrationMonitor::service() {
    std::set<UUID> considered;
    std::set<UUID> results;

    Time curt = mLocService->context()->time;
    for(ObjectInfoByNextEvent::iterator it = mObjectInfo.get<nextevent>().begin();
        it != mObjectInfo.get<nextevent>().end() && it->nextEvent < curt;
        it++) {

        considered.insert(it->objid);

        Vector3f obj_pos = mLocService->currentPosition(it->objid);

        // NOTE: its possible the object wanders out of the region covered by *all* servers,
        // which is not properly handled by Loc yet.  Therefore we have secondary check which
        // ensures the object has moved into *some other server's* region as well as out of ours.
        if (mCSeg->region().contains(obj_pos, 0.0f))
            results.insert(it->objid);

        // NOTE: Objects stay in the index until they are removed by an actual migration --
        // i.e. the Server may reject this MigrationMonitor's suggestion.  Updates to the
        // next event happen after this loop to avoid iterator invalidation.  The updates
        // also takes care of static objects, which have long periods until their next event,
        // but which are forced to be considered periodically
    }

    // Update events for all objects we considered
    ObjectInfoByID& by_id = mObjectInfo.get<objid>();
    for(std::set<UUID>::iterator it = considered.begin(); it != considered.end(); it++) {
        by_id.modify(
            by_id.find(*it),
            std::tr1::bind(&MigrationMonitor::changeNextEventTime, std::tr1::placeholders::_1, computeNextEventTime(*it, mLocService->location(*it)))
        );
    }

    return results;
}

bool MigrationMonitor::onThisServer(const Vector3f& pos) const {
    return inRegion(pos);
}

bool MigrationMonitor::inRegion(const Vector3f& pos) const {
    for(BoundingBoxList::const_iterator it = mBoundingRegions.begin(); it != mBoundingRegions.end(); it++) {
        BoundingBox3f bb = *it;
        if (bb.contains(pos, 0.0f)) return true;
    }

    return false;
}

Time MigrationMonitor::computeNextEventTime(const UUID& obj, const TimedMotionVector3f& newloc) {
    Time curt = mLocService->context()->time;

    // Short cut: if its static, only verify it is in the server's boundaries
    if (newloc.velocity().lengthSquared() == 0.f) {
        if (inRegion(newloc.position()))
            return curt + Duration::seconds(100); // Effectively infinite time
        else
            return curt; // For some reason its out of the region, force the check on the next round
    }

    // Figure out which block its currently in
    Vector3f curpos = newloc.position(curt);
    BoundingBox3f curbox;
    bool foundbox = false;
    for(BoundingBoxList::const_iterator it = mBoundingRegions.begin(); it != mBoundingRegions.end(); it++) {
        BoundingBox3f bb = *it;
        if (bb.contains(curpos, 0.0f)) {
            curbox = bb;
            foundbox = true;
            break;
        }
    }

    if (!foundbox)
        return curt; // Couldn't find the bounding box its in, must not be any, force check on next round

    // Otherwise, we can now compute when an edge will be hit

    Vector3f curdir = newloc.velocity();

    Vector3f to_min = curbox.min() - curpos;
    Vector3f to_max = curbox.max() - curpos;

    // Uggh, component-wise divide would be nice...
    Vector3f time_to_min( to_min.x / curdir.x, to_min.y / curdir.y, to_min.z / curdir.z );
    Vector3f time_to_max( to_max.x / curdir.x, to_max.y / curdir.y, to_max.z / curdir.z );

    // For each dim, one should be negative (backward), one positive (forward).  Use max to pick forward direction
    Vector3f time_to_hit = time_to_min.max(time_to_max);

    // Take care of zeroes in velocity -- if velocity is too small, set time to hit extremely large
    if (fabs(curdir.x) < 0.00001f) time_to_hit.x = 100000.f;
    if (fabs(curdir.y) < 0.00001f) time_to_hit.y = 100000.f;
    if (fabs(curdir.z) < 0.00001f) time_to_hit.z = 100000.f;

    // And choose the minimum of those times
    float32 time_to_first_hit = std::min( std::min( time_to_hit.x, time_to_hit.y ), time_to_hit.z );
    assert(time_to_first_hit < 100000.f); // If we got here, then velocity couldn't be small enough to cause this
    assert(time_to_first_hit >= 0.f); // And at least one of them *must* be positive, or something is wrong with bbox
    // And convert to seconds
    Duration to_first_hit = Duration::seconds(time_to_first_hit);
    return curt + to_first_hit;
}

// Helper for multi_index modify method
void MigrationMonitor::changeNextEventTime(ObjectInfo& objinfo, const Time& newt) {
    objinfo.nextEvent = newt;
}

/** LocationServiceListener Interface. */

void MigrationMonitor::localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    assert( mObjectInfo.get<objid>().find(uuid) == mObjectInfo.get<objid>().end());

    mObjectInfo.insert( ObjectInfo(uuid, computeNextEventTime(uuid, loc)) );
}

void MigrationMonitor::localObjectRemoved(const UUID& uuid) {
    mObjectInfo.get<objid>().erase(uuid);
}

void MigrationMonitor::localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    assert( mObjectInfo.get<objid>().find(uuid) != mObjectInfo.get<objid>().end());

    ObjectInfoByID& by_id = mObjectInfo.get<objid>();
    by_id.modify(
        by_id.find(uuid),
        std::tr1::bind(&MigrationMonitor::changeNextEventTime, std::tr1::placeholders::_1, computeNextEventTime(uuid, newval))
    );
}

void MigrationMonitor::localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    // We only care about location, not bounds
}

void MigrationMonitor::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    // We don't care about replicas
}
void MigrationMonitor::replicaObjectRemoved(const UUID& uuid) {
    // We don't care about replicas
}
void MigrationMonitor::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    // We don't care about replicas
}
void MigrationMonitor::replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    // We don't care about replicas
}

/** CoordinateSegmentation::Listener Interface. */
void MigrationMonitor::updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation) {
    for(std::vector<SegmentationInfo>::const_iterator it = new_segmentation.begin(); it != new_segmentation.end(); it++) {
        if (it->server == mLocService->context()->id()) {
            mBoundingRegions = it->region;

            // Recalculate *all* object potential update times
            ObjectInfoByID& by_id = mObjectInfo.get<objid>();
            for(ObjectInfoByID::iterator obj_it = by_id.begin(); obj_it != by_id.end(); obj_it++) {
                by_id.modify(
                    obj_it,
                    std::tr1::bind(&MigrationMonitor::changeNextEventTime,
                        std::tr1::placeholders::_1,
                        computeNextEventTime(obj_it->objid, mLocService->location(obj_it->objid))
                    )
                );
            }

            return;
        }
    }
}

} // namespace CBR
