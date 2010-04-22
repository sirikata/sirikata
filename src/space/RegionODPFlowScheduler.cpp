/*  cbr
 *  RegionODPFlowScheduler.cpp
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

#include "RegionODPFlowScheduler.hpp"
#include "ServerWeightCalculator.hpp"

namespace CBR {

RegionODPFlowScheduler::RegionODPFlowScheduler(SpaceContext* ctx, ForwarderServiceQueue* parent, ServerID sid, uint32 serv_id, uint32 max_size)
 : ODPFlowScheduler(ctx, parent, sid, serv_id),
   mQueueBuffer(),
   mQueue(Sirikata::SizedResourceMonitor(max_size)),
   mNeedsNotification(true)
{
    mWeightCalculator = WeightCalculatorFactory(mContext->cseg());
}

RegionODPFlowScheduler::~RegionODPFlowScheduler() {
    delete mWeightCalculator;
}

// ODP push interface
bool RegionODPFlowScheduler::push(CBR::Protocol::Object::ObjectMessage* msg) {
    Message* serv_msg = createMessageFromODP(msg, mDestServer);
    if (!mQueue.push(serv_msg, false)) {
        delete serv_msg;
        return false;
    }

    if (mNeedsNotification) {
        mNeedsNotification = false;
        notifyPushFront();
    }
    return true;
}


static RegionODPFlowScheduler::Type null_response = NULL;

const RegionODPFlowScheduler::Type& RegionODPFlowScheduler::front() const {
    if (mQueueBuffer == NULL) {
        bool avail = mQueue.pop(mQueueBuffer);
        if (!avail) {
            mNeedsNotification = true;
            return null_response;
        }
    }

    return mQueueBuffer;
}

RegionODPFlowScheduler::Type& RegionODPFlowScheduler::front() {
    if (mQueueBuffer == NULL) {
        bool avail = mQueue.pop(mQueueBuffer);
        if (!avail) {
            mNeedsNotification = true;
            return null_response;
        }
    }

    return mQueueBuffer;
}

bool RegionODPFlowScheduler::empty() const {
    bool is_empty = mQueueBuffer == NULL && mQueue.probablyEmpty();
    if (is_empty) mNeedsNotification = true;
    return is_empty;
}

RegionODPFlowScheduler::Type RegionODPFlowScheduler::pop() {
    front(); // Prime front element
    Message* result = mQueueBuffer;
    mQueueBuffer = NULL;
    if (result == NULL)
        return NULL;

    // If the queue reads as empty after a pop, we're going to need
    // notification.  Otherwise, even if it got emptied, we weren't able to
    // observe it before it got another element.
    front(); // Reprimes front element, might mark for notification on next round

    return result;
}


// Get the sum of the weights of active queues.
float RegionODPFlowScheduler::totalActiveWeight() {
    return mWeightCalculator->weight(mContext->id(), mDestServer);
}

// Get the total used weight of active queues.  If all flows are saturating,
// this should equal totalActiveWeights, otherwise it will be smaller.
float RegionODPFlowScheduler::totalSenderUsedWeight() {
    // No flow tracking, so we just give the entire server weight
    return mWeightCalculator->weight(mContext->id(), mDestServer);
}

// Get the total used weight of active queues.  If all flows are saturating,
// this should equal totalActiveWeights, otherwise it will be smaller.
float RegionODPFlowScheduler::totalReceiverUsedWeight() {
    // No flow tracking, so we just give the entire server weight
    return mWeightCalculator->weight(mContext->id(), mDestServer);
}

} // namespace CBR
