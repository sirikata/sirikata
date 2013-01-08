// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/Platform.hpp>
#include "TwitterAggregateManager.hpp"
#include "Options.hpp"

#include <sirikata/core/command/Commander.hpp>

#define AGG_LOG(lvl, msg) SILOG(aggregate-manager, lvl, msg)

using namespace std::tr1::placeholders;

namespace Sirikata {

TwitterAggregateManager::TwitterAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username)
 :  mLoc(loc)
{
    mLoc->addListener(this, true);

    String local_path = GetOptionValue<String>(OPT_TWAGGMGR_LOCAL_PATH);
    String local_url_prefix = GetOptionValue<String>(OPT_TWAGGMGR_LOCAL_URL_PREFIX);
    uint16 n_gen_threads = GetOptionValue<uint16>(OPT_TWAGGMGR_GEN_THREADS);
    uint16 n_upload_threads = GetOptionValue<uint16>(OPT_TWAGGMGR_UPLOAD_THREADS);

    if (mLoc->context()->commander()) {
        mLoc->context()->commander()->registerCommand(
            "space.aggregates.stats",
            std::tr1::bind(&TwitterAggregateManager::commandStats, this, _1, _2, _3)
        );
    }
}

TwitterAggregateManager::~TwitterAggregateManager() {
}

void TwitterAggregateManager::addAggregate(const UUID& uuid) {
  AGG_LOG(detailed, "addAggregate called: uuid=" << uuid);
}

void TwitterAggregateManager::removeAggregate(const UUID& uuid) {
  AGG_LOG(detailed, "removeAggregate: " << uuid);
}

void TwitterAggregateManager::addChild(const UUID& uuid, const UUID& child_uuid) {
    AGG_LOG(detailed, "addChild:  "  << uuid << " CHILD " << child_uuid);
}

void TwitterAggregateManager::removeChild(const UUID& uuid, const UUID& child_uuid) {
    AGG_LOG(detailed, "removeChild:  "  << uuid << " CHILD " << child_uuid);
}

void TwitterAggregateManager::aggregateObserved(const UUID& objid, uint32 nobservers, uint32 nchildren) {
}

void TwitterAggregateManager::generateAggregateMesh(const UUID& uuid, const Duration& delayFor) {
}



void TwitterAggregateManager::localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc,
                                                const TimedMotionQuaternion& orient,
                                                const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
                                                const String& zernike)
{
}

void TwitterAggregateManager::localObjectRemoved(const UUID& uuid, bool agg) {
}

void TwitterAggregateManager::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
}

void TwitterAggregateManager::localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {
}

void TwitterAggregateManager::localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) {
}

void TwitterAggregateManager::localMeshUpdated(const UUID& uuid, bool agg, const String& newval) {
}

void TwitterAggregateManager::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc,
                                                const TimedMotionQuaternion& orient,
                                                const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
                                                const String& zernike)
{
}

void TwitterAggregateManager::replicaObjectRemoved(const UUID& uuid) {
}

void TwitterAggregateManager::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
}

void TwitterAggregateManager::replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
}

void TwitterAggregateManager::replicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval) {
}

void TwitterAggregateManager::replicaMeshUpdated(const UUID& uuid, const String& newval) {
}



void TwitterAggregateManager::commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    //result.put("stats.raw_updates", mRawAggregateUpdates.read());

    cmdr->result(cmdid, result);
}

}
