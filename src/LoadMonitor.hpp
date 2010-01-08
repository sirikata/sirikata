/*  cbr
 *  LoadMonitor.hpp
 *
 *  Copyright (c) 2009, Tahir Azim.
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

#ifndef _CBR_LOAD_MONITOR_HPP_
#define _CBR_LOAD_MONITOR_HPP_


#include "ServerMessageQueue.hpp"
#include "Message.hpp"
#include "PollingService.hpp"

#include "CBR_CSeg.pbj.hpp"

namespace CBR {

class ServerMessageQueue;
class CoordinateSegmentation;

typedef struct ServerLoadInfo{
  ServerID mServerID;
  float mLoadReading;

  float mNetworkCapacity;

  ServerLoadInfo(ServerID svrID, float load, float capacity)
    : mServerID(svrID), mLoadReading(load), mNetworkCapacity(capacity)
  {
  }

} ServerLoadInfo;



class LoadMonitor : public MessageRecipient, public PollingService {
public:
    LoadMonitor(SpaceContext* ctx, ServerMessageQueue* serverMsgQueue, CoordinateSegmentation* cseg);
    ~LoadMonitor();

  void addLoadReading();

  void sendLoadReadings();

  float getCurrentLoadReading();

  float getAveragedLoadReading();

    // From MessageRecipient
    void receiveMessage(Message* msg);

private:
    virtual void poll();

    void loadStatusMessage(const ServerID source, const CBR::Protocol::CSeg::LoadMessage& load_status_msg);

  enum {
    SEND_TO_NEIGHBORS,
    SEND_TO_ALL,
    SEND_TO_NONE,
    SEND_TO_CENTRAL,
    SEND_TO_DHT,
  };

  bool handlesAdjacentRegion(ServerID server_id);

  bool isAdjacent(BoundingBox3f& box1, BoundingBox3f& box2);

    SpaceContext* mContext;
    ServerMessageQueue* mServerMsgQueue;
    CoordinateSegmentation* mCoordinateSegmentation;

    TimeProfiler::Stage* mProfiler;

    float mCurrentLoadReading;
    float mAveragedLoadReading;

    std::map<ServerID, float> mRemoteLoadReadings;
};

}


#endif
