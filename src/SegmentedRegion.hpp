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

#define MAX_BBOX_LIST_SIZE 50000
#define MAX_SERVER_REGIONS_CHANGED 2

typedef struct SerializedVector{
  float32 x, y, z;  

  void serialize(const Vector3f& vect) {
    x = vect.x;
    y = vect.y;
    z = vect.z;
  }  

  void deserialize(Vector3f& vect) {
    vect = Vector3f(x,y,z);
  }

}__attribute__((__packed__)) SerializedVector ;

typedef struct SerializedBBox{
  float32 minX;
  float32 minY;
  float32 minZ;

  float32 maxX;
  float32 maxY;
  float32 maxZ;

  void serialize(const BoundingBox3f& bbox) {
    minX = bbox.min().x;
    minY = bbox.min().y;
    minZ = bbox.min().z;

    maxX = bbox.max().x;
    maxY = bbox.max().y;
    maxZ = bbox.max().z;    
  }  

  void deserialize(BoundingBox3f& bbox) {
    bbox = BoundingBox3f(Vector3f(minX,minY,minZ),
			 Vector3f(maxX, maxY, maxZ));
  }

}__attribute__((__packed__)) SerializedBBox ;

typedef struct SerializedSegmentChange{
  ServerID serverID;
  uint32 listLength;
  SerializedBBox bboxList[MAX_BBOX_LIST_SIZE];

}__attribute__((__packed__)) SerializedSegmentChange ;

typedef struct GenericMessage {
  uint8 type;
} GenericMessage;

typedef struct LookupRequestMessage {
  uint8 type;
  float32 x, y, z;

  LookupRequestMessage() {
    type = LOOKUP_REQUEST;
  }
}__attribute__((__packed__)) LookupRequestMessage ;


typedef struct LookupResponseMessage {
  uint8 type;
  ServerID serverID;

  LookupResponseMessage() {
    type = LOOKUP_RESPONSE;
  }
}__attribute__((__packed__)) LookupResponseMessage ;

typedef struct NumServersRequestMessage {
  uint8 type;  

  NumServersRequestMessage() {
    type = NUM_SERVERS_REQUEST;
  }
}__attribute__((__packed__)) NumServersRequestMessage;

typedef struct NumServersResponseMessage {
  uint8 type;
  uint32 numServers;

  NumServersResponseMessage() {
    type = NUM_SERVERS_RESPONSE;
  }
}__attribute__((__packed__)) NumServersResponseMessage;

typedef struct ServerRegionRequestMessage {
  uint8 type;
  ServerID serverID;

  ServerRegionRequestMessage() {
    type = SERVER_REGION_REQUEST;
  }
}__attribute__((__packed__)) ServerRegionRequestMessage;

typedef struct ServerRegionResponseMessage {
  uint8 type;
  
  uint32 listLength;
  SerializedBBox bboxList[MAX_BBOX_LIST_SIZE];

  ServerRegionResponseMessage() {
    type = SERVER_REGION_RESPONSE;
    
    listLength = 0;
  }
}__attribute__((__packed__)) ServerRegionResponseMessage;

typedef struct RegionRequestMessage {
  uint8 type;  

  RegionRequestMessage() {
    type = REGION_REQUEST;
  }
}__attribute__((__packed__)) RegionRequestMessage;

typedef struct RegionResponseMessage {
  uint8 type;
  SerializedBBox bbox;

  RegionResponseMessage() {
    type = REGION_RESPONSE;
  }
}__attribute__((__packed__)) RegionResponseMessage;

typedef struct SegmentationChangeMessage {
  uint8_t type;
  uint8 numEntries;
  SerializedSegmentChange changedSegments[MAX_SERVER_REGIONS_CHANGED];

  SegmentationChangeMessage() {
    type = SEGMENTATION_CHANGE;
    numEntries =0;
  }

  uint32 serialize(uint8** buff) {
    
    int bufSize = sizeof(uint8_t) + sizeof(uint8_t);

    for (int i=0; i<numEntries; i++) {
      bufSize += sizeof(ServerID) + sizeof(uint32);
      bufSize += changedSegments[i].listLength * sizeof(SerializedBBox);
    }    

    *buff = new uint8[bufSize];
    printf("inside_serialize, buffer=%x, bufSize=%d\n", (*buff), bufSize);

    uint8 offset = 0;

    memcpy((*buff)+offset, &type, sizeof(uint8_t));
    offset+=sizeof(uint8_t);
    
    memcpy((*buff)+offset, &numEntries, sizeof(uint8_t));
    offset+=sizeof(uint8_t);

    for (int i=0; i<numEntries; i++) {
      memcpy((*buff)+offset, &changedSegments[i].serverID, sizeof(ServerID));
      offset += sizeof(ServerID);

      memcpy((*buff)+offset, &changedSegments[i].listLength, sizeof(uint32));
      offset += sizeof(uint32);

      memcpy((*buff)+offset, &changedSegments[i].bboxList, 
	     changedSegments[i].listLength * sizeof(SerializedBBox));
      offset += changedSegments[i].listLength * sizeof(SerializedBBox);
    }

    return bufSize;
  }

}__attribute__((__packed__)) SegmentationChangeMessage;

typedef struct SegmentationListenMessage {
  uint8_t type;
  
  char host[255];
  uint16 port;
  
  SegmentationListenMessage() {
    type = SEGMENTATION_LISTEN;
  }
}__attribute__((__packed__)) SegmentationListenMessage;

typedef struct SegmentedRegion {

  SegmentedRegion() {
    mLeafCount = 0;
    mLeftChild  = mRightChild = NULL;
    mServer = 0;
  }

  void destroy() {
    if (mLeftChild != NULL) {
      mLeftChild->destroy();
 
      SegmentedRegion* prevLeftChild = mLeftChild;
      mLeftChild = NULL;
     
      delete prevLeftChild;
    }
    if (mRightChild != NULL) {
      mRightChild->destroy();

      SegmentedRegion* prevRightChild = mRightChild;
      mRightChild = NULL;
      
      delete prevRightChild;
    }
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

  SegmentedRegion* getSibling(SegmentedRegion* region) {
    assert(region->mLeftChild == NULL && region->mRightChild == NULL);    

    if (mRightChild == NULL && mLeftChild == NULL) return NULL;

    if (mRightChild == region) return mLeftChild;
    if (mLeftChild == region) return mRightChild;

    SegmentedRegion* r1 = mLeftChild->getSibling(region);

    if (r1 != NULL) return r1;

    SegmentedRegion* r2 = mRightChild->getSibling(region);

    if (r2 != NULL) return r2;

    return NULL;
  }

  SegmentedRegion* getParent(SegmentedRegion* region) {    
    if (mRightChild == NULL && mLeftChild == NULL) return NULL;

    if (mRightChild == region) return this;
    if (mLeftChild == region) return this;

    SegmentedRegion* r1 = mLeftChild->getParent(region);

    if (r1 != NULL) return r1;

    SegmentedRegion* r2 = mRightChild->getParent(region);

    if (r2 != NULL) return r2;

    return NULL;
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

  int countNodes() const {
    if (mRightChild == NULL && mLeftChild == NULL) return 1;

    int count = 0;

    if (mLeftChild != NULL) {
      count += mLeftChild->countNodes();
    }
    if (mRightChild != NULL) {
      count += mRightChild->countNodes();
    }

    return count+1;
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

const SegmentedRegion* lookup(const Vector3f& pos) const {
    if (mRightChild == NULL && mLeftChild == NULL) {
      //std::cout << "Left and right child is null: boundingbox = " << mBoundingBox<<"\n";
      if (mBoundingBox.contains(pos)) {
	return this;
      }
    }
    
    const SegmentedRegion* region = NULL;
    /*if (mLeftChild != NULL) {
      std::cout << "Left child: boundingbox = " << mLeftChild->mBoundingBox<<"\n";
    }*/

    if (mLeftChild != NULL && mLeftChild->mBoundingBox.contains(pos)) {
      //std::cout << "Contained in left child "  << mLeftChild->mBoundingBox << "\n";      
      region = mLeftChild->lookup(pos);
    }
    
    /*if (mRightChild != NULL) {
      std::cout << "Right child: boundingbox = " << mRightChild->mBoundingBox<<"\n";
    }*/

    if (mRightChild!=NULL && region == NULL && mRightChild->mBoundingBox.contains(pos)){
      //std::cout << "Contained in right child " << mRightChild->mBoundingBox << "\n";
      region= mRightChild->lookup(pos); 
    }
      
    return region;
  }


  void serverRegion(const ServerID& server, BoundingBoxList& boundingBoxList) const {
    //std::cout << mServer << " : " << mBoundingBox << " \n"; 

    if (mServer == server && mRightChild==NULL && mLeftChild==NULL) {
      //std::cout << "Adding to bblist : " << mBoundingBox << "\n";
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
  uint32 mLeafCount;
  BoundingBox3f mBoundingBox;

} SegmentedRegion;

typedef struct SerializedSegmentedRegion {
  ServerID mServerID;
  uint32 mLeftChildIdx;
  uint32 mRightChildIdx;
  SerializedBBox mBoundingBox;
  uint32 mLeafCount;

  SerializedSegmentedRegion() {
    mLeftChildIdx = 0;
    mRightChildIdx = 0;
  }
      
}__attribute__((__packed__)) SerializedSegmentedRegion;



typedef struct SerializedBSPTree {
  uint32 mNodeCount;
  SerializedSegmentedRegion* mSegmentedRegions;  //allocate dynamically based on
                                                 //size of region.
  
  SerializedBSPTree(int numNodes) {
    mNodeCount = numNodes;
    mSegmentedRegions = new SerializedSegmentedRegion[numNodes];
  }

  ~SerializedBSPTree() {
    delete mSegmentedRegions;
  }

  void deserializeBSPTree(SegmentedRegion* region, uint32 idx, 
			  SerializedBSPTree* serializedTree)
  {
    assert(idx<serializedTree->mNodeCount);

    region->mServer = serializedTree->mSegmentedRegions[idx].mServerID;
    
    region->mLeafCount = serializedTree->mSegmentedRegions[idx].mLeafCount;

    serializedTree->mSegmentedRegions[idx].mBoundingBox.deserialize(region->mBoundingBox);

    if (serializedTree->mSegmentedRegions[idx].mLeftChildIdx == 0 && 
       serializedTree->mSegmentedRegions[idx].mRightChildIdx == 0)
    {    
      //std::cout << region->mServer << " : " <<region->mLeafCount << "\n";
      //std::cout << region->mBoundingBox << "\n";
    }

    //std::cout << "left " << serializedTree->mSegmentedRegions[idx].mLeftChildIdx << "\n";
    //std::cout << "right " <<  serializedTree->mSegmentedRegions[idx].mRightChildIdx << "\n";

    if (serializedTree->mSegmentedRegions[idx].mLeftChildIdx != 0) {
      region->mLeftChild = new SegmentedRegion();
      deserializeBSPTree(region->mLeftChild, serializedTree->mSegmentedRegions[idx].mLeftChildIdx, serializedTree);
    }

    if (serializedTree->mSegmentedRegions[idx].mRightChildIdx != 0) {
      region->mRightChild = new SegmentedRegion();
      deserializeBSPTree(region->mRightChild,serializedTree->mSegmentedRegions[idx].mRightChildIdx, serializedTree);
    }
  }

private:
  SerializedBSPTree() {
    mNodeCount = 0;
  }

}__attribute__((__packed__)) SerializedBSPTree;


}
#endif
