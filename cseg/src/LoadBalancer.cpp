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
#define UNDERLOAD_THRESHOLD 50

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
  boost::mutex::scoped_lock overloadedRegionsListLock(mOverloadedRegionsListMutex);
  boost::mutex::scoped_lock underloadedRegionsListLock(mUnderloadedRegionsListMutex);

  if (segRegion->mLoadValue > OVERLOAD_THRESHOLD) {    
    std::vector<SegmentedRegion*>::iterator it = std::find(mOverloadedRegionsList.begin(),
                                                           mOverloadedRegionsList.end(), segRegion);
    if (it == mOverloadedRegionsList.end()) {
      std::cout << "Adding to overloaded: " << sid << "\n";

      mOverloadedRegionsList.push_back(segRegion);
    }


    it = std::find(mUnderloadedRegionsList.begin(),
                   mUnderloadedRegionsList.end(), segRegion);
    if (it != mUnderloadedRegionsList.end()) {
      mUnderloadedRegionsList.erase(it);
    }
  }
  else if (segRegion->mLoadValue < UNDERLOAD_THRESHOLD) {
    std::vector<SegmentedRegion*>::iterator it = std::find(mUnderloadedRegionsList.begin(),
                                                           mUnderloadedRegionsList.end(), segRegion);
    if (it == mUnderloadedRegionsList.end()) {
      mUnderloadedRegionsList.push_back(segRegion);
      std::cout << "Adding to underloaded: " << sid << "\n";
    }
  }
  else {
    std::vector<SegmentedRegion*>::iterator it = std::find(mUnderloadedRegionsList.begin(),
                                                           mUnderloadedRegionsList.end(), segRegion);
    if (it != mUnderloadedRegionsList.end()) {
      mUnderloadedRegionsList.erase(it);
      std::cout << "Removing from underloaded: " << sid << "\n";
    }
  }
}

void LoadBalancer::handleSegmentationChange(Sirikata::Protocol::CSeg::ChangeMessage segChangeMessage) {  
  for (int i=0; i < segChangeMessage.region_size(); i++) {
    Sirikata::Protocol::CSeg::SplitRegion region = segChangeMessage.region(i); 

    for (uint32 i=0; i<mAvailableServers.size(); i++) {
      if (mAvailableServers[i].mServer == region.id() &&
          region.bounds().min().x != region.bounds().max().x)
        {
          mAvailableServers[i].mAvailable = false;
          break;
        }
    }
  }
}

void LoadBalancer::service() {
  boost::mutex::scoped_lock overloadedRegionsListLock(mOverloadedRegionsListMutex);
  boost::mutex::scoped_lock underloadedRegionsListLock(mUnderloadedRegionsListMutex);

  //splitting overloaded regions
  for (std::vector<SegmentedRegion*>::iterator it = mOverloadedRegionsList.begin();
       it != mOverloadedRegionsList.end(); 
       it++)
  {
    uint32 availableSvrIndex = getAvailableServerIndex();
    if (availableSvrIndex != INT_MAX) {

      ServerID availableServer = mAvailableServers[availableSvrIndex].mServer;

      mAvailableServers[availableSvrIndex].mAvailable = false;

      SegmentedRegion* overloadedRegion = *it;
      overloadedRegion->mLeftChild = new SegmentedRegion(overloadedRegion);
      overloadedRegion->mRightChild = new SegmentedRegion(overloadedRegion);

      BoundingBox3f region = overloadedRegion->mBoundingBox;
      float minX = region.min().x; float minY = region.min().y;
      float maxX = region.max().x; float maxY = region.max().y;
      float minZ = region.min().z; float maxZ = region.max().z;

      assert(overloadedRegion->mSplitAxis != SegmentedRegion::UNDEFINED);

      if (overloadedRegion->mSplitAxis == SegmentedRegion::Y) {
        overloadedRegion->mLeftChild->mBoundingBox = BoundingBox3f( region.min(),
							    Vector3f( (minX+maxX)/2.0, maxY, maxZ) );
        overloadedRegion->mRightChild->mBoundingBox = BoundingBox3f( Vector3f( (minX+maxX)/2.0,minY,minZ),
                                                             region.max() );

        overloadedRegion->mLeftChild->mSplitAxis = overloadedRegion->mRightChild->mSplitAxis = SegmentedRegion::X;
      }
      else {
        overloadedRegion->mLeftChild->mBoundingBox = BoundingBox3f( region.min(),
                                                                    Vector3f( maxX, (minY+maxY)/2.0, maxZ) );
        overloadedRegion->mRightChild->mBoundingBox = BoundingBox3f( Vector3f( minX,(minY+maxY)/2.0,minZ),
                                                             region.max() );

        overloadedRegion->mLeftChild->mSplitAxis = overloadedRegion->mRightChild->mSplitAxis = SegmentedRegion::Y;
      }
      overloadedRegion->mLeftChild->mServer = overloadedRegion->mServer;
      overloadedRegion->mRightChild->mServer = availableServer;

      std::cout << "Split\n";
      std::cout << overloadedRegion->mServer << " : " << overloadedRegion->mLeftChild->mBoundingBox << "\n";
      std::cout << availableServer << " : " << overloadedRegion->mRightChild->mBoundingBox << "\n";
      
      mCSeg->mWholeTreeServerRegionMap.erase(overloadedRegion->mServer);
      mCSeg->mLowerTreeServerRegionMap.erase(availableServer);
      mCSeg->mWholeTreeServerRegionMap.erase(availableServer);
      mCSeg->mLowerTreeServerRegionMap.erase(overloadedRegion->mServer);


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
      
      return; //enough work for this iteration. No further splitting or merging.
    }
    else {
      //No idle servers are available at this time...
      break;
    }
  }

  //merging underloaded regions
  for (std::vector<SegmentedRegion*>::iterator it = mUnderloadedRegionsList.begin();
       it != mUnderloadedRegionsList.end();
       it++)
  {
    SegmentedRegion* underloadedRegion = *it;
    if (underloadedRegion->mParent == NULL) {
      mUnderloadedRegionsList.erase(it);
      break;
    }

    bool isRightChild = (underloadedRegion->mParent->mRightChild == underloadedRegion);
    SegmentedRegion* sibling = NULL;
    if (isRightChild) {
      sibling = underloadedRegion->mParent->mLeftChild;
    }
    else {
      sibling = underloadedRegion->mParent->mRightChild;
    }

    std::vector<SegmentedRegion*>::iterator sibling_it = 
               std::find(mUnderloadedRegionsList.begin(), mUnderloadedRegionsList.end(), sibling);
    if (sibling_it == mUnderloadedRegionsList.end()) 
    {
      mUnderloadedRegionsList.erase(it);
      break;
    }

    SegmentedRegion* parent = underloadedRegion->mParent;
    parent->mServer = parent->mLeftChild->mServer;
    for (uint32 i=0; i<mAvailableServers.size(); i++) {
      if (mAvailableServers[i].mServer == parent->mRightChild->mServer) {
        mAvailableServers[i].mAvailable = true;
        break;
      }
    }

    mCSeg->mWholeTreeServerRegionMap.erase(parent->mRightChild->mServer);
    mCSeg->mLowerTreeServerRegionMap.erase(parent->mRightChild->mServer);
    mCSeg->mWholeTreeServerRegionMap.erase(parent->mLeftChild->mServer);
    mCSeg->mLowerTreeServerRegionMap.erase(parent->mLeftChild->mServer);

    std::vector<SegmentationInfo> segInfoVector;
    SegmentationInfo segInfo, segInfo2;
    segInfo.server = parent->mRightChild->mServer;
    segInfo.region = mCSeg->serverRegion(parent->mRightChild->mServer);
    segInfoVector.push_back( segInfo );

    segInfo2.server = parent->mLeftChild->mServer;
    segInfo2.region = mCSeg->serverRegion(parent->mLeftChild->mServer);
    segInfoVector.push_back(segInfo2);

    Thread thrd(boost::bind(&DistributedCoordinateSegmentation::notifySpaceServersOfChange,mCSeg,segInfoVector));

    mUnderloadedRegionsList.erase(it);
    sibling_it = std::find(mUnderloadedRegionsList.begin(), mUnderloadedRegionsList.end(), sibling);   
    mUnderloadedRegionsList.erase(sibling_it);

    std::cout << "Merged " << parent->mLeftChild->mServer << " : " << parent->mRightChild->mServer << "!\n";

    delete parent->mLeftChild;
    delete parent->mRightChild;
    parent->mLeftChild = NULL;
    parent->mRightChild = NULL;


    break;
  }
}

uint32 LoadBalancer::numAvailableServers() {
  return mAvailableServers.size();
}

}
