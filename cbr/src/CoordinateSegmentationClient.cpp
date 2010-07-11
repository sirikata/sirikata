/*  Sirikata
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

#include "CoordinateSegmentationClient.hpp"
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>

#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <sirikata/core/options/CommonOptions.hpp>
#include "ServerMessage.hpp"
#include <sirikata/core/network/ServerIDMap.hpp>

namespace Sirikata {

template<typename T>
T clamp(T val, T minval, T maxval) {
    if (val < minval) return minval;
    if (val > maxval) return maxval;
    return val;
}

void memdump1(uint8* buffer, int len) {
  for (int i=0; i<len; i++) {
    int val = buffer[i];
    printf("%x ",  val);
  }
  printf("\n");
  fflush(stdout);
}

using Sirikata::Network::TCPResolver;
using Sirikata::Network::TCPSocket;
using Sirikata::Network::TCPListener;

CoordinateSegmentationClient::CoordinateSegmentationClient(SpaceContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim, ServerIDMap* sidmap)
  : CoordinateSegmentation(ctx),  mRegion(region), mTopLevelRegion(NULL),  mBSPTreeValid(false),
    mAvailableServersCount(0), mIOService(Network::IOServiceFactory::makeIOService()),
    mSidMap(sidmap), mLeaseExpiryTime(Timer::now() + Duration::milliseconds(60000.0))
{
  mTopLevelRegion.mBoundingBox = BoundingBox3f( Vector3f(0,0,0), Vector3f(0,0,0));

  Address4* addy = mSidMap->lookupInternal(mContext->id());

  mAcceptor = boost::shared_ptr<TCPListener>(new TCPListener(*mIOService,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), addy->port+10000)));

  startAccepting();

  sendSegmentationListenMessage();
}

void CoordinateSegmentationClient::startAccepting() {
  mSocket = boost::shared_ptr<TCPSocket>(new TCPSocket(*mIOService));
  mAcceptor->async_accept(*mSocket, boost::bind(&CoordinateSegmentationClient::accept_handler,this));
}

void CoordinateSegmentationClient::accept_handler() {
  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  for (;;)
    {
      boost::array<uint8, 1024> buf;
      boost::system::error_code error;

      size_t len = mSocket->read_some(boost::asio::buffer(buf), error);

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
      else if (error) {
	std::cout << "Error reading request from " << mSocket->remote_endpoint().address().to_string()
		    <<" in accept_handler\n";
        throw boost::system::system_error(error); // Some other error.
      }
    }

  if ( dataReceived != NULL) {
    SegmentationChangeMessage* segChangeMessage = (SegmentationChangeMessage*) dataReceived;

    std::cout << "\n\nReceived segmentation change message with numEntries=" << ((int)segChangeMessage->numEntries) << " at time " << mContext->simTime().raw() << "\n";

    mTopLevelRegion.destroy();
    mServerRegionCache.clear();

    int offset = 2;
    std::vector<SegmentationInfo> segInfoVector;
    for (int i=0; i<segChangeMessage->numEntries; i++) {
      BoundingBoxList bbList;
      SegmentationInfo segInfo;
      SerializedSegmentChange* segChange = (SerializedSegmentChange*) (dataReceived+offset);

      for (unsigned int j=0; j < segChange->listLength; j++) {
      	BoundingBox3f bbox;
      	SerializedBBox sbbox= segChange->bboxList[j];
      	sbbox.deserialize(bbox);
      	bbList.push_back(bbox);
      }

      segInfo.server = segChange->serverID;
      segInfo.region = bbList;

      printf("segInfo.server=%d, segInfo.region.size=%d\n", segInfo.server, (int)segInfo.region.size());
      fflush(stdout);

      segInfoVector.push_back(segInfo);
      offset += (4 + 4 + sizeof(SerializedBBox)*segChange->listLength);
    }

    notifyListeners(segInfoVector);

    free(dataReceived);
  }

  mSocket->close();
  startAccepting();
}

CoordinateSegmentationClient::~CoordinateSegmentationClient() {

}

void CoordinateSegmentationClient::sendSegmentationListenMessage() {
  SegmentationListenMessage requestMessage;
  Address4* addy = mSidMap->lookupInternal(mContext->id());
    
  requestMessage.port = addy->port+10000;

  struct in_addr ip_addr;
  ip_addr.s_addr = addy->ip;
  char* addr = inet_ntoa(ip_addr);

  strncpy(requestMessage.host, addr, 128);
  printf("host=%s, port=%d\n", requestMessage.host, requestMessage.port);


  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    std::cout << "Error connecting to CSEG server for segmentation listen subscription\n";
    return ;
  }

  boost::asio::write(*socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );

}

void CoordinateSegmentationClient::reportLoad(ServerID sid, const BoundingBox3f& bbox, uint32 load) {

  LoadReportMessage requestMessage;

  requestMessage.loadValue = load;
  requestMessage.bbox.serialize(bbox);
  requestMessage.server = sid;

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    std::cout << "Error connecting to CSEG server for load reporting\n";
    return ;
  }

  boost::asio::write(*socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );

  boost::system::error_code error;
  boost::array<uint8, 1> buf;

  socket->read_some(boost::asio::buffer(buf), error);
}

boost::shared_ptr<TCPSocket> CoordinateSegmentationClient::getLeasedSocket() {
  if (mLeasedSocket.get() != 0 && mLeasedSocket->is_open()) {
    return mLeasedSocket;
  }
  else {        
    TCPResolver resolver(*mIOService);

    TCPResolver::query query(boost::asio::ip::tcp::v4(), GetOptionValue<String>("cseg-service-host"),
			     GetOptionValue<String>("cseg-service-tcp-port"));
    
    TCPResolver::iterator endpoint_iterator = resolver.resolve(query);
    
    TCPResolver::iterator end;
    
    mLeasedSocket = boost::shared_ptr<TCPSocket>( new TCPSocket(*mIOService) );    
    
    boost::system::error_code error = boost::asio::error::host_not_found;
    
    while (error && endpoint_iterator != end)
      {
	      mLeasedSocket->close();      
	      mLeasedSocket->connect(*endpoint_iterator++, error);      
      }
    
    if (error) {
      mLeasedSocket->close();
      
      std::cout << "Error connecting to  CSEG server for lookup...: " << error.message() << "\n";
      fflush(stdout);
      
      return boost::shared_ptr<TCPSocket>();
    }    

    mLeaseExpiryTime = Timer::now() + Duration::milliseconds(60000.0);
  }

  return mLeasedSocket;
}

ServerID CoordinateSegmentationClient::lookup(const Vector3f& pos)  {  
  LookupRequestMessage lookupMessage;
  lookupMessage.x = pos.x;
  lookupMessage.y = pos.y;
  lookupMessage.z = pos.z;  
  
  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    return 0;
  }
  
  boost::asio::write(*socket,
                     boost::asio::buffer((const void*)&lookupMessage,sizeof (lookupMessage)),
                     boost::asio::transfer_all() );

  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  bool failedOnce=false;
  for (;;)
  {
      boost::system::error_code error;
      boost::array<uint8, 1048576> buf;

      size_t len = socket->read_some(boost::asio::buffer(buf), error);

      if (dataReceived == NULL) {
      	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
	      dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;
      if (error == boost::asio::error::eof || bytesReceived >= sizeof(LookupResponseMessage) )
        break; // Connection closed cleanly by peer.
      else if (error) {
	       std::cout << "Error reading response from " << socket->remote_endpoint().address().to_string()
		      <<" in lookup\n\n\n";
      }
  }

  LookupResponseMessage* response = (LookupResponseMessage*)dataReceived;
  
  assert(response->type == LOOKUP_RESPONSE);

  ServerID retval = response->serverID;

  if (dataReceived != NULL) {
    free(dataReceived);
  }
  

  return retval;
}

BoundingBoxList CoordinateSegmentationClient::serverRegion(const ServerID& server)
{
  BoundingBoxList boundingBoxList;

  if (mServerRegionCache.find(server) != mServerRegionCache.end()) {
    // Returning cached serverRegion...
    return mServerRegionCache[server];
  }


  // Going to server for serverRegion for 'server'

  ServerRegionRequestMessage requestMessage;
  requestMessage.serverID = server;

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    //TODO: should return some error code instead; assuming no failures for now
    return boundingBoxList;
  }

  
  boost::asio::write(*socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );
  //Sent over serverRegion request to cseg server;


  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  uint32 serverRegionListLength = INT_MAX;
  for (;;)
  {
      boost::system::error_code error;
      boost::array<uint8, 1048576> buf;

      size_t len = socket->read_some(boost::asio::buffer(buf), error);

      if (dataReceived == NULL) {
      	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
      	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;

      if (serverRegionListLength == INT_MAX && bytesReceived >= sizeof(uint8) + sizeof(uint32)) {
	ServerRegionResponseMessage* msg = (ServerRegionResponseMessage*) dataReceived;
	serverRegionListLength = msg->listLength; 
      }      

      if (error == boost::asio::error::eof ||
	  bytesReceived >= sizeof(uint8) + sizeof(uint32) + serverRegionListLength*sizeof(SerializedBBox))
      {
        break; // Connection closed cleanly by peer.
      }
      else if (error) {
	      std::cout << "Error reading response from " << socket->remote_endpoint().address().to_string() <<" in serverRegion\n";
      }
  }

  if (dataReceived == NULL) {
    boundingBoxList.push_back(BoundingBox3f(Vector3f(0,0,0), Vector3f(0,0,0)));

    return boundingBoxList;
  }

  //Received reply from cseg server for server region

  ServerRegionResponseMessage* response = (ServerRegionResponseMessage*) malloc(bytesReceived);
  memcpy(response, dataReceived, bytesReceived);

  assert(response->type == SERVER_REGION_RESPONSE);

  for (uint32 i=0; i < response->listLength; i++) {
    BoundingBox3f bbox;
    SerializedBBox* sbbox = &(response->bboxList[i]);
    sbbox->deserialize(bbox);

    boundingBoxList.push_back(bbox);
  }

  free(dataReceived);
  free(response);

  //printf("serverRegion for %d returned %d boxes\n", server, boundingBoxList.size());
  /*for (int i=0 ; i<boundingBoxList.size(); i++) {
    //std::cout << server << " has bounding box : " << boundingBoxList[i] << "\n";
  }*/
  //memdump1((uint8*) response, bytesReceived);

  mServerRegionCache[server] = boundingBoxList;
 
  return boundingBoxList;
}

BoundingBox3f CoordinateSegmentationClient::region()  {
  if ( mTopLevelRegion.mBoundingBox.min().x  != mTopLevelRegion.mBoundingBox.max().x ) {
    //Returning cached region
    return mTopLevelRegion.mBoundingBox;
  }

  std::cout << "Going to server for region\n";fflush(stdout);

  RegionRequestMessage requestMessage;

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    return mTopLevelRegion.mBoundingBox;
  }

  boost::asio::write(*socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );

  std::cout << "Sent over region request to cseg server\n";

  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  for (;;)
  {
      boost::system::error_code error;
      boost::array<uint8, 1048576> buf;

      size_t len = socket->read_some(boost::asio::buffer(buf), error);
      

      if (dataReceived == NULL) {
      	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
      	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;
      if (error == boost::asio::error::eof || bytesReceived >= sizeof(RegionResponseMessage) ) {       
        break; // Connection closed cleanly by peer.
      }
      else if (error) {
	std::cout << "Error reading response from " << socket->remote_endpoint().address().to_string()<<" in region\n";
        fflush(stdout);
        
      	return mTopLevelRegion.mBoundingBox;
      }
  }

  std::cout << "Received reply from cseg server for region"  << "\n";fflush(stdout);

  if (dataReceived == NULL) {
    return mTopLevelRegion.mBoundingBox;
  }

  RegionResponseMessage* response = (RegionResponseMessage*) malloc(bytesReceived);
  memcpy(response, dataReceived, bytesReceived);
  assert(response->type == REGION_RESPONSE);


  BoundingBox3f bbox;
  SerializedBBox* sbbox = &response->bbox;
  sbbox->deserialize(bbox);

  free(dataReceived);
  free(response);

  std::cout << "Region returned " <<  bbox << "\n";fflush(stdout);

  mTopLevelRegion.mBoundingBox = bbox;

  return bbox;
}

uint32 CoordinateSegmentationClient::numServers()  {
  if (mAvailableServersCount > 0) {
    //Returning cached numServers 
    return mAvailableServersCount;
  }

  // Going to server for numServers

  NumServersRequestMessage requestMessage;

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    return 0;
  }

  boost::asio::write(*socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );

  // Sent over numservers request to cseg server


  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  for (;;)
  {
      boost::system::error_code error;
      boost::array<uint8, 1048576> buf;

      size_t len = socket->read_some(boost::asio::buffer(buf), error);

      if (dataReceived == NULL) {
	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;
      if (error == boost::asio::error::eof || bytesReceived >= sizeof(NumServersResponseMessage))
        break; // Connection closed cleanly by peer.
      else if (error) {
	std::cout << "Error reading response from " << socket->remote_endpoint().address().to_string() <<" in numservers\n";
	
	return mAvailableServersCount;
      }
  }

  // Received reply from cseg server for numservers

  NumServersResponseMessage* response = (NumServersResponseMessage*)dataReceived;
  assert(response->type == NUM_SERVERS_RESPONSE);

  uint32 retval = response->numServers;
  mAvailableServersCount = retval;

  if (dataReceived != NULL) {
    free(dataReceived);
  }

  std::cout << "numServers returned " <<  retval << "\n";
  return retval;
}

void CoordinateSegmentationClient::service() {
    mIOService->poll();

    boost::mutex::scoped_lock scopedLock(mMutex);


    if (mLeasedSocket.get() != 0 && mLeasedSocket->is_open() && Timer::now() > mLeaseExpiryTime ) {
      std::cout << "EXPIRED LEASE; CLOSED CONNECTION AT CLIENT\n"; fflush(stdout);

      mLeasedSocket->close();
    }
}

void CoordinateSegmentationClient::receiveMessage(Message* msg) {
}

void CoordinateSegmentationClient::csegChangeMessage(Sirikata::Protocol::CSeg::ChangeMessage* ccMsg) {

}

void CoordinateSegmentationClient::migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo ) {
}

void CoordinateSegmentationClient::downloadUpdatedBSPTree() {
}


} // namespace Sirikata
