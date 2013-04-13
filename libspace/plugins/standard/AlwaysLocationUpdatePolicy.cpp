/*  Sirikata
 *  AlwaysLocationUpdatePolicy.cpp
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

#include "AlwaysLocationUpdatePolicy.hpp"
#include <sirikata/space/ServerMessage.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/space/ObjectHostSession.hpp>

#include <sirikata/core/odp/SST.hpp>
#include <sirikata/core/ohdp/SST.hpp>
#include "Protocol_Frame.pbj.hpp"

namespace Sirikata {

void InitAlwaysLocationUpdatePolicyOptions() {
    Sirikata::InitializeClassOptions ico(ALWAYS_POLICY_OPTIONS, NULL,
        new OptionValue(LOC_MAX_PER_RESULT, "5", Sirikata::OptionValueType<uint32>(), "Maximum number of loc updates to report in each result message."),
        NULL);
}

AlwaysLocationUpdatePolicy::AlwaysLocationUpdatePolicy(SpaceContext* ctx, const String& args)
 : LocationUpdatePolicy(),
   mStatsPoller(
       ctx->mainStrand,
       std::tr1::bind(&AlwaysLocationUpdatePolicy::reportStats, this),
       "AlwaysLocationUpdatePolicy Stats Poll",
       Duration::seconds((int64)1)
   ),
   mLastStatsTime(ctx->simTime()),
   mTimeSeriesServerUpdatesName(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".loc.server_updates_per_second"),
   mServerUpdatesPerSecond(0),
   mTimeSeriesOHUpdatesName(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".loc.oh_updates_per_second"),
   mOHUpdatesPerSecond(0),
   mTimeSeriesObjectUpdatesName(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".loc.object_updates_per_second"),
   mObjectUpdatesPerSecond(0),
   mServerSubscriptions(this, /*include_all_data=*/true, mServerUpdatesPerSecond),
   mOHSubscriptions(this, /*include_all_data=*/true, mOHUpdatesPerSecond),
   mObjectSubscriptions(this, /*include_all_data=*/false, mObjectUpdatesPerSecond)
{
    OptionSet* optionsSet = OptionSet::getOptions(ALWAYS_POLICY_OPTIONS,NULL);
    optionsSet->parse(args);
}

AlwaysLocationUpdatePolicy::~AlwaysLocationUpdatePolicy() {
}

void AlwaysLocationUpdatePolicy::start() {
    mStatsPoller.start();
}

void AlwaysLocationUpdatePolicy::stop() {
    mStatsPoller.stop();
}

void AlwaysLocationUpdatePolicy::reportStats() {
    Time tnow = mLocService->context()->recentSimTime();
    float32 since_last_seconds = (tnow - mLastStatsTime).seconds();
    mLastStatsTime = tnow;

    mLocService->context()->timeSeries->report(
        mTimeSeriesServerUpdatesName,
        mServerUpdatesPerSecond.read() / since_last_seconds
    );
    mServerUpdatesPerSecond = 0;

    mLocService->context()->timeSeries->report(
        mTimeSeriesOHUpdatesName,
        mOHUpdatesPerSecond.read() / since_last_seconds
    );
    mOHUpdatesPerSecond = 0;

    mLocService->context()->timeSeries->report(
        mTimeSeriesObjectUpdatesName,
        mObjectUpdatesPerSecond.read() / since_last_seconds
    );
    mObjectUpdatesPerSecond = 0;
}


// Server subscriptions

void AlwaysLocationUpdatePolicy::subscribe(ServerID remote, const UUID& uuid, SeqNoPtr seqnoPtr)
{
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleServerSubscribe, this, remote, uuid, seqnoPtr),
        "AlwaysLocationUpdatePolicy::handleServerSubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleServerSubscribe(ServerID remote, const UUID& uuid, SeqNoPtr seqno) {
    if (validSubscriber(remote) && mLocService->contains(uuid))
        mServerSubscriptions.subscribe(remote, uuid, seqno);
}


void AlwaysLocationUpdatePolicy::subscribe(ServerID remote, const UUID& uuid, ProxIndexID index_id, SeqNoPtr seqnoPtr)
{
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleServerIndexSubscribe, this, remote, uuid, index_id, seqnoPtr),
        "AlwaysLocationUpdatePolicy::handleServerIndexSubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleServerIndexSubscribe(ServerID remote, const UUID& uuid, ProxIndexID index_id, SeqNoPtr seqno) {
    if (validSubscriber(remote) && mLocService->contains(uuid))
        mServerSubscriptions.subscribe(remote, uuid, index_id, seqno);
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote, const UUID& uuid) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleServerUnsubscribe, this, remote, uuid),
        "AlwaysLocationUpdatePolicy::handleServerUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleServerUnsubscribe(ServerID remote, const UUID& uuid) {
    mServerSubscriptions.unsubscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote, const UUID& uuid, ProxIndexID index_id) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleServerIndexUnsubscribe, this, remote, uuid, index_id),
        "AlwaysLocationUpdatePolicy::handleServerIndexUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleServerIndexUnsubscribe(ServerID remote, const UUID& uuid, ProxIndexID index_id) {
    mServerSubscriptions.unsubscribe(remote, uuid, index_id);
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote, const std::tr1::function<void()>& cb) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleServerCompleteUnsubscribe, this, remote, cb),
        "AlwaysLocationUpdatePolicy::handleServerCompleteUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleServerCompleteUnsubscribe(ServerID remote, const std::tr1::function<void()>& cb) {
    mServerSubscriptions.unsubscribe(remote);
    if (cb) cb();
}



// OH subscriptions

void AlwaysLocationUpdatePolicy::subscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectHostSubscribe, this, remote, uuid),
        "AlwaysLocationUpdatePolicy::handleObjectHostSubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectHostSubscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    if (validSubscriber(remote) && mLocService->contains(uuid))
        mOHSubscriptions.subscribe(remote, uuid, mLocService->context()->ohSessionManager()->getSession(remote)->seqNoPtr());
}

void AlwaysLocationUpdatePolicy::subscribe(const OHDP::NodeID& remote, const UUID& uuid, ProxIndexID index_id) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectHostIndexSubscribe, this, remote, uuid, index_id),
        "AlwaysLocationUpdatePolicy::handleObjectHostIndexSubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectHostIndexSubscribe(const OHDP::NodeID& remote, const UUID& uuid, ProxIndexID index_id) {
    if (validSubscriber(remote) && mLocService->contains(uuid))
        mOHSubscriptions.subscribe(remote, uuid, index_id, mLocService->context()->ohSessionManager()->getSession(remote)->seqNoPtr());
}

void AlwaysLocationUpdatePolicy::unsubscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectHostUnsubscribe, this, remote, uuid),
        "AlwaysLocationUpdatePolicy::handleObjectHostUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectHostUnsubscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    mOHSubscriptions.unsubscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(const OHDP::NodeID& remote, const UUID& uuid, ProxIndexID index_id) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectHostIndexUnsubscribe, this, remote, uuid, index_id),
        "AlwaysLocationUpdatePolicy::handleObjectHostIndexUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectHostIndexUnsubscribe(const OHDP::NodeID& remote, const UUID& uuid, ProxIndexID index_id) {
    mOHSubscriptions.unsubscribe(remote, uuid, index_id);
}

void AlwaysLocationUpdatePolicy::unsubscribe(const OHDP::NodeID& remote, const std::tr1::function<void()>& cb)
{
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectHostCompleteUnsubscribe, this, remote, cb),
        "AlwaysLocationUpdatePolicy::handleObjectHostCompleteUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectHostCompleteUnsubscribe(const OHDP::NodeID& remote, const std::tr1::function<void()>& cb) {
    mOHSubscriptions.unsubscribe(remote);
    if (cb) cb();
}




// Object subscriptions

void AlwaysLocationUpdatePolicy::subscribe(const UUID& remote, const UUID& uuid) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectSubscribe, this, remote, uuid),
        "AlwaysLocationUpdatePolicy::handleObjectSubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectSubscribe(const UUID& remote, const UUID& uuid) {
    if (validSubscriber(remote) && mLocService->contains(uuid))
        mObjectSubscriptions.subscribe(remote, uuid, mLocService->context()->objectSessionManager()->getSession(ObjectReference(remote))->getSeqNoPtr());
}

void AlwaysLocationUpdatePolicy::subscribe(const UUID& remote, const UUID& uuid, ProxIndexID index_id) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectIndexSubscribe, this, remote, uuid, index_id),
        "AlwaysLocationUpdatePolicy::handleObjectIndexSubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectIndexSubscribe(const UUID& remote, const UUID& uuid, ProxIndexID index_id) {
    if (validSubscriber(remote) && mLocService->contains(uuid))
        mObjectSubscriptions.subscribe(remote, uuid, index_id, mLocService->context()->objectSessionManager()->getSession(ObjectReference(remote))->getSeqNoPtr());
}

void AlwaysLocationUpdatePolicy::unsubscribe(const UUID& remote, const UUID& uuid) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectUnsubscribe, this, remote, uuid),
        "AlwaysLocationUpdatePolicy::handleObjectUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectUnsubscribe(const UUID& remote, const UUID& uuid) {
    mObjectSubscriptions.unsubscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(const UUID& remote, const UUID& uuid, ProxIndexID index_id) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectIndexUnsubscribe, this, remote, uuid, index_id),
        "AlwaysLocationUpdatePolicy::handleObjectIndexUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectIndexUnsubscribe(const UUID& remote, const UUID& uuid, ProxIndexID index_id) {
    mObjectSubscriptions.unsubscribe(remote, uuid, index_id);
}

void AlwaysLocationUpdatePolicy::unsubscribe(const UUID& remote, const std::tr1::function<void()>& cb) {
    mLocService->context()->mainStrand->post(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::handleObjectCompleteUnsubscribe, this, remote, cb),
        "AlwaysLocationUpdatePolicy::handleObjectCompleteUnsubscribe"
    );
}
void AlwaysLocationUpdatePolicy::handleObjectCompleteUnsubscribe(const UUID& remote, const std::tr1::function<void()>& cb) {
    mObjectSubscriptions.unsubscribe(remote);
    if (cb) cb();
}




void AlwaysLocationUpdatePolicy::localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& physics) {
    // Ignore, initial additions will be handled by a prox update
}

LocationServiceListener::RemovalStatus AlwaysLocationUpdatePolicy::localObjectRemoved(const UUID& uuid, bool agg, const LocationServiceListener::RemovalCallback&callback) {
    callback();
    return LocationServiceListener::IMMEDIATE;
}

void AlwaysLocationUpdatePolicy::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
    mServerSubscriptions.locationUpdated(uuid, newval, mLocService);
    mOHSubscriptions.locationUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.locationUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {
    mServerSubscriptions.orientationUpdated(uuid, newval, mLocService);
    mOHSubscriptions.orientationUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.orientationUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) {
    mServerSubscriptions.boundsUpdated(uuid, newval, mLocService);
    mOHSubscriptions.boundsUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.boundsUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::localMeshUpdated(const UUID& uuid, bool agg, const String& newval) {
    mServerSubscriptions.meshUpdated(uuid, newval, mLocService);
    mOHSubscriptions.meshUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.meshUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::localPhysicsUpdated(const UUID& uuid, bool agg, const String& newval) {
    mServerSubscriptions.physicsUpdated(uuid, newval, mLocService);
    mOHSubscriptions.physicsUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.physicsUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::localQueryDataUpdated(const UUID& uuid, bool agg, const String& newval) {
    mServerSubscriptions.queryDataUpdated(uuid, newval, mLocService);
    mOHSubscriptions.queryDataUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.queryDataUpdated(uuid, newval, mLocService);
}


void AlwaysLocationUpdatePolicy::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& physics) {
    // Ignore, initial additions will be handled by a prox update
}

LocationServiceListener::RemovalStatus AlwaysLocationUpdatePolicy::replicaObjectRemoved(const UUID& uuid) {
    // Ignore, removals will be handled by a prox update
    return LocationServiceListener::IMMEDIATE;
}

void AlwaysLocationUpdatePolicy::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    mObjectSubscriptions.locationUpdated(uuid, newval, mLocService);
    mOHSubscriptions.locationUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
    mObjectSubscriptions.orientationUpdated(uuid, newval, mLocService);
    mOHSubscriptions.orientationUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval) {
    mObjectSubscriptions.boundsUpdated(uuid, newval, mLocService);
    mOHSubscriptions.boundsUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaMeshUpdated(const UUID& uuid, const String& newval) {
    mObjectSubscriptions.meshUpdated(uuid, newval, mLocService);
    mOHSubscriptions.meshUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaPhysicsUpdated(const UUID& uuid, const String& newval) {
    mObjectSubscriptions.physicsUpdated(uuid, newval, mLocService);
    mOHSubscriptions.physicsUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaQueryDataUpdated(const UUID& uuid, const String& newval) {
    mObjectSubscriptions.queryDataUpdated(uuid, newval, mLocService);
    mOHSubscriptions.queryDataUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::service() {
    mServerSubscriptions.service();
    mOHSubscriptions.service();
    mObjectSubscriptions.service();
}

void AlwaysLocationUpdatePolicy::tryCreateChildStream(const UUID& dest, ODPSST::StreamPtr parent_stream, std::string* msg, int count, const SubscriberInfoPtr&numOutstandingMessageCount) {
    if (!validSubscriber(dest)) {
        //mObjectSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
        return;
    }

    parent_stream->createChildStream(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::objectLocSubstreamCallback, this, _1, _2, dest, parent_stream, msg, count+1, numOutstandingMessageCount),
        (void*)msg->data(), msg->size(),
        OBJECT_PORT_LOCATION, OBJECT_PORT_LOCATION
    );
}

void AlwaysLocationUpdatePolicy::objectLocSubstreamCallback(int x, ODPSST::StreamPtr substream, const UUID& dest, ODPSST::StreamPtr parent_stream, std::string* msg, int count, const SubscriberInfoPtr &numOutstandingMessageCount) {
    // If we got it, the data got sent and we can drop the stream
    if (substream) {
        //mObjectSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
        substream->close(false);
        return;
    }

    // If we didn't get it and we haven't retried too many times, try
    // again. Otherwise, report error and give up.
    if (count < 5) {
        tryCreateChildStream(dest, parent_stream, msg, count, numOutstandingMessageCount);
    }
    else {
        SILOG(always_loc,error,"Failed multiple times to open loc update substream.");
        //mObjectSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
    }
}

void AlwaysLocationUpdatePolicy::tryCreateChildStream(const OHDP::NodeID& dest, OHDPSST::StreamPtr parent_stream, std::string* msg, int count, const SubscriberInfoPtr&numOutstandingMessageCount) {
    if (!validSubscriber(dest)) {
        //mOHSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
        return;
    }

    parent_stream->createChildStream(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::ohLocSubstreamCallback, this, _1, _2, dest, parent_stream, msg, count+1, numOutstandingMessageCount),
        (void*)msg->data(), msg->size(),
        OBJECT_PORT_LOCATION, OBJECT_PORT_LOCATION
    );
}

void AlwaysLocationUpdatePolicy::ohLocSubstreamCallback(int x, OHDPSST::StreamPtr substream, const OHDP::NodeID& dest, OHDPSST::StreamPtr parent_stream, std::string* msg, int count, const SubscriberInfoPtr&numOutstandingMessageCount) {
    // If we got it, the data got sent and we can drop the stream
    if (substream) {
        //mOHSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
        substream->close(false);
        return;
    }

    // If we didn't get it and we haven't retried too many times, try
    // again. Otherwise, report error and give up.
    if (count < 5) {
        tryCreateChildStream(dest, parent_stream, msg, count, numOutstandingMessageCount);
    }
    else {
        //mOHSubscriptions.decrementOutstandingMessageCount(dest);
        SILOG(always_loc,error,"Failed multiple times to open loc update substream.");
        delete msg;
    }
}

bool AlwaysLocationUpdatePolicy::validSubscriber(const UUID& dest) {
    return (mLocService->context()->objectSessionManager()->getSession(ObjectReference(dest)) != NULL);
}

bool AlwaysLocationUpdatePolicy::validSubscriber(const OHDP::NodeID& dest) {
    return (mLocService->context()->ohSessionManager()->getSession(dest));
}

bool AlwaysLocationUpdatePolicy::validSubscriber(const ServerID& dest) {
    // FIXME we might be able to do something based on active servers from the
    // ServerIDMap, but right now we just assume other servers are always valid
    // subscribers since we should always be able to connect to them and send
    // updates.
    return true;
}


bool AlwaysLocationUpdatePolicy::isSelfSubscriber(const UUID& sid, const UUID& observed) {
    return sid == observed;
}

bool AlwaysLocationUpdatePolicy::isSelfSubscriber(const OHDP::NodeID& sid, const UUID& observed) {
    // TODO(ewencp) we could do better here by only returning true if the
    // observed object is on the given OH.
    return true;
}

bool AlwaysLocationUpdatePolicy::isSelfSubscriber(const ServerID& sid, const UUID& observed) {
    // Servers never need self info since they don't request changes
    return false;
}

bool AlwaysLocationUpdatePolicy::trySend(const UUID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu, const SubscriberInfoPtr& numOutstandingMessageCount)
{
    std::string bluMsg = serializePBJMessage(blu);

    ObjectSession* session = mLocService->context()->objectSessionManager()->getSession(ObjectReference(dest));
    if (session == NULL) {
        //mObjectSubscriptions.decrementOutstandingMessageCount(dest);
        return false;
    }
    ODPSST::StreamPtr locServiceStream = session->getStream();
    if (!locServiceStream) {
        //mObjectSubscriptions.decrementOutstandingMessageCount(dest);
        return false;
    }

    Sirikata::Protocol::Frame msg_frame;
    msg_frame.set_payload(bluMsg);
    std::string* framed_loc_msg = new std::string(serializePBJMessage(msg_frame));
    tryCreateChildStream(dest, locServiceStream, framed_loc_msg, 0, numOutstandingMessageCount);
    return true;
}

bool AlwaysLocationUpdatePolicy::trySend(const OHDP::NodeID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu, const SubscriberInfoPtr& numOutstandingMessageCount)
{
    std::string bluMsg = serializePBJMessage(blu);

    ObjectHostSessionPtr session = mLocService->context()->ohSessionManager()->getSession(dest);
    if (!session) {
        //mOHSubscriptions.decrementOutstandingMessageCount(dest);
        return false;
    }
    OHDPSST::StreamPtr locServiceStream = session->stream();
    if (!locServiceStream) {
        //mOHSubscriptions.decrementOutstandingMessageCount(dest);
        return false;
    }

    Sirikata::Protocol::Frame msg_frame;
    msg_frame.set_payload(bluMsg);
    std::string* framed_loc_msg = new std::string(serializePBJMessage(msg_frame));
    tryCreateChildStream(dest, locServiceStream, framed_loc_msg, 0, numOutstandingMessageCount);
    return true;
}

bool AlwaysLocationUpdatePolicy::trySend(const ServerID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu, const SubscriberInfoPtr& numOutstandingMessageCount) {
    Message* msg = new Message(
        mLocService->context()->id(),
        SERVER_PORT_LOCATION,
        dest,
        SERVER_PORT_LOCATION,
        serializePBJMessage(blu)
    );

    // There's no retries/async step for servers since they either get on the
    // queues or they don't and everything after that is reliable. Therefore, we
    // immediately adjust the number of oustanding messages back.
    //mServerSubscriptions.decrementOutstandingMessageCount(dest);

    return mLocMessageRouter->route(msg);
}

} // namespace Sirikata
