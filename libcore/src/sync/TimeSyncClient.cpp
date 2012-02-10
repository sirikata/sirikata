/*  Sirikata
 *  TimeSyncClient.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/sync/TimeSyncClient.hpp>

#include <Protocol_TimeSync.pbj.hpp>
#include <sirikata/core/network/Message.hpp> //serializePBJMessage
#include <sirikata/core/network/ObjectMessage.hpp> // OBJECT_PORT_TIMESYNC

namespace Sirikata {

TimeSyncClient::TimeSyncClient(Context* ctx, OHDP::Service* ohdp_service, const SpaceID& space, const Duration& polling_interval, UpdatedCallback cb)
 : PollingService(ctx->mainStrand, "TimeSync", polling_interval, ctx, "TimeSync"),
   mContext(ctx),
   mSeqno(0),
   mHasBeenInitialized(false),
   mOffset(Duration::zero()),
   mCB(cb)
{
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;

    mPort = ohdp_service->bindOHDPPort(space, OHDP::PortID(OBJECT_PORT_TIMESYNC));
    if (mPort == NULL) {
        SILOG(timesync,fatal,"Couldn't bind port " << OBJECT_PORT_TIMESYNC << " for TimeSyncClient, must already be in use...");
    }
    else {
        SILOG(timesync, detailed, "Listening for time sync messages...");
        // Set up listener for sync message responses
        mPort->receive(
            std::tr1::bind(&TimeSyncClient::handleSyncMessage, this, _1, _2, _3)
        );
    }
}

TimeSyncClient::~TimeSyncClient() {
    delete mPort;
}

void TimeSyncClient::addNode(const OHDP::NodeID& ss) {
    mServers.insert(ss);
}

void TimeSyncClient::removeNode(const OHDP::NodeID& ss) {
    mServers.erase(ss);
}

void TimeSyncClient::poll() {
    if (mServers.empty()) return;

    SILOG(timesync, detailed, "Sending time sync message");

    // We could choose any of the servers, just use the first one we have in our set
    OHDP::NodeID nid = *(mServers.begin());
    OHDP::Endpoint server_endpoint(mPort->endpoint().space(), nid, OBJECT_PORT_TIMESYNC);

    // Send the next update request
    Sirikata::Protocol::TimeSync sync_msg;
    uint8 seqno = mSeqno++;
    sync_msg.set_seqno(seqno);
    mRequestTimes[seqno] = mContext->simTime();

    std::string serialized = serializePBJMessage(sync_msg);
    mPort->send(server_endpoint, MemoryReference(serialized));
}

void TimeSyncClient::handleSyncMessage(const OHDP::Endpoint &src, const OHDP::Endpoint &dst, MemoryReference payload) {
    SILOG(timesync, detailed, "Received time sync reply");

    Sirikata::Protocol::TimeSync sync_msg;
    bool parse_success = sync_msg.ParseFromArray(payload.data(), payload.size());
    if (!parse_success) {
        LOG_INVALID_MESSAGE_BUFFER(sync, error, ((char*)payload.data()), payload.size());
        return;
    }

    Time local_start_t = mRequestTimes[sync_msg.seqno()];
    Time server_t = sync_msg.t();
    Time local_finish_t = mContext->simTime();

    // Sanity check the round trip time to avoid using outdated packets
    static Duration sMaxRTT = Duration::seconds(5); // this could definitely be lower
    Duration rtt = local_finish_t - local_start_t;
    if (local_finish_t < local_start_t ||
        rtt > sMaxRTT) {
        SILOG(timesync, detailed, "Ignoring time sync reply: out of date or non-sensical results");
        return;
    }

    // FIXME use averaging, falloff, etc instead of just replacing the value outright
    mOffset = server_t - (local_start_t + (rtt/2.f));
    SILOG(timesync, detailed, "RTT " << rtt << " updates offset to " << mOffset);
    mHasBeenInitialized = true;
    if (mCB)
        mCB();
}

} // namespace Sirikata
