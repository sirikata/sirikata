/*  Sirikata
 *  MigrationMonitor.hpp
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

#ifndef _SIRIKATA_MIGRATION_MONITOR_HPP_
#define _SIRIKATA_MIGRATION_MONITOR_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/LocationService.hpp>
#include <sirikata/space/CoordinateSegmentation.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace Sirikata {

/** MigrationMonitor keeps track of objects that are currently on
 *  this server and determines when they have left the server, or
 *  more generally, when they should begin to migrate to another
 *  server.
 */
class MigrationMonitor : public LocationServiceListener, public CoordinateSegmentation::Listener {
public:
    typedef std::tr1::function<void(const UUID&)> MigrationCallback;

    /** Create a new MigrationMonitor.  The MigrationCallback is called any time a migration is detected.  Note that
     *  it may be called from a thread other than the main thread, so it should be thread safe.
     *  \param ctx SpaceContext for this simulation
     *  \param locservice location service for this server
     *  \param cseg coordinate segmentation used for this server
     *  \param cb callback to be invoked when a migration is detected
     */
    MigrationMonitor(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, MigrationCallback cb);
    ~MigrationMonitor();

    // Indicates whether the given position is on this server, useful to check if object should be
    // permitted to join this server.
    bool onThisServer(const Vector3f& pos) const;

private:

    /** LocationServiceListener Interface. */
    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void localObjectRemoved(const UUID& uuid);
    virtual void localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void localOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void localMeshUpdated(const UUID& uuid, const String& newval);
    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void replicaMeshUpdated(const UUID& uuid, const String& newval);

    // Handlers for location events we care about.  These are handled in our internal strand
    void handleLocalObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    void handleLocalObjectRemoved(const UUID& uuid);
    void handleLocalLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);

    /** CoordinateSegmentation::Listener Interface. */
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation);
    void handleUpdatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation);

    // Given our current information, sets up the next timeout, cancelling any outstanding timeouts.
    // This is conservative -- it will only replace the timer if the earliest event time changed
    void waitForNextEvent();

    // Service the migration monitor, return a set of objects for which migrations should be started
    void service();

    bool inRegion(const Vector3f& pos) const;

    Time computeNextEventTime(const UUID& obj, const TimedMotionVector3f& newloc);

    SpaceContext* mContext;
    LocationService* mLocService;
    CoordinateSegmentation* mCSeg;
    BoundingBoxList mBoundingRegions;

    struct ObjectInfo {
        ObjectInfo(UUID id, Time next)
         : objid(id),
           nextEvent(next)
        {}

        UUID objid;
        Time nextEvent;
    };


    // Tags used by ObjectInfoSet
    struct objid {};
    struct nextevent {};

    typedef boost::multi_index_container<
        ObjectInfo,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique< boost::multi_index::tag<objid>, BOOST_MULTI_INDEX_MEMBER(ObjectInfo,UUID,objid) >,
            boost::multi_index::ordered_non_unique< boost::multi_index::tag<nextevent>, BOOST_MULTI_INDEX_MEMBER(ObjectInfo,Time,nextEvent) >
        >
    > ObjectInfoSet;
    typedef ObjectInfoSet::index<objid>::type ObjectInfoByID;
    typedef ObjectInfoSet::index<nextevent>::type ObjectInfoByNextEvent;

    static void changeNextEventTime(ObjectInfo& objinfo, const Time& newt);

    ObjectInfoSet mObjectInfo;

    Network::IOStrand* mStrand;
    Network::IOTimerPtr mTimer;

    Time mMinEventTime;

    MigrationCallback mCB;
};

} // namespace Sirikata

#endif //_SIRIKATA_MIGRATION_MONITOR_HPP_
