/*  Sirikata
 *  FairServerMessageQueue.cpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#include "FairServerMessageQueue.hpp"
#include <sirikata/space/SpaceNetwork.hpp>
#include <sirikata/space/ServerMessage.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/trace/Trace.hpp>

namespace Sirikata{


FairServerMessageQueue::SenderAdapterQueue::SenderAdapterQueue(Sender* sender, ServerID sid)
        : mSender(sender),
          mDestServer(sid),
          mFront(NULL)
{
}

Message* FairServerMessageQueue::SenderAdapterQueue::front() {
    if (mFront != NULL)
        return mFront;

    mFront = mSender->serverMessagePull(mDestServer);
    return mFront;
}

Message* FairServerMessageQueue::SenderAdapterQueue::pop() {
    Message* result = front();
    mFront = NULL;
    return result;
}

bool FairServerMessageQueue::SenderAdapterQueue::empty() {
    // Don't bother with server message empty since it'll require a lock anyway,
    // just try pulling and buffering.
    return front() == NULL;
}

FairServerMessageQueue::FairServerMessageQueue(SpaceContext* ctx, SpaceNetwork* net, Sender* sender)
        : ServerMessageQueue(ctx, net, sender),
          mServerQueues(),
          mServiceScheduled(false),
          mStoppedBlocked(0),
          mStoppedUnderflow(0)
{
}

FairServerMessageQueue::~FairServerMessageQueue() {
    SILOG(fsmq,info,
        "FSMQ: underflow: " << mStoppedUnderflow <<
        ", blocked: " << mStoppedBlocked
    );
}

void FairServerMessageQueue::scheduleServicing() {
    if (!mServiceScheduled.read()) {
        mServiceScheduled = true;
        mSenderStrand->post(
            std::tr1::bind(&FairServerMessageQueue::service, this)
        );
    }
}

void FairServerMessageQueue::service() {
#define MAX_MESSAGES_PER_ROUND 100

    mProfiler->started();

    // Unmark scheduling
    mServiceScheduled = false;

    Time tcur = mContext->simTime();

    // Send
    Message* next_msg = NULL;
    ServerID sid;
    bool last_blocked = false;
    uint32 num_sent = 0;
    uint32 cum_sent_size = 0;
    while( num_sent < MAX_MESSAGES_PER_ROUND && !mContext->stopped() ) {
        uint32 packet_size = 0;
        {
            MutexLock lck(mMutex);

            next_msg = mServerQueues.front(&sid);
            if (next_msg == NULL )
                break;

            last_blocked = false;

            uint32 sent_size = trySend(sid, next_msg);
            bool sent_success = (sent_size != 0);
            if (!sent_success) {
                last_blocked = true;
                disableDownstream(sid);
                continue;
            }

            // Pop the message
            Message* next_msg_popped = mServerQueues.pop();
            assert(next_msg == next_msg_popped);

            cum_sent_size += sent_size;
        }

        // Record trace send times
        // FIXME serverDatagramSent can't rely on these anymore
        /*
        static Time start_time = Time::null();
        static Time end_time = Time::null();
        {
            // Lock needed for getQueueWeight
            MutexLock lck(mMutex);
        CONTEXT_TRACE_NO_TIME(serverDatagramSent, start_time, end_time, mServerQueues.getQueueWeight(next_msg->dest_server()),
            next_msg->dest_server(), next_msg->id(), packet_size);
        }
        */

        // Get rid of the message
        num_sent++;
        delete next_msg;
    }


    if (num_sent == MAX_MESSAGES_PER_ROUND) {
        mBlocked = true;
        // We ran out of our budget, need to reschedule.
        scheduleServicing();
        mStoppedBlocked++;
    }
    else {
        // There are 2 ways we could get here:
        // 1. mServerQueues.empty() == true: We ran out of input messages to
        // send.  We have to discard the bytes we didn't have input for. We
        // track this manually to avoid locking for mServerQueues
        // 2. Otherwise, we had enough bytes, there were items in the queue,
        // but we couldn't get a front item.  This means all downstream
        // queues were blocking us from sending, so we should discard the
        // bytes so we don't artificially collect extra, old bandwidth over
        // time.
        if (!last_blocked) {
            mBlocked = false;
            mStoppedUnderflow++;
        }
        else {
            mBlocked = true;
            mStoppedBlocked++;
        }
    }

    // Update capacity estimate.
    mCapacityEstimator.estimate_rate(tcur, cum_sent_size);
    mProfiler->finished();
}


void FairServerMessageQueue::messageReady(ServerID sid) {
    MutexLock lck(mMutex);

    // Make sure we are setup for this server. We don't have a weight for this
    // server yet, so we must use a default. We use the average of all known
    // weights, and hope to get an update soon.
    if (!mServerQueues.hasQueue(sid)) {
        addInputQueue(
            sid,
            getAverageServerWeight()
        );
    }

    // Notify and service
    mServerQueues.notifyPushFront(sid);
    scheduleServicing();
}

float FairServerMessageQueue::getAverageServerWeight() const {
    // NOTE: MUST have mMutex locked, currently only called from messageReady.
    return mServerQueues.avg_weight();
}

void FairServerMessageQueue::networkReadyToSend(const ServerID& from) {
    // The connection is ready to send again, enable the input queue associated
    // with it.
    //mSenderStrand->post(
    //    std::tr1::bind(&FairServerMessageQueue::enableDownstream, this, from)
    //                           );

    SILOG(fsmq,info, "Network ready: " << (mContext->simTime()-Time::null()).toSeconds());
    enableDownstream(from);
}

void FairServerMessageQueue::addInputQueue(ServerID sid, float weight) {
    // NOTE: MUST have lock, acquired by messageReady or handleUpdateReceiverStats

    assert( !mServerQueues.hasQueue(sid) );
    SILOG(fairsender,info,"Adding input queue for " << sid);
    mServerQueues.addQueue(new SenderAdapterQueue(mSender,sid),sid,weight);
}

void FairServerMessageQueue::handleUpdateReceiverStats(ServerID sid, double total_weight, double used_weight) {
    MutexLock lck(mMutex);

    if (!mServerQueues.hasQueue(sid))
        addInputQueue(sid, used_weight);
    else
        mServerQueues.setQueueWeight(sid, used_weight);
}

void FairServerMessageQueue::removeInputQueue(ServerID sid) {
    MutexLock lck(mMutex);

    assert( mServerQueues.hasQueue(sid) );
    mServerQueues.removeQueue(sid);
}

void FairServerMessageQueue::enableDownstream(ServerID sid) {
    MutexLock lck(mMutex);

    // Will ignore queues it doesn't know about.
    mServerQueues.enableQueue(sid);

    scheduleServicing();
}

void FairServerMessageQueue::disableDownstream(ServerID sid) {
    SILOG(fsmq,info, "Network disabled: " << (mContext->simTime()-Time::null()).toSeconds());
    // NOTE: must ensure mMutex is locked
    mServerQueues.disableQueue(sid);
}

}
