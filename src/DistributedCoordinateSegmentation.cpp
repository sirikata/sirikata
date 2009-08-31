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

DistributedCoordinateSegmentation::DistributedCoordinateSegmentation(const ServerID server_id, const BoundingBox3f& region, const Vector3ui32& perdim, MessageDispatcher* msg_source, MessageRouter* msg_router, Trace* trace, int nservers)
  : mServerID(server_id),
    mMessageDispatcher(msg_source),
    mMessageRouter(msg_router),
    mCurrentTime(Time::null()),
    mTrace(trace)
{
  mMessageDispatcher->registerMessageRecipient(MESSAGE_TYPE_CSEG_CHANGE, this);

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
  /* Bind the server to port 1234. */
  address.port = 1234;
  
  server = enet_host_create (& address /* the address to bind the server host to */, 
			     254      /* allow up to 254 clients and/or outgoing connections */,
			     0      /* assume any amount of incoming bandwidth */,
			     0      /* assume any amount of outgoing bandwidth */);
  if (server == NULL)
    {
      printf ("An error occurred while trying to create an ENet server host.\n");
      exit (EXIT_FAILURE);
    }
  
}

DistributedCoordinateSegmentation::~DistributedCoordinateSegmentation() {
   mMessageDispatcher->unregisterMessageRecipient(MESSAGE_TYPE_CSEG_CHANGE, this);

  //delete all the SegmentedRegion objects created with 'new'
}

ServerID DistributedCoordinateSegmentation::lookup(const Vector3f& pos) const {
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

BoundingBoxList DistributedCoordinateSegmentation::serverRegion(const ServerID& server) const
{
  BoundingBoxList boundingBoxList;
  mTopLevelRegion.serverRegion(server, boundingBoxList);

  if (boundingBoxList.size() == 0) {
    boundingBoxList.push_back(BoundingBox3f(Vector3f(0,0,0), Vector3f(0,0,0)));
  }

  return boundingBoxList;
}

BoundingBox3f DistributedCoordinateSegmentation::region() const {
  return mTopLevelRegion.mBoundingBox;
}

uint32 DistributedCoordinateSegmentation::numServers() const {
  int count = mTopLevelRegion.countServers();
  return mAvailableServers.size();
  return count;
}

void DistributedCoordinateSegmentation::tick(const Time& t) {
  ENetEvent event;

  if (mCurrentTime == Time::null()) {
    mCurrentTime = t;
  }
     
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

	     responseMessage.bbox.minX = bbox.min().x;
	     responseMessage.bbox.minY = bbox.min().y;
	     responseMessage.bbox.minZ = bbox.min().z;

	     responseMessage.bbox.maxX = bbox.max().x;
	     responseMessage.bbox.maxY = bbox.max().y;
	     responseMessage.bbox.maxZ = bbox.max().z;	     
	     
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
	       responseMessage.bboxList[i].minX = bbox.min().x;
	       responseMessage.bboxList[i].minY = bbox.min().y;
	       responseMessage.bboxList[i].minZ = bbox.min().z;
	       
	       responseMessage.bboxList[i].maxX = bbox.max().x;
	       responseMessage.bboxList[i].maxY = bbox.max().y;
	       responseMessage.bboxList[i].maxZ = bbox.max().z;	     
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

  uint16_t availableSvrIndex = 65535;
  ServerID availableServer;
  for (int i=0; i<mAvailableServers.size(); i++) {
    if (mAvailableServers[i].mAvailable == true) {
      availableSvrIndex = i;
      availableServer = mAvailableServers[i].mServer;
      break;
    }
  }
  
  if (availableSvrIndex !=65535 && t - mCurrentTime > Duration::seconds(30)) {
    mCurrentTime = t;
    std::cout << "split\n";

    SegmentedRegion* randomLeaf = mTopLevelRegion.getRandomLeaf();
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
 
      segmentChange->bboxList[j].minX = bbox.min().x;
      segmentChange->bboxList[j].minY = bbox.min().y;
      segmentChange->bboxList[j].minZ = bbox.min().z;
      
      segmentChange->bboxList[j].maxX = bbox.max().x;
      segmentChange->bboxList[j].maxY = bbox.max().y;
      segmentChange->bboxList[j].maxZ = bbox.max().z;
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

} // namespace CBR
