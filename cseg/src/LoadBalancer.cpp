/*  Sirikata
 *  LoadBalancer.cpp
 *
 *  Copyright (c) 2010, Tahir Azim
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

#include "LoadBalancer.hpp"
#include "DistributedCoordinateSegmentation.hpp"

#define OVERLOAD_THRESHOLD 1000

namespace Sirikata {

LoadBalancer::LoadBalancer(DistributedCoordinateSegmentation* cseg, int nservers, const Vector3ui32& perdim) {
  for (int i=0; i<nservers;i++) {
    ServerAvailability sa;
    sa.mServer = i+1;
    (i < (int)(perdim.x*perdim.y*perdim.z)) ? (sa.mAvailable = 0) : (sa.mAvailable = 1);
    
    mAvailableServers.push_back(sa);
  }

  mCSeg = cseg;
}

LoadBalancer::~LoadBalancer() {

}

uint32 LoadBalancer::getAvailableServerIndex() {
  uint32 availableSvrIndex = INT_MAX;
  
  for (uint32 i=0; i<mAvailableServers.size(); i++) {
    if (mAvailableServers[i].mAvailable == true) {
      availableSvrIndex = i;
      break;
    }
  }
  
  return availableSvrIndex;
}

void LoadBalancer::reportRegionLoad(SegmentedRegion* segRegion, ServerID sid, uint32 loadValue) {
  if (segRegion->mLoadValue > OVERLOAD_THRESHOLD) {
    std::cout << "Adding to overloaded regions list\n"; fflush(stdout);
    boost::mutex::scoped_lock overloadedRegionsListLock(mOverloadedRegionsListMutex);
    
    bool found = false;
    for (uint i = 0; i < mOverloadedRegionsList.size(); i++) {
      if (mOverloadedRegionsList[i]->mServer == sid) {
        found = true;
      }
    }
    
    if (!found) {
      mOverloadedRegionsList.push_back(segRegion);
    }
  }
}

void LoadBalancer::handleSegmentationChange(SegmentationChangeMessage* segChangeMessage) {  
  for (int i=0; i<segChangeMessage->numEntries; i++) {
    SerializedSegmentChange* segChange = (SerializedSegmentChange*) (&(segChangeMessage->changedSegments[i]));

    for (uint32 i=0; i<mAvailableServers.size(); i++) {
      if (mAvailableServers[i].mServer == segChange->serverID &&
          segChange->listLength>=1 &&
          segChange->bboxList[0].minX != segChange->bboxList[0].maxX)
        {
          mAvailableServers[i].mAvailable = false;
          break;
        }
    }
  }
}

void LoadBalancer::service() {
  boost::mutex::scoped_lock overloadedRegionsListLock(mOverloadedRegionsListMutex);

  for (std::vector<SegmentedRegion*>::iterator it = mOverloadedRegionsList.begin();
       it != mOverloadedRegionsList.end(); 
       it++)
  {
    uint32 availableSvrIndex = getAvailableServerIndex();
    if (availableSvrIndex != INT_MAX) {

      ServerID availableServer = mAvailableServers[availableSvrIndex].mServer;

      mAvailableServers[availableSvrIndex].mAvailable = false;

      SegmentedRegion* overloadedRegion = *it;
      overloadedRegion->mLeftChild = new SegmentedRegion();
      overloadedRegion->mRightChild = new SegmentedRegion();

      BoundingBox3f region = overloadedRegion->mBoundingBox;
      float minX = region.min().x; float minY = region.min().y;
      float maxX = region.max().x; float maxY = region.max().y;
      float minZ = region.min().z; float maxZ = region.max().z;

      overloadedRegion->mLeftChild->mBoundingBox = BoundingBox3f( region.min(),
							    Vector3f( (minX+maxX)/2, maxY, maxZ) );
      overloadedRegion->mRightChild->mBoundingBox = BoundingBox3f( Vector3f( (minX+maxX)/2,minY,minZ),
                                                             region.max() );
      overloadedRegion->mLeftChild->mServer = overloadedRegion->mServer;
      overloadedRegion->mRightChild->mServer = availableServer;

      std::cout << overloadedRegion->mServer << " : " << overloadedRegion->mLeftChild->mBoundingBox << "\n";
      std::cout << availableServer << " : " << overloadedRegion->mRightChild->mBoundingBox << "\n";
      fflush(stdout);
      
      mCSeg->mWholeTreeServerRegionMap.erase(overloadedRegion->mServer);
      mCSeg->mLowerTreeServerRegionMap.erase(availableServer);

      std::vector<SegmentationInfo> segInfoVector;
      SegmentationInfo segInfo, segInfo2;
      segInfo.server = overloadedRegion->mServer;
      segInfo.region = mCSeg->serverRegion(overloadedRegion->mServer);
      segInfoVector.push_back( segInfo );

      segInfo2.server = availableServer;
      segInfo2.region = mCSeg->serverRegion(availableServer);
      segInfoVector.push_back(segInfo2);

      Thread thrd(boost::bind(&DistributedCoordinateSegmentation::notifySpaceServersOfChange,mCSeg,segInfoVector));
      
      mOverloadedRegionsList.erase(it);
      
      break;
    }
    else {
      //No idle servers are available at this time...
      break;
    }
  }
}

uint32 LoadBalancer::numAvailableServers() {
  return mAvailableServers.size();
}

}
