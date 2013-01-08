// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SPACE_TWITTER_AGGREGATE_MANAGER_HPP_
#define _SIRIKATA_SPACE_TWITTER_AGGREGATE_MANAGER_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/space/AggregateManager.hpp>

#include <sirikata/core/command/Command.hpp>

namespace Sirikata {

class SIRIKATA_SPACE_EXPORT TwitterAggregateManager : public AggregateManager {
  public:

    /** Create an AggregateManager. */
    TwitterAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username);
    ~TwitterAggregateManager();

    virtual void addAggregate(const UUID& uuid);
    virtual void removeAggregate(const UUID& uuid);
    virtual void addChild(const UUID& uuid, const UUID& child_uuid) ;
    virtual void removeChild(const UUID& uuid, const UUID& child_uuid);
    virtual void aggregateObserved(const UUID& objid, uint32 nobservers, uint32 nchildren);
    virtual void generateAggregateMesh(const UUID& uuid, const Duration& delayFor = Duration::milliseconds(1.0f) );

  private:
    LocationService* mLoc;

    //Part of the LocationServiceListener interface.
    virtual void localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient,
        const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
        const String& zernike);
    virtual void localObjectRemoved(const UUID& uuid, bool agg) ;
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    virtual void localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval);
    virtual void localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) ;
    virtual void localMeshUpdated(const UUID& uuid, bool agg, const String& newval) ;
    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient,
        const AggregateBoundingInfo& bounds, const String& mesh, const String& physics, const String& zernike);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval);
    virtual void replicaMeshUpdated(const UUID& uuid, const String& newval);

    // Command handlers
    void commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);

};

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_TWITTER_AGGREGATE_MANAGER_HPP_
