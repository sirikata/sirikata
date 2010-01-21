/*  cbr
 *  FairServerMessageReceiver.cpp
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

#include "FairServerMessageReceiver.hpp"
#include <sirikata/network/IOStrandImpl.hpp>
#include "ServerWeightCalculator.hpp"

namespace CBR {

FairServerMessageReceiver::FairServerMessageReceiver(SpaceContext* ctx, Network* net, ServerWeightCalculator* swc, Listener* listener, uint32 recv_bytes_per_sec)
        : ServerMessageReceiver(ctx, net, listener),
          mRecvRate(recv_bytes_per_sec),
          mServerWeightCalculator(swc),
          mServiceTimer(
              IOTimer::create(
                  ctx->ioService,
                  ctx->mainStrand->wrap( std::tr1::bind(&FairServerMessageReceiver::service, this) )
                              )
                        ),
          mLastServiceTime(ctx->simTime()),
          mReceiveQueues(),
          mRemainderReceiveBytes(0),
          mLastReceiveEndTime(ctx->time),
          mReceiveSet()
{
}

FairServerMessageReceiver::~FairServerMessageReceiver() {
}

void FairServerMessageReceiver::handleReceived(const ServerID& from) {
    // Given the new data we need to update our view of the world
    mReceiveQueues.notifyPushFront(from);

    // Cancel any outstanding work callbacks
    mServiceTimer->cancel();

    // And run service (which will handle setting up
    // any new service callbacks that may be needed).
    service();
}

void FairServerMessageReceiver::service() {
    mProfiler->started();

    Time tcur = mContext->simTime();
    Duration since_last = tcur - mLastServiceTime;
    uint64 recv_bytes = since_last.toSeconds() * mRecvRate + mRemainderReceiveBytes;

    // Receive
    ServerID sid;
    Message* next_recv_msg = NULL;
    while( recv_bytes > 0 && (next_recv_msg = mReceiveQueues.front(&recv_bytes,&sid)) != NULL ) {
        Message* next_recv_msg_popped = mReceiveQueues.pop(&recv_bytes);
        assert(next_recv_msg_popped == next_recv_msg);

        uint32 packet_size = next_recv_msg->serializedSize();
        Duration recv_duration = Duration::seconds((float)packet_size / (float)mRecvRate);
        Time start_time = mLastReceiveEndTime;
        Time end_time = mLastReceiveEndTime + recv_duration;
        mLastReceiveEndTime = end_time;

        /*
           FIXME at some point we should record this here instead of in Server.cpp
        mContext->trace()->serverDatagramReceived();
        */
        mListener->serverMessageReceived(next_recv_msg);
    }

    if (mReceiveQueues.empty()) {
        mRemainderReceiveBytes = 0;
        mLastReceiveEndTime = tcur;
    }
    else {
        mRemainderReceiveBytes = recv_bytes;
        //mLastReceiveEndTime = already recorded, last end receive time

        // Since the queues are not empty that means we must have stopped due to
        // insufficient budget of bytes.  Setup a timer to let us check again
        // soon.
        // FIXME we should calculate an exact duration instead of making it up
        mServiceTimer->wait( Duration::microseconds(100) );
    }

    mLastServiceTime = tcur;

    mProfiler->finished();
}

void FairServerMessageReceiver::setServerWeight(ServerID sid, float weight) {
    if (!mReceiveQueues.hasQueue(sid)) {
        mReceiveQueues.addQueue(
            new NetworkQueueWrapper(mContext, sid, mNetwork, Trace::SPACE_TO_SPACE_READ_FROM_NET),
            sid,
            weight);
    }
    else
        mReceiveQueues.setQueueWeight(sid, weight);

    // add to the receive set
    mReceiveSet.insert(sid);
}

void FairServerMessageReceiver::networkReceivedConnection(const ServerID& from) {
    mContext->mainStrand->post( std::tr1::bind(&FairServerMessageReceiver::handleReceivedConnection, this, from) );
}

void FairServerMessageReceiver::handleReceivedConnection(const ServerID& from) {
    SILOG(fairreceiver,info,"Received connection from " << from << ", setting weight");
    setServerWeight(
        from,
        mServerWeightCalculator->weight(mContext->id(), from)
                    );
}

void FairServerMessageReceiver::networkReceivedData(const ServerID& from) {
    SILOG(fairreceiver,insane,"Received network data from " << from);

    // No matter what, we'll post an event.  Since a new front() is available
    // this could completely change the amount of time we're waiting so the main
    // strand always needs to know.
    mContext->mainStrand->post( std::tr1::bind(&FairServerMessageReceiver::handleReceived, this, from) );
}

} // namespace CBR
