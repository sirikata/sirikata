/*  cbr
 *  ServerWeightCalculator.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include "ServerWeightCalculator.hpp"
#include "ServerMessageQueue.hpp"
#include "ServerMessageReceiver.hpp"

namespace CBR {

ServerWeightCalculator::ServerWeightCalculator(CoordinateSegmentation* cseg, const WeightFunction& weightFunc)
 : mCSeg(cseg),
   mWeightFunc(weightFunc)
{
}

ServerWeightCalculator::~ServerWeightCalculator() {
}

float64 ServerWeightCalculator::weight(const BoundingBox3f& source_bbox, BoundingBox3f& dest_bbox) {
    return mWeightFunc(Vector3d(source_bbox.min()),Vector3d(source_bbox.max()),Vector3d(dest_bbox.min()),Vector3d(dest_bbox.max()));
}

float64 ServerWeightCalculator::weight(ServerID source, ServerID dest, Normalization norm) {
    BoundingBox3f source_bbox = mCSeg->serverRegion(source)[0];
    BoundingBox3f dest_bbox = mCSeg->serverRegion(dest)[0];

    //how much data am I able to send if I was sending everywhere
    double totalSendBandwidth=mWeightFunc(Vector3d(source_bbox.min()),Vector3d(source_bbox.max()),Vector3d(mCSeg->region().min()),Vector3d(mCSeg->region().max()))-mWeightFunc(Vector3d(source_bbox.min()),Vector3d(source_bbox.max()),Vector3d(source_bbox.min()),Vector3d(source_bbox.max()));
    //how much data is my destination able to receive
    double totalReceiveBandwidth=mWeightFunc(Vector3d(mCSeg->region().min()),Vector3d(mCSeg->region().max()),Vector3d(dest_bbox.min()),Vector3d(dest_bbox.max()))-mWeightFunc(Vector3d(dest_bbox.min()),Vector3d(dest_bbox.max()),Vector3d(dest_bbox.min()),Vector3d(dest_bbox.max()));
    //take the min of recv and send
    double totalBandwidthCap=totalSendBandwidth>totalReceiveBandwidth?totalSendBandwidth:totalReceiveBandwidth;
    //we only want to normalize by what the poor fool will receive, our fair queue will apportion our send bandwidth

    double unnormalized_result = weight(source_bbox, dest_bbox);
    double result = unnormalized_result;

    switch (norm) {
      case DO_NOT_NORMALIZE:
        result = unnormalized_result;
        break;
      case NORMALIZE_BY_MIN_SEND_AND_RECEIVE_RATE:
        result = (totalBandwidthCap == 0) ? 0 : unnormalized_result/totalBandwidthCap;
        break;
      case NORMALIZE_BY_RECEIVE_RATE:
        result = (totalReceiveBandwidth == 0)?0:unnormalized_result/totalReceiveBandwidth;
        break;
      case NORMALIZE_BY_SEND_RATE:
        result = (totalSendBandwidth==0) ? 0 : unnormalized_result/totalSendBandwidth;
        break;
      default:
        assert(false);
        break;
    }

    return result;
}

} // namespace CBR
