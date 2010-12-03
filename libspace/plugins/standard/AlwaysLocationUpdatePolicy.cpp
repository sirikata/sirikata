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

#include "Protocol_Frame.pbj.hpp"

namespace Sirikata {

void InitAlwaysLocationUpdatePolicyOptions() {
    Sirikata::InitializeClassOptions ico(ALWAYS_POLICY_OPTIONS, NULL,
        new OptionValue(LOC_MAX_PER_RESULT, "5", Sirikata::OptionValueType<uint32>(), "Maximum number of loc updates to report in each result message."),
        NULL);
}

AlwaysLocationUpdatePolicy::AlwaysLocationUpdatePolicy(const String& args)
 : LocationUpdatePolicy(),
   mServerSubscriptions(this),
   mObjectSubscriptions(this)
{
    OptionSet* optionsSet = OptionSet::getOptions(ALWAYS_POLICY_OPTIONS,NULL);
    optionsSet->parse(args);
}

AlwaysLocationUpdatePolicy::~AlwaysLocationUpdatePolicy() {
}

void AlwaysLocationUpdatePolicy::subscribe(ServerID remote, const UUID& uuid) {
    mServerSubscriptions.subscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote, const UUID& uuid) {
    mServerSubscriptions.unsubscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote) {
    mServerSubscriptions.unsubscribe(remote);
}

void AlwaysLocationUpdatePolicy::subscribe(const UUID& remote, const UUID& uuid) {
    mObjectSubscriptions.subscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(const UUID& remote, const UUID& uuid) {
    mObjectSubscriptions.unsubscribe(remote, uuid);
}

void AlwaysLocationUpdatePolicy::unsubscribe(const UUID& remote) {
    mObjectSubscriptions.unsubscribe(remote);
}


void AlwaysLocationUpdatePolicy::localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh) {
    // Ignore, initial additions will be handled by a prox update
}

void AlwaysLocationUpdatePolicy::localObjectRemoved(const UUID& uuid, bool agg) {
    // Ignore, removals will be handled by a prox update
}

void AlwaysLocationUpdatePolicy::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
    mServerSubscriptions.locationUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.locationUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {
    mServerSubscriptions.orientationUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.orientationUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::localBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval) {
    mServerSubscriptions.boundsUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.boundsUpdated(uuid, newval, mLocService);
}

void AlwaysLocationUpdatePolicy::localMeshUpdated(const UUID& uuid, bool agg, const String& newval) {
    mServerSubscriptions.meshUpdated(uuid, newval, mLocService);
    mObjectSubscriptions.meshUpdated(uuid, newval, mLocService);
}


void AlwaysLocationUpdatePolicy::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh) {
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

void AlwaysLocationUpdatePolicy::service() {
    mServerSubscriptions.service();
    mObjectSubscriptions.service();
}

void AlwaysLocationUpdatePolicy::tryCreateChildStream(SSTStreamPtr parent_stream, std::string* msg, int count) {
    parent_stream->createChildStream(
        std::tr1::bind(&AlwaysLocationUpdatePolicy::locSubstreamCallback, this, _1, _2, parent_stream, msg, count+1),
        (void*)msg->data(), msg->size(),
        OBJECT_PORT_LOCATION, OBJECT_PORT_LOCATION
    );
}

void AlwaysLocationUpdatePolicy::locSubstreamCallback(int x, SSTStreamPtr substream, SSTStreamPtr parent_stream, std::string* msg, int count) {
    // If we got it, the data got sent and we can drop the stream
    if (substream) {
        substream->close(false);
        return;
    }

    // If we didn't get it and we haven't retried too many times, try
    // again. Otherwise, report error and give up.
    if (count < 5)
        tryCreateChildStream(parent_stream, msg, count);
    else
        SILOG(always_loc,error,"Failed multiple times to open loc update substream.");
}

bool AlwaysLocationUpdatePolicy::trySend(const UUID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu)
{
  std::string bluMsg = serializePBJMessage(blu);
  SSTStreamPtr locServiceStream = mLocService->getObjectStream(dest);

  bool sent = false;
  if (locServiceStream) {
      Sirikata::Protocol::Frame msg_frame;
      msg_frame.set_payload(bluMsg);
      std::string* framed_loc_msg = new std::string(serializePBJMessage(msg_frame));
      tryCreateChildStream(locServiceStream, framed_loc_msg, 0);
      sent = true;
  }

  return sent;
}

bool AlwaysLocationUpdatePolicy::trySend(const ServerID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu) {
    Message* msg = new Message(
        mLocService->context()->id(),
        SERVER_PORT_LOCATION,
        dest,
        SERVER_PORT_LOCATION,
        serializePBJMessage(blu)
    );
    return mLocMessageRouter->route(msg);
}

} // namespace Sirikata
