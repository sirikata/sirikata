/*  cbr
 *  CoordinateSegmentationClient.cpp
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

#include "CoordinateSegmentationClient.hpp"

#include <algorithm>
#include <boost/tokenizer.hpp>
#include "Options.hpp"
#include "Network.hpp"
#include "Message.hpp"



namespace CBR {

typedef boost::asio::ip::tcp tcp;

template<typename T>
T clamp(T val, T minval, T maxval) {
    if (val < minval) return minval;
    if (val > maxval) return maxval;
    return val;
}



CoordinateSegmentationClient::CoordinateSegmentationClient(SpaceContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim)
  : CoordinateSegmentation(ctx), mAvailableServersCount(0), mRegion(region), mBSPTreeValid(true)
{
  mContext->dispatcher->registerMessageRecipient(MESSAGE_TYPE_CSEG_CHANGE, this);

  mTopLevelRegion.mBoundingBox = region;

  client = enet_host_create (NULL , 1, 0, 0);

  if (client == NULL)
  {
      printf ("An error occurred while trying to create an ENet client host.\n");
  }

  ENetAddress address;
  ENetEvent event;

  // Connect to CSEG server
  enet_address_set_host (&address, GetOption("cseg-service-host")->as<String>().c_str() );
  address.port = atoi( GetOption("cseg-service-enet-port")->as<String>().c_str() );

  // Initiate the connection, allocating one channel.
  peer = enet_host_connect (client, &address, 1);

  if (peer == NULL)
    {
      printf ("No available peers for initiating an ENet connection.\n");
    }

  /* Wait up to 200 milliseconds for the connection attempt to succeed. */
  if (enet_host_service (client, & event, 200) > 0 &&
      event.type == ENET_EVENT_TYPE_CONNECT)
    {
      printf("Connection to CSEG succeeded.\n");
    }
    else
    {
        enet_peer_reset (peer);
        printf ("Connection to CSEG failed.\n");
    }


  mSubscriptionClient = enet_host_create (NULL , 1, 0, 0);

  if (mSubscriptionClient == NULL)
  {
      printf ("An error occurred while trying to create the subscription client host.\n");
  }

  // Initiate the connection, allocating one channel.
  ENetPeer * subscriptionPeer = enet_host_connect (mSubscriptionClient, &address, 1);

  if (subscriptionPeer == NULL)
  {
      printf ("No available peers for initiating a subscription connection.\n");
  }

  /* Wait up to 200 milliseconds for the connection attempt to succeed. */
  if (enet_host_service (mSubscriptionClient, & event, 200) > 0 &&
      event.type == ENET_EVENT_TYPE_CONNECT)
    {
      printf("Connection to CSEG for subscription succeeded.\n");
    }
    else
    {
        enet_peer_reset (subscriptionPeer);
        printf ("Connection to CSEG for subscription failed.\n");
    }

  SegmentationListenMessage slMessage;
  ENetPacket * packet = enet_packet_create (&slMessage,
					    sizeof(slMessage),
                                            ENET_PACKET_FLAG_RELIABLE);

  enet_peer_send(subscriptionPeer, 0, packet);

  downloadUpdatedBSPTree();
}
  
CoordinateSegmentationClient::~CoordinateSegmentationClient() {
   mContext->dispatcher->unregisterMessageRecipient(MESSAGE_TYPE_CSEG_CHANGE, this);

   enet_peer_disconnect (peer, 0);

  //delete all the SegmentedRegion objects created with 'new'
   enet_host_destroy(client);

}

ServerID CoordinateSegmentationClient::lookup(const Vector3f& pos)  {

  if ( mBSPTreeValid) {
    Vector3f searchVec = pos;
    BoundingBox3f region = mTopLevelRegion.mBoundingBox;
    
    int i=0;
    (searchVec.z < region.min().z) ? searchVec.z = region.min().z : (i=0);
    (searchVec.x < region.min().x) ? searchVec.x = region.min().x : (i=0);
    (searchVec.y < region.min().y) ? searchVec.y = region.min().y : (i=0);
    
    (searchVec.z > region.max().z) ? searchVec.z = region.max().z : (i=0);
    (searchVec.x > region.max().x) ? searchVec.x = region.max().x : (i=0);
    (searchVec.y > region.max().y) ? searchVec.y = region.max().y : (i=0);
    
    ServerID svrID =  mTopLevelRegion.lookup(searchVec);
    assert(svrID != 1000000);

    return svrID;
  }

  downloadUpdatedBSPTree();

  LookupRequestMessage lookupMessage;
  lookupMessage.x = pos.x;
  lookupMessage.y = pos.y;
  lookupMessage.z = pos.z;

  ENetPacket * packet = enet_packet_create (&lookupMessage,
					    sizeof(lookupMessage),
                                            ENET_PACKET_FLAG_RELIABLE);

  enet_peer_send(peer, 0, packet);

  ENetEvent event;
  if (enet_host_service (client, & event, 200) > 0) {
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
      LookupResponseMessage* response = (LookupResponseMessage*) event.packet->data;
      //printf("lookupresponsetype=%d\n",  response->type);

      ServerID svrID = response->serverID;

      // Clean up the packet now that we're done using it.
      enet_packet_destroy (event.packet);

      return svrID;
    }
  }

  return NULL;
}

BoundingBoxList CoordinateSegmentationClient::serverRegion(const ServerID& server)
{
  BoundingBoxList boundingBoxList;

  if (mServerRegionCache.find(server) != mServerRegionCache.end()) {
    return mServerRegionCache[server];
  }

  ServerRegionRequestMessage requestMessage;
  requestMessage.serverID = server;
  ENetPacket * packet = enet_packet_create (&requestMessage,
					    sizeof(requestMessage),
                                            ENET_PACKET_FLAG_RELIABLE);

  enet_peer_send(peer, 0, packet);

  ENetEvent event;
  if (enet_host_service (client, & event, 200) > 0) {
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
      ServerRegionResponseMessage* response = (ServerRegionResponseMessage*) event.packet->data;
      //printf("serverregionresponsetype=%d\n",  response->type);

      for (int i = 0; i < response->listLength; i++) {
	SerializedBBox region = response->bboxList[i];
	Vector3f vmin (region.minX,region.minY,region.minZ);
	Vector3f vmax (region.maxX,region.maxY,region.maxZ);

	//std::cout << "Returned bbox for server " << server << " : " << BoundingBox3f(vmin, vmax)<<std::endl;

	boundingBoxList.push_back( BoundingBox3f(vmin, vmax) );
      }

      // Clean up the packet now that we're done using it.
      enet_packet_destroy (event.packet);
    }
  }
  
  mServerRegionCache[server] = boundingBoxList;

  return boundingBoxList;
}

BoundingBox3f CoordinateSegmentationClient::region()  {
  if (mBSPTreeValid) {
    return mTopLevelRegion.mBoundingBox;
  }

  downloadUpdatedBSPTree();
  

  RegionRequestMessage requestMessage;

  ENetPacket * packet = enet_packet_create (&requestMessage,
					    sizeof(requestMessage),
                                            ENET_PACKET_FLAG_RELIABLE);

  enet_peer_send(peer, 0, packet);

  ENetEvent event;
  if (enet_host_service (client, & event, 200) > 0) {
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
      RegionResponseMessage* response = (RegionResponseMessage*) event.packet->data;
      //      printf("regionresponsetype=%d\n",  response->type);

      SerializedBBox region = response->bbox;
      Vector3f vmin (region.minX,region.minY,region.minZ);
      Vector3f vmax (region.maxX,region.maxY,region.maxZ);

      BoundingBox3f bbox(vmin, vmax);

      // Clean up the packet now that we're done using it.
      enet_packet_destroy (event.packet);

      return bbox;
    }
  }

  return BoundingBox3f();
}

uint32 CoordinateSegmentationClient::numServers()  { 
  if (mAvailableServersCount > 0) return mAvailableServersCount;

  NumServersRequestMessage requestMessage;

  ENetPacket * packet = enet_packet_create (&requestMessage,
					    sizeof(requestMessage),
                                            ENET_PACKET_FLAG_RELIABLE);

  enet_peer_send(peer, 0, packet);

  ENetEvent event;
  if (enet_host_service (client, & event, 200) > 0) {
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
      NumServersResponseMessage* response = (NumServersResponseMessage*) event.packet->data;

      int numServers = response->numServers;
      //      printf("numServers=%d, type=%d\n", numServers, response->type);

      // Clean up the packet now that we're done using it.
      enet_packet_destroy (event.packet);

      mAvailableServersCount = numServers;
      return numServers;
    }
  }

  return 0;
}

void CoordinateSegmentationClient::service() {
  ENetEvent event;
  ENetPacket* packet;

  if ( enet_host_service (mSubscriptionClient, & event,0) > 0)
  {
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
      SegmentationChangeMessage* segChangeMessage = (SegmentationChangeMessage*)
	                                                      event.packet->data;

      std::vector<Listener::SegmentationInfo> segInfoVector;
      for (int i=0; i<segChangeMessage->numEntries; i++) {
	Listener::SegmentationInfo segInfo;
	SerializedSegmentChange segChange = segChangeMessage->changedSegments[i];
	BoundingBoxList bbList;

	for (unsigned int j=0; j < segChange.listLength; j++) {
	  Vector3f vmin(segChange.bboxList[j].minX,
			segChange.bboxList[j].minY,
			segChange.bboxList[j].minZ);
	  Vector3f vmax(segChange.bboxList[j].maxX,
			segChange.bboxList[j].maxY,
			segChange.bboxList[j].maxZ);

	  std::cout << segChange.serverID << " : " << vmin << " " << vmax << "\n";
	  BoundingBox3f bbox (vmin, vmax);
	  bbList.push_back(bbox);
	}

	segInfo.server = segChange.serverID;
	segInfo.region = bbList;

	segInfoVector.push_back(segInfo);
      }

      mAvailableServersCount = 0;
      mBSPTreeValid = false;
      mTopLevelRegion.destroy();
      mServerRegionCache.clear();
      notifyListeners(segInfoVector);
    }
  }
}

void CoordinateSegmentationClient::receiveMessage(Message* msg) {
}

void CoordinateSegmentationClient::csegChangeMessage(CSegChangeMessage* ccMsg) {

}

void CoordinateSegmentationClient::migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo ) {
}

void CoordinateSegmentationClient::downloadUpdatedBSPTree() {
  boost::asio::io_service io_service;
    
  tcp::resolver resolver(io_service);
 
  tcp::resolver::query query(tcp::v4(),GetOption("cseg-service-host")->as<String>(),
			               GetOption("cseg-service-tcp-port")->as<String>());

  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
 
  tcp::resolver::iterator end;


  tcp::socket socket(io_service);
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
  if (error)
    throw boost::system::system_error(error);

 
  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  for (;;)
    {
      boost::array<uint8, 1048576> buf;
      boost::system::error_code error;
      
      size_t len = socket.read_some(boost::asio::buffer(buf), error);      
      
      if (dataReceived == NULL) {
	dataReceived = (uint8*) malloc (len);	
      }
      else if (len > 0){
	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);
      
      bytesReceived += len;
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.
    }

  uint32 nodeCount;
  memcpy(&nodeCount, dataReceived, sizeof(uint32));
  SerializedBSPTree serializedBSPTree(nodeCount);  
  printf("nodecount=%d\n" ,serializedBSPTree.mNodeCount );
  memcpy(serializedBSPTree.mSegmentedRegions, dataReceived + 4, bytesReceived-4);
  serializedBSPTree.deserializeBSPTree(&mTopLevelRegion, 0, &serializedBSPTree);
    
  mBSPTreeValid = true;
  if (dataReceived != NULL) {
    free(dataReceived);
  }
}


} // namespace CBR
