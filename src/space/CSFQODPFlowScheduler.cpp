/*  cbr
 *  CSFQODPFlowScheduler.cpp
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

#include "CSFQODPFlowScheduler.hpp"
#include "ServerWeightCalculator.hpp"
#include "LocationService.hpp"
#include "CoordinateSegmentation.hpp"
#include "Random.hpp"
#include "craq_oseg/CraqEntry.hpp"
#include "Statistics.hpp"
#define _Kf (Duration::milliseconds((int64)10000))
#define _Kf_double (_Kf.toSeconds())
#define _Ka (Duration::milliseconds((int64)200))
#define _Ka_double (_Ka.toSeconds())

#define KALPHA 29 // Max times fair rate can be decreased during interval

#define CSFQLOG(level, msg) SILOG(csfqodp,level, mContext->id() << "->" << mDestServer << ": " << msg)

namespace CBR {

CSFQODPFlowScheduler::CSFQODPFlowScheduler(SpaceContext* ctx, ForwarderServiceQueue* parent, ServerID sid, uint32 serv_id, uint32 max_size, LocationService* loc)
 : ODPFlowScheduler(ctx, parent, sid, serv_id),
   mQueueBuffer(),
   mQueue(Sirikata::SizedResourceMonitor(max_size)),
   mNeedsNotification(true),
   mLoc(loc),
   mArrivalRate(_Ka_double),
   mSumEstimatedArrivalRates(0),
   mAcceptedRate(_Ka_double),
   mCapacityRate(_Ka_double),
   mAlpha(0.0),
   mAlphaWindowed(0.0),
   mCongested(true),
   mCongestionStartTime(Time::null()),
   mCongestionWindow(_Ka),
   mKAlphaReductionsLeft(KALPHA),
   mTotalActiveWeight(0)
{
    mWeightCalculator = WeightCalculatorFactory(mContext->cseg());

    for(int i = 0; i < NUM_DOWNSTREAM; i++)
        mTotalUsedWeight[i] = 0.0;
}

CSFQODPFlowScheduler::~CSFQODPFlowScheduler() {
    delete mWeightCalculator;

#ifdef CSFQODP_DEBUG
    CSFQLOG(warn,"Flow");
    for(FlowMap::iterator flow_it = mFlows.begin(); flow_it != mFlows.end(); flow_it++) {
        ObjectPair op = flow_it->first;
        const FlowInfo& fi = flow_it->second;
        CSFQLOG(warn,"  " <<
            "[" << op.source.toString() << ":" << op.dest.toString() << "] " <<
            "weight: " << fi.weight <<
            " sused: " << fi.usedWeight[SENDER] <<
            " rused: " << fi.usedWeight[RECEIVER] <<
            " -> arrived: " << fi.arrived <<
            " accepted: " << fi.accepted
        );
    }
#endif
}

// ODP push interface
bool CSFQODPFlowScheduler::push(CBR::Protocol::Object::ObjectMessage* msg, const CraqEntry&source_entry, const CraqEntry& dest_entry) {
    boost::lock_guard<boost::mutex> lck(mPushMutex); // FIXME

    ObjectPair op(msg->source_object(), msg->dest_object());
    Time curtime = mContext->recentSimTime();
    FlowInfo* flow_info = getFlow(op,source_entry,dest_entry, curtime);

    // FIXME update weights, due to possible movement?
    double weight = flow_info->weight;

    // Priority computation failure...
    if (!weight) {
        return false;
    }

    int32 packet_size = msg->ByteSize();

#ifdef CSFQODP_DEBUG
    flow_info->arrived += packet_size;
#endif

    double label = 0;
#define _edge true // Maybe someday we'll bother with core routers
    if (_edge) {
        // Remove old used weight
        double savedTotalUsedWeight[NUM_DOWNSTREAM]={0};

        for(int i = 0; i < NUM_DOWNSTREAM; i++) {
            savedTotalUsedWeight[i]=mTotalUsedWeight[i];
            mTotalUsedWeight[i] -= flow_info->usedWeight[i];
        }
        // Compute label, updating the rate
        double old_rate = flow_info->rate.get();
        mSumEstimatedArrivalRates -= old_rate;
        double est_flow_rate = flow_info->rate.estimate_rate(curtime, packet_size, _Kf_double);
        mSumEstimatedArrivalRates += est_flow_rate;
        double flow_rate =
            (mSumEstimatedArrivalRates == 0) ?
            mArrivalRate.get() :
            (est_flow_rate / mSumEstimatedArrivalRates) * mArrivalRate.get();
        double w_norm = normalizedFlowWeight(weight);
        label = flow_rate / w_norm;

        // If we were setting label annotations, we would do it here
        // setLabel(p,label);

        // Add updated used weight
        // For acc_rate and total_rates, use_global_values controls
        // whether we're using the values for the entire fair queue
        // (which must be collected downstream and reported back) or
        // the local values (which can obviously only take into
        // account flows we know about).  Note that the correct
        // setting is use_global_values == true.
        double sender_acc_rate = std::max(mSenderCapacity, 1.0);
        // Using the max avoids possible zeros
        double sender_total_weights = std::max(mSenderTotalWeight, savedTotalUsedWeight[SENDER]);
        flow_info->usedWeight[SENDER] = std::min(flow_rate * (sender_total_weights / sender_acc_rate), weight);

        double receiver_acc_rate = std::max(mReceiverCapacity, 1.0);
        // Using the max avoids possible zeros
        double receiver_total_weights = std::max(mReceiverTotalWeight, savedTotalUsedWeight[RECEIVER]);
        flow_info->usedWeight[RECEIVER] = std::min(flow_rate * (receiver_total_weights / receiver_acc_rate), weight);

        for(int i = 0; i < NUM_DOWNSTREAM; i++)
            mTotalUsedWeight[i] += flow_info->usedWeight[i];
    }

    double prob_drop = std::max(0.0, label ? 1.0 - mAlpha / label : 0);

    // Initialization and reinitialization case: if mAlpha is 0, allow
    // everything through.
    if (mAlpha != 0.0) {
        if ((randFloat() < prob_drop)) {
            estimateAlpha(packet_size, curtime, label, true);
            TRACE_DROP(DROPPED_CSFQ_PROBABILISTIC);
            return false;
        }
    }

    // We're not labelling packets, but in case we do at some point, this is
    // where we'd do it.
    //if (prob_drop > 0) {
        // Annotate packet, label = mAlpha
    //}

    // Try to enqueue.
    Message* serv_msg = createMessageFromODP(msg, mDestServer);
    QueuedMessage qmsg(serv_msg, packet_size);
    bool enqueue_success = mQueue.push(qmsg, false);
    // If we overflowed, drop and adjust alpha
    if (!enqueue_success) {
        if (mKAlphaReductionsLeft-- >= 0)
            mAlpha *= 0.99;
        delete qmsg.msg;
        TRACE_DROP(DROPPED_CSFQ_OVERFLOW);
        return false;
    }

    // Finally, restimate alpha.
#ifdef CSFQODP_DEBUG
    flow_info->accepted += packet_size;
#endif
    estimateAlpha(packet_size, curtime, label, false);

    if (mNeedsNotification) {
        mNeedsNotification = false;
        notifyPushFront();
    }

    return true;
}

void CSFQODPFlowScheduler::estimateAlpha(int32 packet_size, Time& arrival_time, double label, bool dropped) {
    mArrivalRate.estimate_rate(arrival_time, packet_size);
    if (!dropped)
        mAcceptedRate.estimate_rate(arrival_time, packet_size);

    // compute the initial value of mAlpha
    if (mAlpha == 0.) {
        if (!queueExceedsLowWaterMark()) {
            if (mAlphaWindowed < label) mAlphaWindowed = label;
            return;
        }
        if (mAlpha < mAlphaWindowed)
            mAlpha = mAlphaWindowed;
        if (mAlpha == 0.)
            mAlpha = minCongestedAlpha(); // arbitrary initialization
        mAlphaWindowed = 0.;
    }

    // Update mAlpha

    // We need to deal with both down stream queues. The capacity dedicated to
    // us is the only parameter dependent on the downstream queues.  Therefore,
    // we compute for both and work with the minimum.
    double sender_cap = mSenderCapacity;
    double sender_total_weights = std::max(mSenderTotalWeight, mTotalUsedWeight[SENDER]);
    double sfrac = (sender_total_weights != 0.0) ? mTotalUsedWeight[SENDER] / sender_total_weights : 1.0;
    sender_cap *= mTotalUsedWeight[SENDER] / mSenderTotalWeight;
    double receiver_cap = mReceiverCapacity;
    double receiver_total_weights = std::max(mReceiverTotalWeight, mTotalUsedWeight[RECEIVER]);
    double rfrac = (receiver_total_weights != 0.0) ? mTotalUsedWeight[RECEIVER] / receiver_total_weights : 1.0;
    receiver_cap *= rfrac;

    double cap = std::min(sender_cap, receiver_cap);

    if (cap <= mArrivalRate.get()) { // link congested
        if (!mCongested) { // uncongested -> congested
            mCongested = true;
            mCongestionStartTime = arrival_time;
            mKAlphaReductionsLeft = KALPHA;
        } else {
            if (arrival_time < mCongestionStartTime + mCongestionWindow)
                return;
            mCongestionStartTime = arrival_time;
            mAlpha *= cap/mAcceptedRate.get(arrival_time, _Ka_double);
            if (cap < mAlpha) {
                mAlpha = cap;
            }
        }
    } else {  // (_capacity_rate.get() > mArrivalRate.get()) => link uncongested
        if (mCongested) { // congested -> uncongested
            mCongested = false;
            mCongestionStartTime = arrival_time;
            mAlphaWindowed = 0;
        } else {
            if (arrival_time < mCongestionStartTime + mCongestionWindow) {
                if (mAlphaWindowed < label) mAlphaWindowed = label;
            } else {
                mAlpha = mAlphaWindowed;
                mCongestionStartTime = arrival_time;
                if (!queueExceedsLowWaterMark())
                    mAlpha = 0.;
                else
                    mAlphaWindowed = 0.;
            }
        }
    }
}

static CSFQODPFlowScheduler::Type null_response = NULL;

const CSFQODPFlowScheduler::Type& CSFQODPFlowScheduler::front() const {
    if (mQueueBuffer.msg == NULL) {
        bool avail = mQueue.pop(mQueueBuffer);
        if (!avail) {
            mNeedsNotification = true;
            return null_response;
        }
    }

    return mQueueBuffer.msg;
}

CSFQODPFlowScheduler::Type& CSFQODPFlowScheduler::front() {
    if (mQueueBuffer.msg == NULL) {
        bool avail = mQueue.pop(mQueueBuffer);
        if (!avail) {
            mNeedsNotification = true;
            return null_response;
        }
    }

    return mQueueBuffer.msg;
}

bool CSFQODPFlowScheduler::empty() const {
    bool is_empty = mQueueBuffer.msg == NULL && mQueue.probablyEmpty();
    if (is_empty) mNeedsNotification = true;
    return is_empty;
}

CSFQODPFlowScheduler::Type CSFQODPFlowScheduler::pop() {
    front(); // Prime front element
    QueuedMessage result = mQueueBuffer;
    mQueueBuffer = QueuedMessage();
    if (result.msg == NULL)
        return NULL;

    // If the queue reads as empty after a pop, we're going to need
    // notification.  Otherwise, even if it got emptied, we weren't able to
    // observe it before it got another element.
    front(); // Reprimes front element, might mark for notification on next round

    mCapacityRate.estimate_rate(mContext->recentSimTime(), result.size());
    return result.msg;
}


// Get the sum of the weights of active queues.
float CSFQODPFlowScheduler::totalActiveWeight() {
    return mTotalActiveWeight;
}

// Get the total used weight of active queues.  If all flows are saturating,
// this should equal totalActiveWeights, otherwise it will be smaller.
float CSFQODPFlowScheduler::totalSenderUsedWeight() {
    // No flow tracking, so we just give the entire server weight
    return mTotalUsedWeight[SENDER];
}

// Get the total used weight of active queues.  If all flows are saturating,
// this should equal totalActiveWeights, otherwise it will be smaller.
float CSFQODPFlowScheduler::totalReceiverUsedWeight() {
    return mTotalUsedWeight[RECEIVER];
}

BoundingBox3f CSFQODPFlowScheduler::getObjectWeightRegion(const UUID& objid, const CraqEntry& info) const {
    // We might have exact info
    if (mLoc->contains(objid)) {
        Vector3f pos = mLoc->currentPosition(objid);
        BoundingSphere3f bounds = mLoc->bounds(objid);
        BoundingBox3f bb(pos + bounds.center(), bounds.radius());
        return bb;
    }

    if (info.server() == mContext->id())
        CSFQLOG(warn,"Using approximation for local object!");
    if (info.radius()==1.0) {
        CSFQLOG(warn,"Radius approximation failure! (migration? should we do a cache lookup)");
    }
    // Otherwise, we need to use server info
    // Blech, why is this a bbox list?
    BoundingBoxList server_bbox_list = mContext->cseg()->serverRegion(info.server());
    BoundingBox3f server_bbox = BoundingBox3f::null();
    for(uint32 i = 0; i < server_bbox_list.size(); i++)
        server_bbox.mergeIn(server_bbox_list[i]);
    return BoundingBox3f(server_bbox.center(), info.radius());
}

CSFQODPFlowScheduler::FlowInfo* CSFQODPFlowScheduler::getFlow(const ObjectPair& new_packet_pair, const CraqEntry&source_info, const CraqEntry&dst_info, const Time& t) {
    FlowMap::iterator where = mFlows.find(new_packet_pair);
    if (where==mFlows.end()) {
        BoundingBox3f source_bbox = getObjectWeightRegion(new_packet_pair.source, source_info);
        BoundingBox3f dest_bbox = getObjectWeightRegion(new_packet_pair.dest, dst_info);

        double weight = mWeightCalculator->weight(source_bbox, dest_bbox);

        std::pair<FlowMap::iterator, bool> ins_it = mFlows.insert(FlowMap::value_type(new_packet_pair,FlowInfo(weight, t)));
        assert(ins_it.second == true);

        mTotalActiveWeight += weight;
        for(int i = 0; i < NUM_DOWNSTREAM; i++)
            mTotalUsedWeight[i] += weight;

        where = ins_it.first;
    }
    return &(where->second);
}

void CSFQODPFlowScheduler::removeFlow(const ObjectPair& packet_pair) {
    FlowMap::iterator where = mFlows.find(packet_pair);
    assert(where != mFlows.end());
    assert(0);
    FlowInfo* fi = &(where->second);
    mTotalActiveWeight -= fi->weight;
    for(int i = 0; i < NUM_DOWNSTREAM; i++)
        mTotalUsedWeight[i] -= fi->usedWeight[i];
    fi = NULL;
    mFlows.erase(where);
}

int CSFQODPFlowScheduler::flowCount() const {
    return mFlows.size();
}

float CSFQODPFlowScheduler::normalizedFlowWeight(float unnorm_weight) {
    // We need normalized weights or else things won't add up properly to give
    // us C total output.  The paper ignores this, presumably because they have
    // a static set of weights which they have already normalized.
    return unnorm_weight / mTotalActiveWeight;
}

} // namespace CBR
