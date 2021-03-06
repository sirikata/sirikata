/*  Sirikata
 *  LoadMonitor.cpp
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


#include <sirikata/space/LoadMonitor.hpp>
#include <sirikata/space/CoordinateSegmentation.hpp>
#include <sirikata/core/options/CommonOptions.hpp>

#define THRESHOLD 5
#define ALPHA 0.3


namespace Sirikata {

bool loadInfoComparator(const ServerLoadInfo sli1, const ServerLoadInfo sli2) {
    return sli1.mLoadReading < sli2.mLoadReading;
}

LoadMonitor::LoadMonitor(SpaceContext* ctx, CoordinateSegmentation* cseg)
 : PollingService(ctx->mainStrand, "LoadMonitor Poll", Duration::seconds(5)),
   mContext(ctx),
   mCoordinateSegmentation(cseg),
   mCurrentLoadReading(0),
   mAveragedLoadReading(0)
{
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_LOAD_STATUS, this);
    mLoadServerMessageService = mContext->serverRouter()->createServerMessageService("load-monitor");
    mProfiler = mContext->profiler->addStage("Load Monitor");
}

LoadMonitor::~LoadMonitor() {
    delete mProfiler;
    delete mLoadServerMessageService;
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_LOAD_STATUS, this);
}

void LoadMonitor::addLoadReading() {
    /* FIXME load reading of server message queue is now gone, not sure what we
     * want to measure instead, or maybe augment server message queue to provide
     * different statistics.
  std::vector<QueueInfo> qInfo;
  mServerMsgQueue->getQueueInfo(qInfo);

  float currentLoad = 0;
  mCurrentLoadReading = 0;
  for (std::vector<QueueInfo>::iterator it = qInfo.begin(); it != qInfo.end(); it++) {
    QueueInfo q = *it;
    currentLoad += q.mTXUsed * q.mTXWeight;
  }

  mCurrentLoadReading = currentLoad;
    */
  mAveragedLoadReading = ALPHA * mAveragedLoadReading +
                         (1.0-ALPHA) * mCurrentLoadReading;

  printf("mAveragedLoadReading at %d=%f\n", mContext->id(), mAveragedLoadReading);


  if (mAveragedLoadReading > THRESHOLD) {
    std::vector<ServerLoadInfo> migrationHints;
    std::map<ServerID, float>::const_iterator map_iter;
    for (map_iter = mRemoteLoadReadings.begin();
	 map_iter != mRemoteLoadReadings.end();
	 map_iter++)
    {
      ServerLoadInfo sli(map_iter->first, map_iter->second, 0);
      migrationHints.push_back(sli);
    }

    sort(migrationHints.begin(), migrationHints.end(), loadInfoComparator);


    mCoordinateSegmentation->migrationHint(migrationHints);
  }

  sendLoadReadings();
}

float LoadMonitor::getAveragedLoadReading() {
  return mAveragedLoadReading;
}


void LoadMonitor::sendLoadReadings() {
  //send mCurrentLoadReading to other servers
  uint32 total_servers = mCoordinateSegmentation->numServers();

  Sirikata::Protocol::CSeg::LoadMessage load_msg;
  load_msg.set_load(mAveragedLoadReading);
  std::string serialized_load = serializePBJMessage(load_msg);

  for (uint32 i=1 ; i <= total_servers; i++) {
      if (i != mContext->id() && handlesAdjacentRegion(i) ) {
          printf("%d handles adjacent region with %d\n", i, mContext->id());

          Message* msg = new Message(
              mContext->id(),
              SERVER_PORT_LOAD_STATUS,
              (ServerID)i,
              SERVER_PORT_LOAD_STATUS,
              serialized_load
          );
          bool send_success = mLoadServerMessageService->route(msg);
          // Ignore send success, lost load readings don't matter
    }
  }
}

void LoadMonitor::receiveMessage(Message* msg) {
    Sirikata::Protocol::CSeg::LoadMessage load_msg;
    bool parsed = parsePBJMessage(&load_msg, msg->payload());

    if (parsed)
        loadStatusMessage(msg->source_server(), load_msg);
}

void LoadMonitor::loadStatusMessage(const ServerID source, const Sirikata::Protocol::CSeg::LoadMessage& load_msg){
  mRemoteLoadReadings[source] = load_msg.load();
}

void LoadMonitor::poll() {
    mProfiler->started();

    if (GetOptionValue<bool>("monitor-load"))
        addLoadReading();

    mProfiler->finished();
}

bool LoadMonitor::handlesAdjacentRegion(ServerID server_id) {
  BoundingBoxList otherBoundingBoxList = mCoordinateSegmentation->serverRegion(server_id);
  BoundingBoxList myBoundingBoxList = mCoordinateSegmentation->serverRegion(mContext->id());

  for (std::vector<BoundingBox3f>::iterator other_it=otherBoundingBoxList.begin();
       other_it != otherBoundingBoxList.end();
       other_it++)
  {
    for (std::vector<BoundingBox3f>::iterator my_it=myBoundingBoxList.begin();
         my_it != myBoundingBoxList.end();
         my_it++)
    {
      if (isAdjacent(*other_it, *my_it)) return true;
    }
  }

  return false;
}

bool LoadMonitor::isAdjacent(BoundingBox3f& box1, BoundingBox3f& box2) {

  if (box1.min().x == box2.max().x || box1.max().x == box2.min().x) {
    //if box1.miny or box1.maxy lies between box2.maxy and box2.miny, return true;
    //if box2.miny or box2.maxy lies between box1.maxy and box1.miny, return true;

    if (box2.min().y <= box1.min().y && box1.min().y <= box2.max().y) return true;
    if (box2.min().y <= box1.max().y && box1.max().y <= box2.max().y) return true;

    if (box1.min().y <= box2.min().y && box2.min().y <= box1.max().y) return true;
    if (box1.min().y <= box2.max().y && box2.max().y <= box1.max().y) return true;

  }

  if (box1.min().y == box2.max().y || box1.max().y == box2.min().y) {
    //if box1.minx or box1.maxx lies between box2.maxx and box2.minx, return true;
    //if box2.minx or box2.maxx lies between box1.maxx and box1.minx, return true;
    if (box2.min().x <= box1.min().x && box1.min().x <= box2.max().x) return true;
    if (box2.min().x <= box1.max().x && box1.max().x <= box2.max().x) return true;

    if (box1.min().x <= box2.min().x && box2.min().x <= box1.max().x) return true;
    if (box1.min().x <= box2.max().x && box2.max().x <= box1.max().x) return true;

  }

  return false;
}


}
