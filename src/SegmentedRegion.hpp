/*  cbr
 *  SegmentedRegion.cpp
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

#ifndef _CBR_SEGMENTED_REGION_HPP_
#define _CBR_SEGMENTED_REGION_HPP_

namespace CBR {

#define LOOKUP_REQUEST 1
#define LOOKUP_RESPONSE 2
#define  NUM_SERVERS_REQUEST 3
#define  NUM_SERVERS_RESPONSE 4
#define SERVER_REGION_REQUEST 5
#define SERVER_REGION_RESPONSE 6
#define REGION_REQUEST 7
#define REGION_RESPONSE 8
#define SEGMENTATION_CHANGE 9
#define SEGMENTATION_LISTEN 10

#define MAX_BBOX_LIST_SIZE 60
#define MAX_SERVER_REGIONS_CHANGED 3

typedef struct SerializedBBox{
  float minX, minY, minZ;
  float maxX, maxY, maxZ;
} SerializedBBox;

typedef struct SerializedSegmentChange{
  ServerID serverID;
  uint8 listLength;
  SerializedBBox bboxList[MAX_BBOX_LIST_SIZE/MAX_SERVER_REGIONS_CHANGED];

} SerializedSegmentChange;

typedef struct GenericMessage {
  uint8 type;
} GenericMessage;

typedef struct LookupRequestMessage {
  uint8 type;
  float x, y, z;

  LookupRequestMessage() {
    type = LOOKUP_REQUEST;
  }
} LookupRequestMessage;


typedef struct LookupResponseMessage {
  uint8 type;
  ServerID serverID;

  LookupResponseMessage() {
    type = LOOKUP_RESPONSE;
  }
} LookupResponseMessage;

typedef struct NumServersRequestMessage {
  uint8 type;  

  NumServersRequestMessage() {
    type = NUM_SERVERS_REQUEST;
  }
} NumServersRequestMessage;

typedef struct NumServersResponseMessage {
  uint8 type;
  uint32 numServers;

  NumServersResponseMessage() {
    type = NUM_SERVERS_RESPONSE;
  }
} NumServersResponseMessage;

typedef struct ServerRegionRequestMessage {
  uint8 type;
  ServerID serverID;

  ServerRegionRequestMessage() {
    type = SERVER_REGION_REQUEST;
  }
} ServerRegionRequestMessage;

typedef struct ServerRegionResponseMessage {
  uint8 type;
  uint8 listLength;
  SerializedBBox bboxList[MAX_BBOX_LIST_SIZE];

  ServerRegionResponseMessage() {
    type = SERVER_REGION_RESPONSE;
  }
} ServerRegionResponseMessage;

typedef struct RegionRequestMessage {
  uint8 type;  

  RegionRequestMessage() {
    type = REGION_REQUEST;
  }
} RegionRequestMessage;

typedef struct RegionResponseMessage {
  uint8 type;
  SerializedBBox bbox;

  RegionResponseMessage() {
    type = REGION_RESPONSE;
  }
} RegionResponseMessage;

typedef struct SegmentationChangeMessage {
  uint8_t type;
  uint8 numEntries;
  SerializedSegmentChange changedSegments[MAX_SERVER_REGIONS_CHANGED];

  SegmentationChangeMessage() {
    type = SEGMENTATION_CHANGE;
    numEntries =0;
  }
} SegmentationChangeMessage;

typedef struct SegmentationListenMessage {
  uint8_t type;
  
  SegmentationListenMessage() {
    type = SEGMENTATION_LISTEN;
  }
} SegmentationListenMessage;

typedef struct SegmentedRegion {

  SegmentedRegion() {
    mLeafCount = 0;
    mLeftChild  = mRightChild = NULL;
  }

  SegmentedRegion* getRandomLeaf() {
    if (mRightChild == NULL && mLeftChild == NULL) {
      return this;
    }
 
    int random = rand() % 2;
    if (random == 0)
      return mLeftChild->getRandomLeaf();
    else
      return mRightChild->getRandomLeaf();
  }

  int countServers() const {
    if (mRightChild == NULL && mLeftChild == NULL) return 1;

    int count = 0;

    if (mLeftChild != NULL) {
      count += mLeftChild->countServers();
    }
    if (mRightChild != NULL) {
      count += mRightChild->countServers();
    }

    return count;
  }

  SegmentedRegion* lookupSegmentedRegion(const ServerID& server_id) {
    if (mRightChild == NULL && mLeftChild == NULL && mServer==server_id) {
      return this;
    }

    SegmentedRegion* segRegion = mLeftChild->lookupSegmentedRegion(server_id);

    if (segRegion != NULL) {
      return segRegion;
    }

    segRegion = mRightChild->lookupSegmentedRegion(server_id);

    if (segRegion != NULL) {
      return segRegion;
    }

    return NULL;
  }

  ServerID lookup(const Vector3f& pos) const {
    if (mRightChild == NULL && mLeftChild == NULL && mBoundingBox.contains(pos)) {
	return mServer;
    }

    uint32_t FAKE_SVR_ID = 1000000;
    ServerID serverID = FAKE_SVR_ID;

    if (mLeftChild != NULL && mLeftChild->mBoundingBox.contains(pos)) {
      //      printf("going to left child\n");
      serverID = mLeftChild->lookup(pos);
    }

    if (mRightChild!=NULL && serverID == FAKE_SVR_ID && mRightChild->mBoundingBox.contains(pos)){
      //      printf("going to right child\n");
      serverID= mRightChild->lookup(pos); 
    }
      
    if (serverID != FAKE_SVR_ID ) {
      //        printf("%s is in server ID %d\n", pos.toString().c_str(), serverID);
    }

    return serverID;
  }


  void serverRegion(const ServerID& server, BoundingBoxList& boundingBoxList) const {
    if (mServer == server && mRightChild==NULL && mLeftChild==NULL) {
      boundingBoxList.push_back(mBoundingBox);
      return;
    }

    if (mLeftChild != NULL) {
      mLeftChild->serverRegion(server, boundingBoxList);
    }

    if (mRightChild != NULL) {
      mRightChild->serverRegion(server, boundingBoxList);
    }

    return;
  }
  
  ServerID  mServer;
  SegmentedRegion* mLeftChild;
  SegmentedRegion* mRightChild;
  int mLeafCount;
  BoundingBox3f mBoundingBox;

} SegmentedRegion;

}
#endif
