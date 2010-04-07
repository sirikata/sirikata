/*  cbr
 *  RegionODPFlowScheduler.hpp
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

#ifndef _REGION_ODP_FLOW_SCHEDULER_HPP_
#define _REGION_ODP_FLOW_SCHEDULER_HPP_

#include "ODPFlowScheduler.hpp"
#include "Queue.hpp"

namespace CBR {

class ServerWeightCalculator;

/** RegionODPFlowScheduler doesn't collect any real statistics about ODP flows.
 *  Instead, it uses a simple FIFO queue for packets and just reports
 *  region-to-region weights.
 */
class RegionODPFlowScheduler : public ODPFlowScheduler {
public:
    RegionODPFlowScheduler(SpaceContext* ctx, ForwarderServiceQueue* parent, ServerID sid, uint32 serv_id, uint32 max_size);
    virtual ~RegionODPFlowScheduler();

    // Interface: AbstractQueue<Message*>
    virtual const Type& front() const { return mQueue.front(); }
    virtual Type& front() { return mQueue.front(); }
    virtual Type pop() { return mQueue.pop(); }
    virtual bool empty() const { return mQueue.empty(); }
    virtual uint32 size() const { return mQueue.size(); }

    // ODP push interface
    virtual bool push(CBR::Protocol::Object::ObjectMessage* msg);
    // Get the sum of the weights of active queues.
    virtual float totalActiveWeight();

private:
    Queue<Message*> mQueue;
    ServerWeightCalculator* mWeightCalculator;
}; // class RegionODPFlowScheduler

} // namespace CBR

#endif //_REGION_ODP_FLOW_SCHEDULER_HPP_
