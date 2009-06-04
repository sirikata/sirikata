/*  cbr
 *  LBCoordinateSegmentation.hpp
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

#ifndef _CBR_LB_COORDINATE_SEGMENTATION_HPP_
#define _CBR_LB_COORDINATE_SEGMENTATION_HPP_

#include "CoordinateSegmentation.hpp"
#include "ServerMessageQueue.hpp"

namespace CBR {

typedef struct SegmentedRegion {
  
  SegmentedRegion() {
    mChildrenCount = 0;
    
  }

  int countServers() const {
    if (mChildrenCount == 0) return 1;

    int count = 0;
    for (int i=0; i<mChildrenCount; i++) {
      count += mChildren[i].countServers();
    }

    return count;
  }

  SegmentedRegion* lookupSegmentedRegion(const ServerID& server_id) {
    if (mChildrenCount == 0 && mServer==server_id) {
      return this;
    }
        
    for (int i=0; i<mChildrenCount; i++) {
      SegmentedRegion* segRegion = mChildren[i].lookupSegmentedRegion(server_id);
      
      if (segRegion != NULL) {
	return segRegion;
      }
    }

    return NULL;
  }

  ServerID lookup(const Vector3f& pos) const {
    //TODO:efficiency needs to improve; can easily be O(log n) but is O(n) right now.

    /*printf("%d has children = %d and bounding box={%s,%s}\n", mServer,
	   mChildrenCount, mBoundingBox.min().toString().c_str(),
	   mBoundingBox.max().toString().c_str() );*/

    if (mChildrenCount == 0 && mBoundingBox.contains(pos)) {
      //printf("%d returning true \n", mServer);
	return mServer;
    }


    uint32_t FAKE_SVR_ID = 1000000;
    ServerID serverID = FAKE_SVR_ID;
    for (int i=0; i<mChildrenCount; i++) {
      serverID = mChildren[i].lookup(pos);
      
      if (serverID != FAKE_SVR_ID)
	break;
    }

    if (serverID != FAKE_SVR_ID || mServer == 0) {
      //  printf("%s is in server ID %d\n", pos.toString().c_str(), serverID);
    }

    return serverID;
  }


  void serverRegion(const ServerID& server, BoundingBoxList& boundingBoxList) const {
    /* printf("Comparing %d with this one= %d having box={%s,%s}\n", 
	   server, mServer, mBoundingBox.min().toString().c_str(),
	   mBoundingBox.max().toString().c_str());*/

    if (mServer == server) { 
      boundingBoxList.push_back(mBoundingBox);
      return;
    }

    
    for (int i=0; i<mChildrenCount; i++) {
      mChildren[i].serverRegion(server, boundingBoxList);

      /*printf("Box from %d is {%s, %s}\n", mChildren[i].mServer, 
	     mChildren[i].mBoundingBox.min().toString().c_str(),
	     mChildren[i].mBoundingBox.max().toString().c_str() );*/
    }

    return;
  }

  ServerID  mServer;
  SegmentedRegion* mChildren;
  int mChildrenCount;
  BoundingBox3f mBoundingBox;



} SegmentedRegion;


/** Uniform grid implementation of CoordinateSegmentation. */
class LBCoordinateSegmentation : public CoordinateSegmentation {
public:
  LBCoordinateSegmentation(const ServerID svrID, const BoundingBox3f& region, const Vector3ui32& perdim, ServerMessageQueue*, Trace*);
    virtual ~LBCoordinateSegmentation();

    virtual ServerID lookup(const Vector3f& pos) const;
    virtual BoundingBoxList serverRegion(const ServerID& server) const;
    virtual BoundingBox3f region() const;
    virtual uint32 numServers() const;

    virtual void tick(const Time& t);

    virtual void csegChangeMessage(CSegChangeMessage* ccMsg);

    virtual void migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo );

private:
  BoundingBox3f initRegion(const ServerID& server, const Vector3ui32& perdim) const;

  SegmentedRegion mTopLevelRegion;

  ServerID mServerID;

  ServerMessageQueue* mServerMessageQueue;

  Time mCurrentTime;

  Trace* mTrace;

}; // class CoordinateSegmentation

} // namespace CBR

#endif
