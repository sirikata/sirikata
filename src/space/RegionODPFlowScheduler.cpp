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
#include "ExpIntegral.hpp"
#include "SqrIntegral.hpp"
#include "Options.hpp"

namespace CBR {

RegionODPFlowScheduler::RegionODPFlowScheduler(SpaceContext* ctx, ForwarderServiceQueue* parent, ServerID sid, uint32 serv_id, uint32 max_size)
 : ODPFlowScheduler(ctx, parent, sid, serv_id),
   mQueue(max_size)
{
    if (GetOption("gaussian")->as<bool>()) {
        mWeightCalculator =
            new ServerWeightCalculator(
                mContext->cseg(),
                std::tr1::bind(&integralExpFunction,GetOption("flatness")->as<double>(),
                    std::tr1::placeholders::_1,
                    std::tr1::placeholders::_2,
                    std::tr1::placeholders::_3,
                    std::tr1::placeholders::_4)
            );
    } else {
        mWeightCalculator =
            new ServerWeightCalculator(
                mContext->cseg(),
                std::tr1::bind(SqrIntegral(false),GetOption("const-cutoff")->as<double>(),GetOption("flatness")->as<double>(),
                    std::tr1::placeholders::_1,
                    std::tr1::placeholders::_2,
                    std::tr1::placeholders::_3,
                    std::tr1::placeholders::_4)
                                       );
    }
}

RegionODPFlowScheduler::~RegionODPFlowScheduler() {
    delete mWeightCalculator;
}

// ODP push interface
bool RegionODPFlowScheduler::push(CBR::Protocol::Object::ObjectMessage* msg) {
    bool was_empty = empty();

    Message* serv_msg = createMessageFromODP(msg, mDestServer);
    if (mQueue.push(serv_msg) == QueueEnum::PushExceededMaximumSize) {
        delete serv_msg;
        return false;
    }

    notifyPushFront();
    return true;
}

// Get the sum of the weights of active queues.
float RegionODPFlowScheduler::totalActiveWeight() {
    return mWeightCalculator->weight(mContext->id(), mDestServer);
}

// Get the total used weight of active queues.  If all flows are saturating,
// this should equal totalActiveWeights, otherwise it will be smaller.
float RegionODPFlowScheduler::totalUsedWeight() {
    // No flow tracking, so we just give the entire server weight
    return mWeightCalculator->weight(mContext->id(), mDestServer);
}

} // namespace CBR
