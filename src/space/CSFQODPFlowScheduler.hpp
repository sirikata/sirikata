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

namespace CBR {

class ServerWeightCalculator;

/** CSFQODPFlowScheduler tracks all active flows and uses a CSFQ-style
 *  approach to enforce fairness over those flows.
 */
class CSFQODPFlowScheduler : public ODPFlowScheduler {
public:
    CSFQODPFlowScheduler(SpaceContext* ctx, ForwarderServiceQueue* parent, ServerID sid, uint32 serv_id, uint32 max_size);
    virtual ~CSFQODPFlowScheduler();

    // Interface: AbstractQueue<Message*>
    virtual const Type& front() const { return mQueue.front().msg; }
    virtual Type& front() { return mQueue.front().msg; }
    virtual Type pop();
    virtual bool empty() const { return mQueue.empty(); }
    virtual uint32 size() const { return mQueue.size(); }

    // ODP push interface
    virtual bool push(CBR::Protocol::Object::ObjectMessage* msg);
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
                return std::tr1::hash<unsigned int>()(op.source.hash()^op.dest.hash());
            }
        };

        UUID source;
        UUID dest;
    };

    struct FlowInfo {
        FlowInfo(double w)
         : weight(w)
        {
            for(int i = 0; i < NUM_DOWNSTREAM; i++)
                usedWeight[i] = w;
        }

        RateEstimator rate;
        double weight;
        double usedWeight[NUM_DOWNSTREAM];
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

    FlowInfo* getFlow(const ObjectPair& new_packet_pair);
    void removeFlow(const ObjectPair& packet_pair);
    int flowCount() const;
    float normalizedFlowWeight(float unnorm_weight);

    void estimateAlpha(int32 packet_size, Time& arrival_time, double label, bool dropped);
    bool queueExceedsLowWaterMark() const { return true; } // Not necessary in our implementation
    double minCongestedAlpha() const { return mCapacityRate.get() / std::max(1, flowCount()); }

    Queue<QueuedMessage> mQueue;
    ServerWeightCalculator* mWeightCalculator;

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
