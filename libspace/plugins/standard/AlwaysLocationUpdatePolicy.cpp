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
   mServerSubscriptions(this, mServerUpdatesPerSecond),
   mOHSubscriptions(this, mOHUpdatesPerSecond),
   mObjectSubscriptions(this, mObjectUpdatesPerSecond)
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

void AlwaysLocationUpdatePolicy::subscribe(ServerID remote, const UUID& uuid, SeqNoPtr seqnoPtr)
{
    if (validSubscriber(remote))
        mServerSubscriptions.subscribe(remote, uuid, seqnoPtr);
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote, const UUID& uuid) {
    mServerSubscriptions.unsubscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote) {
    mServerSubscriptions.unsubscribe(remote);
}

void AlwaysLocationUpdatePolicy::subscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    if (validSubscriber(remote))
        mOHSubscriptions.subscribe(remote, uuid, mLocService->context()->ohSessionManager()->getSession(remote)->seqNoPtr());
}

void AlwaysLocationUpdatePolicy::unsubscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    mOHSubscriptions.unsubscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(const OHDP::NodeID& remote) {
    mOHSubscriptions.unsubscribe(remote);
}

void AlwaysLocationUpdatePolicy::subscribe(const UUID& remote, const UUID& uuid) {
    if (validSubscriber(remote))
        mObjectSubscriptions.subscribe(remote, uuid, mLocService->context()->objectSessionManager()->getSession(ObjectReference(remote))->getSeqNoPtr());
}

void AlwaysLocationUpdatePolicy::unsubscribe(const UUID& remote, const UUID& uuid) {
    mObjectSubscriptions.unsubscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(const UUID& remote) {
    mObjectSubscriptions.unsubscribe(remote);
}


void AlwaysLocationUpdatePolicy::localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) {
    // Ignore, initial additions will be handled by a prox update
}

void AlwaysLocationUpdatePolicy::localObjectRemoved(const UUID& uuid, bool agg) {
    // Ignore, removals will be handled by a prox update
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

void AlwaysLocationUpdatePolicy::localBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval) {
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


void AlwaysLocationUpdatePolicy::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) {
    // Ignore, initial additions will be handled by a prox update
}

void AlwaysLocationUpdatePolicy::replicaObjectRemoved(const UUID& uuid) {
    // Ignore, removals will be handled by a prox update
}

void AlwaysLocationUpdatePolicy::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    mObjectSubscriptions.locationUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
    mObjectSubscriptions.orientationUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    mObjectSubscriptions.boundsUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaMeshUpdated(const UUID& uuid, const String& newval) {
    mObjectSubscriptions.meshUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::replicaPhysicsUpdated(const UUID& uuid, const String& newval) {
    mObjectSubscriptions.physicsUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::service() {
    mServerSubscriptions.service();
    mOHSubscriptions.service();
    mObjectSubscriptions.service();
}

void AlwaysLocationUpdatePolicy::tryCreateChildStream(const UUID& dest, ODPSST::Stream::Ptr parent_stream, std::string* msg, int count) {
    if (!validSubscriber(dest)) {
        mObjectSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
        return;
    }

    parent_stream->createChildStream(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::objectLocSubstreamCallback, this, _1, _2, dest, parent_stream, msg, count+1),
        (void*)msg->data(), msg->size(),
        OBJECT_PORT_LOCATION, OBJECT_PORT_LOCATION
    );
}

void AlwaysLocationUpdatePolicy::objectLocSubstreamCallback(int x, ODPSST::Stream::Ptr substream, const UUID& dest, ODPSST::Stream::Ptr parent_stream, std::string* msg, int count) {
    // If we got it, the data got sent and we can drop the stream
    if (substream) {
        mObjectSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
        substream->close(false);
        return;
    }

    // If we didn't get it and we haven't retried too many times, try
    // again. Otherwise, report error and give up.
    if (count < 5) {
        tryCreateChildStream(dest, parent_stream, msg, count);
    }
    else {
        SILOG(always_loc,error,"Failed multiple times to open loc update substream.");
        mObjectSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
    }
}

void AlwaysLocationUpdatePolicy::tryCreateChildStream(const OHDP::NodeID& dest, OHDPSST::Stream::Ptr parent_stream, std::string* msg, int count) {
    if (!validSubscriber(dest)) {
        mOHSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
        return;
    }

    parent_stream->createChildStream(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::ohLocSubstreamCallback, this, _1, _2, dest, parent_stream, msg, count+1),
        (void*)msg->data(), msg->size(),
        OBJECT_PORT_LOCATION, OBJECT_PORT_LOCATION
    );
}

void AlwaysLocationUpdatePolicy::ohLocSubstreamCallback(int x, OHDPSST::Stream::Ptr substream, const OHDP::NodeID& dest, OHDPSST::Stream::Ptr parent_stream, std::string* msg, int count) {
    // If we got it, the data got sent and we can drop the stream
    if (substream) {
        mOHSubscriptions.decrementOutstandingMessageCount(dest);
        delete msg;
        substream->close(false);
        return;
    }

    // If we didn't get it and we haven't retried too many times, try
    // again. Otherwise, report error and give up.
    if (count < 5) {
        tryCreateChildStream(dest, parent_stream, msg, count);
    }
    else {
        mOHSubscriptions.decrementOutstandingMessageCount(dest);
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

bool AlwaysLocationUpdatePolicy::trySend(const UUID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu)
{
    std::string bluMsg = serializePBJMessage(blu);

    ObjectSession* session = mLocService->context()->objectSessionManager()->getSession(ObjectReference(dest));
    if (session == NULL) return false;
    ODPSST::Stream::Ptr locServiceStream = session->getStream();
    if (!locServiceStream) return false;

    Sirikata::Protocol::Frame msg_frame;
    msg_frame.set_payload(bluMsg);
    std::string* framed_loc_msg = new std::string(serializePBJMessage(msg_frame));
    tryCreateChildStream(dest, locServiceStream, framed_loc_msg, 0);
    return true;
}

bool AlwaysLocationUpdatePolicy::trySend(const OHDP::NodeID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu)
{
    std::string bluMsg = serializePBJMessage(blu);

    ObjectHostSessionPtr session = mLocService->context()->ohSessionManager()->getSession(dest);
    if (!session) return false;
    OHDPSST::Stream::Ptr locServiceStream = session->stream();
    if (!locServiceStream) return false;

    Sirikata::Protocol::Frame msg_frame;
    msg_frame.set_payload(bluMsg);
    std::string* framed_loc_msg = new std::string(serializePBJMessage(msg_frame));
    tryCreateChildStream(dest, locServiceStream, framed_loc_msg, 0);
    return true;
}

bool AlwaysLocationUpdatePolicy::trySend(const ServerID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu) {
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
    mServerSubscriptions.decrementOutstandingMessageCount(dest);

    return mLocMessageRouter->route(msg);
}

} // namespace Sirikata
