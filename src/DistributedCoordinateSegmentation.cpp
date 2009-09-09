/*  cbr
 *  DistributedCoordinateSegmentation.cpp
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

#include "DistributedCoordinateSegmentation.hpp"

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

DistributedCoordinateSegmentation::DistributedCoordinateSegmentation(SpaceContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim, int nservers)
 : CoordinateSegmentation(ctx),
   mLastUpdateTime(mContext->time)
{
  mContext->dispatcher->registerMessageRecipient(MESSAGE_TYPE_CSEG_CHANGE, this);

  mTopLevelRegion.mBoundingBox = region;

  mTopLevelRegion.mLeftChild = new SegmentedRegion();
  mTopLevelRegion.mRightChild = new SegmentedRegion();

  float minX = region.min().x; float minY = region.min().y;
  float maxX = region.max().x; float maxY = region.max().y;
  float minZ = region.min().z; float maxZ = region.max().z;

  mTopLevelRegion.mLeftChild->mBoundingBox = BoundingBox3f( region.min(),
                                                  Vector3f( maxX, (minY+maxY)/2, maxZ) );
  mTopLevelRegion.mRightChild->mBoundingBox = BoundingBox3f( Vector3f(minX,(minY+maxY)/2,minZ),
                                                             region.max() );

  mTopLevelRegion.mLeftChild->mLeftChild = new SegmentedRegion();
  mTopLevelRegion.mRightChild->mRightChild = new SegmentedRegion();
  mTopLevelRegion.mLeftChild->mRightChild = new SegmentedRegion();
  mTopLevelRegion.mRightChild->mLeftChild = new SegmentedRegion();

  mTopLevelRegion.mLeftChild->mLeftChild->mServer = 1;
  mTopLevelRegion.mLeftChild->mRightChild->mServer = 2;
  mTopLevelRegion.mRightChild->mLeftChild->mServer = 3;
  mTopLevelRegion.mRightChild->mRightChild->mServer = 4;

  mTopLevelRegion.mLeftChild->mLeftChild->mBoundingBox = BoundingBox3f( Vector3f(minX, minY, minZ), Vector3f( (minX+maxX)/2, (minY+maxY)/2, maxZ) );
  mTopLevelRegion.mLeftChild->mRightChild->mBoundingBox = BoundingBox3f( Vector3f((minX+maxX)/2, minY, minZ), Vector3f(maxX, (minY+maxY)/2, maxZ));

  mTopLevelRegion.mRightChild->mLeftChild->mBoundingBox = BoundingBox3f( Vector3f(minX, (minY+maxY)/2, minZ), Vector3f( (minX+maxX)/2, maxY, maxZ));
  mTopLevelRegion.mRightChild->mRightChild->mBoundingBox = BoundingBox3f( Vector3f((minX+maxX)/2, (minY+maxY)/2, minZ), Vector3f(maxX, maxY, maxZ));

  for (int i=0; i<nservers;i++) {
    ServerAvailability sa;
    sa.mServer = i+1;
    (i<4) ? (sa.mAvailable = 0) : (sa.mAvailable = 1);

    mAvailableServers.push_back(sa);
  }

  printf("%d servers\n", nservers);

  ENetAddress address;

  /* Bind the server to the default localhost.     */
  /* A specific host address can be specified by   */
  /* enet_address_set_host (& address, "x.x.x.x"); */

  address.host = ENET_HOST_ANY;
  address.port = atoi( GetOption("cseg-service-enet-port")->as<String>().c_str() );

  server = enet_host_create (& address /* the address to bind the server host to */,
			     254      /* allow up to 254 clients and/or outgoing connections */,
			     0      /* assume any amount of incoming bandwidth */,
			     0      /* assume any amount of outgoing bandwidth */);
  if (server == NULL)
    {
      printf ("An error occurred while trying to create an ENet server host.\n");
      exit (EXIT_FAILURE);
    }  
  
  mAcceptor = boost::shared_ptr<tcp::acceptor>(new tcp::acceptor(mIOService,tcp::endpoint(tcp::v4(), atoi( GetOption("cseg-service-tcp-port")->as<String>().c_str() ))));

  startAccepting();
}

DistributedCoordinateSegmentation::~DistributedCoordinateSegmentation() {
    mContext->dispatcher->unregisterMessageRecipient(MESSAGE_TYPE_CSEG_CHANGE, this);

    //delete all the SegmentedRegion objects created with 'new'
    mTopLevelRegion.destroy();
}

ServerID DistributedCoordinateSegmentation::lookup(const Vector3f& pos)  {
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

BoundingBoxList DistributedCoordinateSegmentation::serverRegion(const ServerID& server) 
{
  BoundingBoxList boundingBoxList;
  mTopLevelRegion.serverRegion(server, boundingBoxList);

  if (boundingBoxList.size() == 0) {
    boundingBoxList.push_back(BoundingBox3f(Vector3f(0,0,0), Vector3f(0,0,0)));
  }

  return boundingBoxList;
}

BoundingBox3f DistributedCoordinateSegmentation::region()  {
  return mTopLevelRegion.mBoundingBox;
}

uint32 DistributedCoordinateSegmentation::numServers() {
  int count = mTopLevelRegion.countServers();
  return mAvailableServers.size();
  //return count;
}

void DistributedCoordinateSegmentation::service() {
  Time t = mContext->time;

  mIOService.poll_one();

  ENetEvent event;

  /* Wait up to 0 milliseconds for an event. */
  if (enet_host_service (server, & event, 0) > 0)
  {
     switch (event.type)
       {
       case ENET_EVENT_TYPE_NONE:
         break;
       case ENET_EVENT_TYPE_CONNECT:
	 {
	   printf ("A new client connected from %x:%u.\n",
		 event.peer -> address.host,
		 event.peer -> address.port);
	   break;
	 }
       case ENET_EVENT_TYPE_RECEIVE:
	 {
	   GenericMessage* genericMessage = (GenericMessage*) event.packet->data;

	   if (genericMessage->type == LOOKUP_REQUEST) {

	     LookupRequestMessage* lookupMessage = (LookupRequestMessage*) event.packet->data;

	     LookupResponseMessage lookupResponseMessage;
	     lookupResponseMessage.serverID =
	       lookup(Vector3f(lookupMessage->x, lookupMessage->y, lookupMessage->z));

	     ENetPacket * packet = enet_packet_create (&lookupResponseMessage,
						       sizeof(lookupResponseMessage),
						       ENET_PACKET_FLAG_RELIABLE);
	     enet_peer_send(event.peer, 0, packet);
	   }
	   else if (genericMessage->type == NUM_SERVERS_REQUEST) {
	     NumServersRequestMessage* message = (NumServersRequestMessage*) event.packet->data;

	     NumServersResponseMessage responseMessage;
	     responseMessage.numServers = numServers();

	     ENetPacket * packet = enet_packet_create (&responseMessage,
						       sizeof(responseMessage),
						       ENET_PACKET_FLAG_RELIABLE);

	     //printf("numServers returning=%d\n", responseMessage.numServers);
	     enet_peer_send(event.peer, 0, packet);
	   }
	   else if (genericMessage->type == REGION_REQUEST) {
	     RegionRequestMessage* message = (RegionRequestMessage*) event.packet->data;

	     RegionResponseMessage responseMessage;
	     BoundingBox3f bbox = region();

	     responseMessage.bbox.serialize(bbox);

	     ENetPacket * packet = enet_packet_create (&responseMessage,
						       sizeof(responseMessage),
						       ENET_PACKET_FLAG_RELIABLE);
	     enet_peer_send(event.peer, 0, packet);
	   }
	   else if (genericMessage->type == SERVER_REGION_REQUEST) {
	     ServerRegionRequestMessage* message = (ServerRegionRequestMessage*) event.packet->data;

	     ServerRegionResponseMessage responseMessage;
	     BoundingBoxList bboxList = serverRegion(message->serverID);

	     int i=0;
	     for (i=0; i<bboxList.size(); i++) {
	       BoundingBox3f bbox = bboxList[i];
	       responseMessage.bboxList[i].serialize(bbox);
	     }

	     responseMessage.listLength = bboxList.size();

	     ENetPacket * packet = enet_packet_create (&responseMessage,
						       sizeof(responseMessage),
						       ENET_PACKET_FLAG_RELIABLE);
	     enet_peer_send(event.peer, 0, packet);
	   }
	   else if (genericMessage->type == SEGMENTATION_LISTEN){
	     mSpacePeers.push_back(event.peer);
	   }

	   /* Clean up the packet now that we're done using it. */
	   enet_packet_destroy (event.packet);
	 }
	 break;

       case ENET_EVENT_TYPE_DISCONNECT:
	 {
	   for (std::vector<ENetPeer*>::iterator it=mSpacePeers.begin(); it!=mSpacePeers.end(); it++){
	     if ( (*it) == event.peer) {
	       mSpacePeers.erase(it);
	       break;
	     }
	   }
	   /* Reset the peer's client information. */
	   event.peer -> data = NULL;
	 }
       }

     return;
  }

  if ( !GetOption("random-splits-merges")->as<bool>() ) {
    return;
  }

  uint16_t availableSvrIndex = 65535;
  ServerID availableServer;
  for (int i=0; i<mAvailableServers.size(); i++) {
    if (mAvailableServers[i].mAvailable == true) {
      availableSvrIndex = i;
      availableServer = mAvailableServers[i].mServer;
      break;
    }
  }
  

  /* Do splits and merges every 15 seconds, except the first time, when its done
     30 seconds after startup.
   */
  static int duration = 30;
  if (availableSvrIndex !=65535 && t - mLastUpdateTime > Duration::seconds(duration)) {
    mLastUpdateTime = t;
    duration = 15;    

    SegmentedRegion* randomLeaf = mTopLevelRegion.getRandomLeaf();

    SegmentedRegion* sibling;
    bool okToMerge = false;
    if ( (sibling=mTopLevelRegion.getSibling(randomLeaf)) != NULL) {
      std::cout << "Siblings: " << sibling->mServer << " and " << randomLeaf->mServer << "\n";
      if (sibling->mLeftChild == NULL && sibling->mRightChild == NULL) {
        okToMerge = true;
      }
    }

    if (okToMerge && rand() % 2 == 0) {
      std::cout << "merge\n";
      SegmentedRegion* parent = mTopLevelRegion.getParent(randomLeaf);

      if (parent == NULL) {
	std::cout << "Parent of " << randomLeaf->mServer <<" not found.\n";
	return;
      }

      SegmentedRegion* leftChild = parent->mLeftChild;
      SegmentedRegion* rightChild = parent->mRightChild;

      parent->mServer = leftChild->mServer;
      parent->mRightChild = parent->mLeftChild = NULL;

      for (int i=0; i<mAvailableServers.size(); i++) {
	if (mAvailableServers[i].mServer == rightChild->mServer) {
	  mAvailableServers[i].mAvailable = true;
	  break;
	}
      }

      std::cout << "Merged! " << leftChild->mServer << " : " << rightChild->mServer << "\n";
      
      std::vector<Listener::SegmentationInfo> segInfoVector;
      Listener::SegmentationInfo segInfo, segInfo2;
      segInfo.server = parent->mServer;
      segInfo.region = serverRegion(parent->mServer);
      segInfoVector.push_back( segInfo );

      segInfo2.server = rightChild->mServer;
      segInfo2.region = serverRegion(rightChild->mServer);
      segInfoVector.push_back(segInfo2);

      notifySpaceServersOfChange(segInfoVector);

      delete rightChild;
      delete leftChild;
    }
    else {
      std::cout << "split\n";
      randomLeaf->mLeftChild = new SegmentedRegion();
      randomLeaf->mRightChild = new SegmentedRegion();

      BoundingBox3f region = randomLeaf->mBoundingBox;
      float minX = region.min().x; float minY = region.min().y;
      float maxX = region.max().x; float maxY = region.max().y;
      float minZ = region.min().z; float maxZ = region.max().z;

      randomLeaf->mLeftChild->mBoundingBox = BoundingBox3f( region.min(),
							    Vector3f( maxX, (minY+maxY)/2, maxZ) );
      randomLeaf->mRightChild->mBoundingBox = BoundingBox3f( Vector3f(minX,(minY+maxY)/2,minZ),
                                                             region.max() );
      randomLeaf->mLeftChild->mServer = randomLeaf->mServer;
      randomLeaf->mRightChild->mServer = availableServer;

      std::cout << randomLeaf->mServer << " : " << randomLeaf->mLeftChild->mBoundingBox << "\n";
      std::cout << availableServer << " : " << randomLeaf->mRightChild->mBoundingBox << "\n";

      mAvailableServers[availableSvrIndex].mAvailable = false;    

      std::vector<Listener::SegmentationInfo> segInfoVector;
      Listener::SegmentationInfo segInfo, segInfo2;
      segInfo.server = randomLeaf->mServer;
      segInfo.region = serverRegion(randomLeaf->mServer);
      segInfoVector.push_back( segInfo );

      segInfo2.server = availableServer;
      segInfo2.region = serverRegion(availableServer);
      segInfoVector.push_back(segInfo2);

      notifySpaceServersOfChange(segInfoVector);
    }
  }
}

void DistributedCoordinateSegmentation::receiveMessage(Message* msg) {
}

void DistributedCoordinateSegmentation::notifySpaceServersOfChange(std::vector<Listener::SegmentationInfo>& segInfoVector)
{
  SegmentationChangeMessage segChangeMsg;

  segChangeMsg.numEntries = (MAX_SERVER_REGIONS_CHANGED < segInfoVector.size())
                           ? MAX_SERVER_REGIONS_CHANGED : segInfoVector.size();

  for (unsigned int i=0 ;i<MAX_SERVER_REGIONS_CHANGED && i<segInfoVector.size(); i++) {

    SerializedSegmentChange* segmentChange = &segChangeMsg.changedSegments[i];
    segmentChange->serverID = segInfoVector[i].server;
    segmentChange->listLength = segInfoVector[i].region.size();

    for (unsigned int j=0; j<segInfoVector[i].region.size(); j++) {
      BoundingBox3f bbox = segInfoVector[i].region[j];

      segmentChange->bboxList[j].serialize(bbox);
    }
  }

  ENetPacket * packet = enet_packet_create (&segChangeMsg,
					    sizeof(segChangeMsg),
					    ENET_PACKET_FLAG_RELIABLE);

  for (std::vector<ENetPeer*>::const_iterator it=mSpacePeers.begin(); it!=mSpacePeers.end(); it++){
    enet_peer_send(*it, 0, packet);
  }
}

void DistributedCoordinateSegmentation::csegChangeMessage(CSegChangeMessage* ccMsg) {
}

void DistributedCoordinateSegmentation::migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo ) {
}

void DistributedCoordinateSegmentation::serializeBSPTree(SerializedBSPTree* serializedBSPTree) {
  uint32 idx = 0;
  traverseAndStoreTree(&mTopLevelRegion, idx, serializedBSPTree);
}

void DistributedCoordinateSegmentation::traverseAndStoreTree(SegmentedRegion* region, uint32& idx, 
							     SerializedBSPTree* serializedTree)
{
  uint32 localIdx = idx;
  serializedTree->mSegmentedRegions[localIdx].mServerID = region->mServer;
  serializedTree->mSegmentedRegions[localIdx].mLeafCount = region->mLeafCount;
  serializedTree->mSegmentedRegions[localIdx].mBoundingBox.serialize(region->mBoundingBox);

  // std::cout << "at index " << localIdx  <<" bbox=" << region->mBoundingBox << "\n";
  
  if (region->mLeftChild != NULL) {
    serializedTree->mSegmentedRegions[localIdx].mLeftChildIdx = idx+1;
    traverseAndStoreTree(region->mLeftChild, ++idx, serializedTree);
  }
  
  if (region->mRightChild != NULL) {
    serializedTree->mSegmentedRegions[localIdx].mRightChildIdx = idx+1;
    traverseAndStoreTree(region->mRightChild, ++idx, serializedTree);
  }
}

void DistributedCoordinateSegmentation::accept_handler()
{    
  int numNodes = mTopLevelRegion.countNodes();
  SerializedBSPTree* serializedBSPTree = new SerializedBSPTree(numNodes);
  int dataSize = 4 + numNodes * sizeof(SerializedSegmentedRegion);
  
  printf("sending %d bytes for nodes=%d\n", dataSize, numNodes);
  serializeBSPTree(serializedBSPTree);

  uint8* buffer = new uint8[dataSize];
  memcpy(buffer, serializedBSPTree, 4);
  memcpy(buffer+sizeof(uint32), serializedBSPTree->mSegmentedRegions, 
	 numNodes*sizeof(SerializedSegmentedRegion));

  boost::asio::write(*mSocket,
		     boost::asio::buffer((const void*)buffer,dataSize),
		     boost::asio::transfer_all() );

  delete buffer;
  delete serializedBSPTree;

  mSocket->close();  

  startAccepting();
}

void DistributedCoordinateSegmentation::startAccepting() {
  mSocket = boost::shared_ptr<tcp::socket>(new tcp::socket(mIOService));
  mAcceptor->async_accept(*mSocket, boost::bind(&DistributedCoordinateSegmentation::accept_handler,this));
}

} // namespace CBR
