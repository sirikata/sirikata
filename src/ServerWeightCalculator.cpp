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

namespace CBR {

ServerWeightCalculator::ServerWeightCalculator(const ServerID& id, CoordinateSegmentation* cseg, const WeightFunction& weightFunc, ServerMessageQueue* sq)
 : mServerID(id),
   mCSeg(cseg),
   mWeightFunc(weightFunc),
   mSendQueue(sq)
{
   
    // compute initial weights
    for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
        if (sid == mServerID) continue;
        calculateWeight(mServerID, sid, NORMALIZE_BY_RECEIVE_RATE);
    }

    mCSeg->addListener(this);
}

ServerWeightCalculator::~ServerWeightCalculator() {
    mCSeg->removeListener(this);
}

void ServerWeightCalculator::updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation) {
    for(std::vector<SegmentationInfo>::const_iterator it = new_segmentation.begin(); it != new_segmentation.end(); it++) {
        if (it->server == mServerID) continue;
        calculateWeight(mServerID, it->server, NORMALIZE_BY_RECEIVE_RATE);
    }
}

void ServerWeightCalculator::calculateWeight(ServerID source, ServerID dest, Normalization norm) {
    BoundingBox3f source_bbox = mCSeg->serverRegion(source)[0];
    BoundingBox3f dest_bbox = mCSeg->serverRegion(dest)[0];
    //how much data am I able to send if I was sending everywhere
    double totalSendBandwidth=mWeightFunc(Vector3d(source_bbox.min()),Vector3d(source_bbox.max()),Vector3d(mCSeg->region().min()),Vector3d(mCSeg->region().max()))-mWeightFunc(Vector3d(source_bbox.min()),Vector3d(source_bbox.max()),Vector3d(source_bbox.min()),Vector3d(source_bbox.max()));
    //how much data is my destination able to receive
    double totalReceiveBandwidth=mWeightFunc(Vector3d(mCSeg->region().min()),Vector3d(mCSeg->region().max()),Vector3d(dest_bbox.min()),Vector3d(dest_bbox.max()))-mWeightFunc(Vector3d(dest_bbox.min()),Vector3d(dest_bbox.max()),Vector3d(dest_bbox.min()),Vector3d(dest_bbox.max()));
    //take the min of recv and send
    double totalBandwidthCap=totalSendBandwidth>totalReceiveBandwidth?totalSendBandwidth:totalReceiveBandwidth;
    //we only want to normalize by what the poor fool will receive, our fair queue will apportion our send bandwidth
    double unnormalized_result=mWeightFunc(Vector3d(source_bbox.min()),Vector3d(source_bbox.max()),Vector3d(dest_bbox.min()),Vector3d(dest_bbox.max()));
    double result = unnormalized_result;

    switch (norm) {
      case DO_NOT_NORMALIZE:
        result = unnormalized_result;
        break;
      case NORMALIZE_BY_MIN_SEND_AND_RECEIVE_RATE:
        result = unnormalized_result/totalBandwidthCap;
        break;
      case NORMALIZE_BY_RECEIVE_RATE:
        result = unnormalized_result/totalReceiveBandwidth;
        break;
      case NORMALIZE_BY_SEND_RATE:
        result = unnormalized_result/totalSendBandwidth;
        break;
      default:
        assert(false);
        break;
    }
    mSendQueue->setServerWeight(dest, result);

    //printf("src_server=%d, dest=%d, weight=%f\n", mSendQueue->getSourceServer(), dest, result );
}

} // namespace CBR
