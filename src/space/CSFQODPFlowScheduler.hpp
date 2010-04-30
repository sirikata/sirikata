/*  cbr
 *  CSFQODPFlowScheduler.hpp
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

#ifndef _CSFQ_ODP_FLOW_SCHEDULER_HPP_
#define _CSFQ_ODP_FLOW_SCHEDULER_HPP_

#include "ODPFlowScheduler.hpp"
#include "Queue.hpp"
#include "RateEstimator.hpp"
#include <sirikata/util/SizedThreadSafeQueue.hpp>

//#define CSFQODP_DEBUG

namespace CBR {

class ServerWeightCalculator;
class LocationService;

/** CSFQODPFlowScheduler tracks all active flows and uses a CSFQ-style
 *  approach to enforce fairness over those flows.
 */
class CSFQODPFlowScheduler : public ODPFlowScheduler {
public:
    CSFQODPFlowScheduler(SpaceContext* ctx, ForwarderServiceQueue* parent, ServerID sid, uint32 serv_id, uint32 max_size, LocationService* loc);
    virtual ~CSFQODPFlowScheduler();

    // Interface: AbstractQueue<Message*>
    virtual const Type& front() const;
    virtual Type& front();
    virtual Type pop();
    virtual bool empty() const;
    virtual uint32 size() const { return mQueue.getResourceMonitor().filledSize(); }

    // ODP push interface
    virtual bool push(CBR::Protocol::Object::ObjectMessage* msg, const CraqEntry&, const CraqEntry&);
    // Get the sum of the weights of active queues.
    virtual float totalActiveWeight();
    // Get the total used weight of active queues.  If all flows are saturating,
    // this should equal totalActiveWeights, otherwise it will be smaller.
    virtual float totalSenderUsedWeight();
    // Get the total used weight of active queues.  If all flows are saturating,
    // this should equal totalActiveWeights, otherwise it will be smaller.
    virtual float totalReceiverUsedWeight();
private:

    enum {
        SENDER = 0,
        RECEIVER = 1,
        NUM_DOWNSTREAM = 2
    };

    struct ObjectPair {
        ObjectPair(const UUID& s, const UUID& d)
         : source(s), dest(d)
        {}

        bool operator<(const ObjectPair& rhs) const {
            return (source < rhs.source || (source == rhs.source && dest < rhs.dest));
        }

        bool operator==(const ObjectPair& rhs) const {
            return (source == rhs.source && dest == rhs.dest);
        }

        class Hasher {
        public:
            size_t operator() (const ObjectPair& op) const {
                return *(uint32*)op.source.getArray().data() ^ *(uint32*)op.dest.getArray().data();
            }
        };

        UUID source;
        UUID dest;
    };

    struct FlowInfo {
        FlowInfo(double w)
         : weight(w)
#ifdef CSFQODP_DEBUG
           ,
           arrived(0),
           accepted(0)
#endif
        {
            for(int i = 0; i < NUM_DOWNSTREAM; i++)
                usedWeight[i] = w;
        }

        RateEstimator rate;
        double weight;
        double usedWeight[NUM_DOWNSTREAM];
#ifdef CSFQODP_DEBUG
        uint64 arrived;
        uint64 accepted;
#endif
    };

    struct QueuedMessage {
        QueuedMessage()
         : msg(NULL),
           _size(0)
        {}

        QueuedMessage(Message* m, int32 s)
         : msg(m),
           _size(s)
        {}

        int32 size() const { return _size; }

        Message* msg;
        int32 _size;
    };

    FlowInfo* getFlow(const ObjectPair& new_packet_pair, const CraqEntry&src_info,const CraqEntry&dst_info);
    void removeFlow(const ObjectPair& packet_pair);
    int flowCount() const;
    float normalizedFlowWeight(float unnorm_weight);

    void estimateAlpha(int32 packet_size, Time& arrival_time, double label, bool dropped);
    bool queueExceedsLowWaterMark() const { return true; } // Not necessary in our implementation
    double minCongestedAlpha() const { return mCapacityRate.get() / std::max(1, flowCount()); }

    // Helper to get the region we compute weight over
    BoundingBox3f getObjectWeightRegion(const UUID& objid, const CraqEntry& sid) const;


    boost::mutex mPushMutex;

    // Note: unfortunately we need to mark these as mutable because a)
    // SizedThreadSafeQueue doesn't have methods marked properly as const and b)
    // ThreadSafeQueue doesn't provide a front() method.
    mutable QueuedMessage mQueueBuffer;
    mutable Sirikata::SizedThreadSafeQueue<QueuedMessage> mQueue;
    mutable Sirikata::AtomicValue<bool> mNeedsNotification;
    ServerWeightCalculator* mWeightCalculator;
    // Used to collect information for weight computation
    LocationService* mLoc;

    // CSFQ Summary Information
    SimpleRateEstimator mArrivalRate;
    SimpleRateEstimator mAcceptedRate;
    SimpleRateEstimator mCapacityRate; // Actual capacity observed (no dummy packets)

    // CSFQ Control Information
    double mAlpha;
    double mAlphaWindowed;
    bool mCongested;
    Time mCongestionStartTime;
    Duration mCongestionWindow;
    int mKAlphaReductionsLeft;

    // Per Flow Information
    typedef std::tr1::unordered_map<ObjectPair, FlowInfo, ObjectPair::Hasher> FlowMap;
    FlowMap mFlows;
    // Flow Summary Information
    double mTotalActiveWeight;
    double mTotalUsedWeight[NUM_DOWNSTREAM];
}; // class CSFQODPFlowScheduler

} // namespace CBR

#endif //_CSFQ_ODP_FLOW_SCHEDULER_HPP_
