/*  cbr
 *  LBCoordinateSegmentation.cpp
 *
 *  Copyright (c) 2009, Tahir Azim
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

#include "LBCoordinateSegmentation.hpp"

#include <algorithm>
#include <boost/tokenizer.hpp>
#include "Options.hpp"
#include "Network.hpp"
#include "Message.hpp"


namespace CBR {

template<typename T>
T clamp(T val, T minval, T maxval) {
    if (val < minval) return minval;
    if (val > maxval) return maxval;
    return val;
}

LBCoordinateSegmentation::LBCoordinateSegmentation(const ServerID server_id, const BoundingBox3f& region, const Vector3ui32& perdim, MessageDispatcher* msg_source, MessageRouter* msg_router, Trace* trace)
  : mServerID(server_id),
    mMessageDispatcher(msg_source),
    mMessageRouter(msg_router),
    mCurrentTime(Time::null()),
    mTrace(trace)
{
    mMessageDispatcher->registerMessageRecipient(MESSAGE_TYPE_CSEG_CHANGE, this);

  mTopLevelRegion.mBoundingBox = region;

  int numServers = perdim.x * perdim.y * perdim.z;
  mTopLevelRegion.mChildrenCount = numServers;

  mTopLevelRegion.mChildren = new SegmentedRegion[numServers];

  for (int i=0; i<numServers; i++) {
    BoundingBox3f ith_region = initRegion(i+1, perdim);
    mTopLevelRegion.mChildren[i].mBoundingBox = ith_region;
    mTopLevelRegion.mChildren[i].mServer = i+1;
  }


}

LBCoordinateSegmentation::~LBCoordinateSegmentation() {
    mMessageDispatcher->unregisterMessageRecipient(MESSAGE_TYPE_CSEG_CHANGE, this);

  //delete all the SegmentedRegion objects created with 'new'
}

ServerID LBCoordinateSegmentation::lookup(const Vector3f& pos) const {
  Vector3f searchVec = pos;
  BoundingBox3f region = mTopLevelRegion.mBoundingBox;

  int i=0;
  (searchVec.z < region.min().z) ? searchVec.z = region.min().z : (i=0);
  (searchVec.x < region.min().x) ? searchVec.x = region.min().x : (i=0);
  (searchVec.y < region.min().y) ? searchVec.y = region.min().y : (i=0);

  (searchVec.z > region.max().z) ? searchVec.z = region.max().z : (i=0);
  (searchVec.x > region.max().x) ? searchVec.x = region.max().x : (i=0);
  (searchVec.y > region.max().y) ? searchVec.y = region.max().y : (i=0);


  return mTopLevelRegion.lookup(searchVec);
}

BoundingBoxList LBCoordinateSegmentation::serverRegion(const ServerID& server) const
{
  BoundingBoxList boundingBoxList;
  mTopLevelRegion.serverRegion(server, boundingBoxList);

  if (boundingBoxList.size() == 0) {
    boundingBoxList.push_back(BoundingBox3f());
  }

  return boundingBoxList;
}

BoundingBox3f LBCoordinateSegmentation::region() const {
  return mTopLevelRegion.mBoundingBox;
}

uint32 LBCoordinateSegmentation::numServers() const {
  int count = mTopLevelRegion.countServers(); return 9;

  return count;
}

void LBCoordinateSegmentation::tick(const Time& t) {
   mCurrentTime = t;
}

BoundingBox3f LBCoordinateSegmentation::initRegion(const ServerID& server, const Vector3ui32& perdim) const {
    if ( server > perdim.x * perdim.y * perdim.z ) {
      return BoundingBox3f( Vector3f(0,0,0), Vector3f(0,0,0));
    }

    BoundingBox3f mRegion = mTopLevelRegion.mBoundingBox;

    ServerID sid = server - 1;
    Vector3i32 server_dim_indices(sid%perdim.x,
                                  (sid/perdim.x)%perdim.y,
                                  (sid/perdim.x/perdim.y)%perdim.z);
    double xsize=mRegion.extents().x/perdim.x;
    double ysize=mRegion.extents().y/perdim.y;
    double zsize=mRegion.extents().z/perdim.z;
    Vector3f region_min(server_dim_indices.x*xsize+mRegion.min().x,
                        server_dim_indices.y*ysize+mRegion.min().y,
                        server_dim_indices.z*zsize+mRegion.min().z);
    Vector3f region_max((1+server_dim_indices.x)*xsize+mRegion.min().x,
                        (1+server_dim_indices.y)*ysize+mRegion.min().y,
                        (1+server_dim_indices.z)*zsize+mRegion.min().z);
    return BoundingBox3f(region_min, region_max);
}

void LBCoordinateSegmentation::receiveMessage(Message* msg) {
    CSegChangeMessage* server_split_msg = dynamic_cast<CSegChangeMessage*>(msg);
    assert(server_split_msg != NULL);

    csegChangeMessage(server_split_msg);

    delete server_split_msg;
}

void LBCoordinateSegmentation::csegChangeMessage(CSegChangeMessage* ccMsg) {

  ServerID originID = GetUniqueIDServerID(ccMsg->id());

  int numberOfRegions = ccMsg->numberOfRegions();
  SplitRegionf* regions = ccMsg->splitRegions();

  //printf("%d received server_split_msg from %d for new server %d\n", mServerID, originID,
  // ccMsg->newServerID());

  SegmentedRegion* segRegion = mTopLevelRegion.lookupSegmentedRegion(originID);

  segRegion->mChildrenCount = numberOfRegions;
  segRegion->mChildren = new SegmentedRegion[numberOfRegions];
  segRegion->mServer = -1;


  BoundingBox3f origBox = segRegion->mBoundingBox;
  for (int i=0; i<numberOfRegions; i++) {
    segRegion->mChildren[i].mServer = regions[i].mNewServerID;
    segRegion->mChildren[i].mChildrenCount = 0;

    Vector3f vmin(regions[i].minX, regions[i].minY, regions[i].minZ);
    Vector3f vmax(regions[i].maxX, regions[i].maxY, regions[i].maxZ);

    segRegion->mChildren[i].mBoundingBox = BoundingBox3f(vmin, vmax);
  }

  int total_servers = numServers();
  std::vector<Listener::SegmentationInfo> segInfoVector;
  for (int j = 1; j <= total_servers; j++) {
    Listener::SegmentationInfo segInfo;
    segInfo.server = j;
    segInfo.region = serverRegion(j);
    segInfoVector.push_back( segInfo );
  }

  notifyListeners(segInfoVector);
}

void LBCoordinateSegmentation::migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo ) {
  static bool lbed=false;

  if ( !lbed  ) {
    if (svrLoadInfo.size() == 0)
      return;

    lbed=true;

    ServerID new_server = svrLoadInfo[0].mServerID;

    SegmentedRegion* segRegion = mTopLevelRegion.lookupSegmentedRegion(mServerID);

    segRegion->mChildrenCount = 2;
    segRegion->mChildren = new SegmentedRegion[2];
    segRegion->mServer = -1;

    BoundingBox3f origBox = segRegion->mBoundingBox;
    BoundingBox3f newBox1(origBox.min(), Vector3f( (origBox.min().x+origBox.max().x)/2, origBox.max().y, origBox.max().z));
    BoundingBox3f newBox2(Vector3f( (origBox.min().x+origBox.max().x)/2, origBox.min().y, origBox.min().z), origBox.max());

    printf("newBox1= %s, %s\n", newBox1.min().toString().c_str(), newBox1.max().toString().c_str());
    printf("newBox2= %s, %s\n", newBox2.min().toString().c_str(), newBox2.max().toString().c_str());

    segRegion->mChildren[0].mBoundingBox = newBox1;
    segRegion->mChildren[1].mBoundingBox = newBox2;

    segRegion->mChildren[0].mServer = mServerID;
    segRegion->mChildren[1].mServer = new_server;


    mTrace->segmentationChanged(mCurrentTime, newBox2, new_server);
    printf("Server %d Splitting and migrating to new_server %d\n", mServerID, new_server);

    uint32 total_servers = numServers();
    std::vector<Listener::SegmentationInfo> segInfoVector;
    printf("total_servers before loop=%d\n", total_servers);
    for (uint32 j = 1; j <= total_servers; j++) {
      Listener::SegmentationInfo segInfo;
      segInfo.server = j;
      segInfo.region = serverRegion(j);
      segInfoVector.push_back( segInfo );
    }

    printf("total_servers before notifylisteners=%d\n", total_servers);

    notifyListeners(segInfoVector);

    printf("total_servers before getoption=%d\n", total_servers);
    if (GetOption(ANALYSIS_LOCVIS)->as<bool>()) {
      return;
    }

    printf("total_servers=%d\n", total_servers);
    for (uint32 i=1 ; i <= total_servers; i++) {
      if (i != mServerID) {
	printf("Sending server_split to %d\n", i);

        CSegChangeMessage* msg = new CSegChangeMessage(mServerID, 2);
	SplitRegionf* regions = msg->splitRegions();

	regions[0].mNewServerID = mServerID;
	regions[0].minX = newBox1.min().x;
	regions[0].minY = newBox1.min().y;
	regions[0].minZ = newBox1.min().z;
	regions[0].maxX = newBox1.max().x;
	regions[0].maxY = newBox1.max().y;
	regions[0].maxZ = newBox1.max().z;

	regions[1].mNewServerID = new_server;
	regions[1].minX = newBox2.min().x;
	regions[1].minY = newBox2.min().y;
	regions[1].minZ = newBox2.min().z;
	regions[1].maxX = newBox2.max().x;
	regions[1].maxY = newBox2.max().y;
	regions[1].maxZ = newBox2.max().z;

        mMessageRouter->route(msg, i);
      }
    }
  }
}



} // namespace CBR
