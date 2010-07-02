/*  Sirikata
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

#include "FairServerMessageReceiver.hpp"
#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata {

FairServerMessageReceiver::FairServerMessageReceiver(SpaceContext* ctx, SpaceNetwork* net, Listener* listener)
        : ServerMessageReceiver(ctx, net, listener),
          mServiceTimer(
              Network::IOTimer::create(
                  ctx->ioService,
                  mReceiverStrand->wrap( std::tr1::bind(&FairServerMessageReceiver::service, this) )
                              )
                        ),
          mReceiveQueues(),
          mReceiveSet(),
          mServiceScheduled(false),
          mStoppedUnderflow(0),
          mStoppedMaxMessages(0),
          mBytesUsed(0)
{
}

FairServerMessageReceiver::~FairServerMessageReceiver() {
    SILOG(fsmr,info,
        "FSMR: Bytes used: " << mBytesUsed
    );
    SILOG(fsmr,info,
        "FSMR: Underflow: " << mStoppedUnderflow <<
        " max messages: " << mStoppedMaxMessages
    );
}

void FairServerMessageReceiver::scheduleServicing() {
    if (!mServiceScheduled.read()) {
        // If we can safely do it, just manage the servicing ourselves.  If
        // we have leftovers, things will get properly rescheduled on the
        // FSMR strand.
        //if (!service()) {
            // We couldn't service, just schedule it
            mServiceTimer->cancel();
            mServiceScheduled = true;
            mReceiverStrand->post(
                std::tr1::bind(&FairServerMessageReceiver::service, this)
            );
            //}
    }
}

void FairServerMessageReceiver::service() {
#define MAX_MESSAGES_PER_ROUND 100

    boost::mutex::scoped_try_lock lock(mServiceMutex);
    if (!lock.owns_lock()) return;

    mProfiler->started();

    mServiceScheduled = false;

    Time tcur = mContext->simTime();

    // Receive
    ServerID sid;
    Message* next_recv_msg = NULL;
    uint32 num_recv = 0;
    uint32 cum_recv_size = 0;
    uint32 went_empty = false;
    while( num_recv < MAX_MESSAGES_PER_ROUND ) {
        {
            boost::lock_guard<boost::mutex> lck(mMutex);

            next_recv_msg = mReceiveQueues.pop(&sid);
        }

        if (next_recv_msg == NULL) {
            went_empty = true;
            break;
        }

        cum_recv_size += next_recv_msg->size();

        CONTEXT_SPACETRACE(serverDatagramReceived, mContext->simTime(), next_recv_msg->source_server(), next_recv_msg->id(), next_recv_msg->serializedSize());
        mListener->serverMessageReceived(next_recv_msg);

        num_recv++;
    }

    mBytesUsed += cum_recv_size;
    mCapacityEstimator.estimate_rate(tcur, cum_recv_size);

    if (!went_empty) {
        // Since the queues are not empty that means we must have stopped due to
        // insufficient budget of bytes.  Setup a timer to let us check again
        // soon.
        // FIXME we should calculate an exact duration instead of making it up
        mBlocked=true;
        mStoppedMaxMessages++;
        mServiceTimer->wait( Duration::microseconds(1) );
    }else {
        mStoppedUnderflow++;
        mBlocked=false;
    }
    mProfiler->finished();

    return;
}

void FairServerMessageReceiver::handleUpdateSenderStats(ServerID sid, double total_weight, double used_weight) {
    boost::lock_guard<boost::mutex> lck(mMutex);

    assert(mReceiveQueues.hasQueue(sid));
    mReceiveQueues.setQueueWeight(sid, used_weight);
}

void FairServerMessageReceiver::networkReceivedConnection(SpaceNetwork::ReceiveStream* strm) {
    ServerID from = strm->id();
    {
        boost::lock_guard<boost::mutex> lck(mMutex);

        double wt = mReceiveQueues.avg_weight();
        SILOG(fairreceiver,info,"Received connection from " << from << ", setting weight to " << wt);

        if (mReceiveQueues.hasQueue(from)) {
            SILOG(FSMR,fatal,"Duplicate network connection received for server " << from << " and propagated up to receive queue.");
            assert(false);
        }

        mReceiveQueues.addQueue(
            new NetworkQueueWrapper(mContext, strm, Trace::SPACE_TO_SPACE_READ_FROM_NET),
            from,
            wt
            );

        // add to the receive set
        mReceiveSet.insert(from);
    }
    // Notify upstream
    mListener->serverConnectionReceived(from);
}

void FairServerMessageReceiver::networkReceivedData(SpaceNetwork::ReceiveStream* strm) {
    // FIXME This should only be triggered when the underlying network queue
    // went empty -> non-empty, so hopefully lock contention shouldn't be an issue.

    SILOG(fairreceiver,insane,"Received network data from " << strm->id());

    {
        boost::lock_guard<boost::mutex> lck(mMutex);
        // Given the new data we need to update our view of the world
        mReceiveQueues.notifyPushFront(strm->id());
    }

    // And run service (which will handle setting up
    // any new service callbacks that may be needed).
    scheduleServicing();
}

} // namespace Sirikata
