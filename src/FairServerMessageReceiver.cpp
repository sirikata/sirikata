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

namespace CBR {

FairServerMessageReceiver::FairServerMessageReceiver(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, uint32 recv_bytes_per_sec)
        : ServerMessageReceiver(ctx, net, sidmap),
          mRecvRate(recv_bytes_per_sec),
          mLastServiceTime(ctx->time),
          mReceiveQueues(),
          mRemainderReceiveBytes(0),
          mLastReceiveEndTime(ctx->time),
          mReceiveSet(),
          mReceiveQueue()
{
}

FairServerMessageReceiver::~FairServerMessageReceiver() {
}

bool FairServerMessageReceiver::receive(Message** msg_out) {
    if (mReceiveQueue.empty()) {
        *msg_out = NULL;
        return false;
    }

    *msg_out = mReceiveQueue.front();
    mReceiveQueue.pop();
    return true;
}

void FairServerMessageReceiver::service() {
    mProfiler->started();

    Duration since_last = mContext->time - mLastServiceTime;
    uint64 recv_bytes = since_last.toSeconds() * mRecvRate + mRemainderReceiveBytes;

    // Receive
    ServerID sid;
    Message* next_recv_msg = NULL;
    mReceiveQueues.service(); // FIXME this shouldn't be necessary if NetworkQueueWrapper could notify the FairQueue
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
        mReceiveQueue.push(next_recv_msg);
    }

    if (mReceiveQueues.empty()) {
        mRemainderReceiveBytes = 0;
        mLastReceiveEndTime = mContext->time;
    }
    else {
        mRemainderReceiveBytes = recv_bytes;
        //mLastReceiveEndTime = already recorded, last end receive time
    }

    mLastServiceTime = mContext->time;

    mProfiler->finished();
}

void FairServerMessageReceiver::setServerWeight(ServerID sid, float weight) {
    if (!mReceiveQueues.hasQueue(sid)) {
        mReceiveQueues.addQueue(new NetworkQueueWrapper(sid, mNetwork, mServerIDMap),sid,weight);
    }
    else
        mReceiveQueues.setQueueWeight(sid, weight);

    // add to the receive set
    mReceiveSet.insert(sid);
}


} // namespace CBR
