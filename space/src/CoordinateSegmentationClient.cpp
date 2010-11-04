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
#include <sirikata/space/ServerMessage.hpp>
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
  : CoordinateSegmentation(ctx),  mBSPTreeValid(false), 
    mAvailableServersCount(0), mTopLevelRegion(NULL), 
    mIOService(Network::IOServiceFactory::makeIOService()),
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
  Sirikata::Protocol::CSeg::CSegMessage csegMessage;

  readCSEGMessage(mSocket, csegMessage);

  mSocket->close();

  boost::mutex::scoped_lock lock(mCacheMutex);
  mLookupCache.clear();
  mTopLevelRegion.destroy();
  mServerRegionCache.clear();
  lock.unlock();
 
  std::map<ServerID, SegmentationInfo> segmentationInfoMap;

  for (int i=0; i < csegMessage.change_message().region_size(); i++) {  
    ServerID id = csegMessage.change_message().region(i).id();

    /* [] will create a new entry in the map if not already there. */
    segmentationInfoMap[id].server = id;
    segmentationInfoMap[id].region.push_back(csegMessage.change_message().region(i).bounds());    
  }


  std::vector<SegmentationInfo> segInfoVector;
  for (std::map<ServerID, SegmentationInfo>::iterator it = segmentationInfoMap.begin();
       it != segmentationInfoMap.end(); it++)
  {
    printf("segInfo.server=%d, segInfo.region.size=%d\n", it->second.server, (int)it->second.region.size());
    fflush(stdout);

    segInfoVector.push_back(it->second);
  }

  notifyListeners(segInfoVector);
  
  startAccepting();
}

CoordinateSegmentationClient::~CoordinateSegmentationClient() {

}

void CoordinateSegmentationClient::sendSegmentationListenMessage() {  
  Address4* addy = mSidMap->lookupInternal(mContext->id());

  Sirikata::Protocol::CSeg::CSegMessage csegMessage;

  csegMessage.mutable_segmentation_listen_message().set_port(addy->port+10000);

  struct in_addr ip_addr;
  ip_addr.s_addr = addy->ip;
  char* addr = inet_ntoa(ip_addr);

  csegMessage.mutable_segmentation_listen_message().set_host(std::string(addr));
  printf("host=%s, port=%d\n", addr, addy->port+10000);

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    std::cout << "Error connecting to CSEG server for segmentation listen subscription\n";
    return ;
  }

  writeCSEGMessage(socket, csegMessage);
}

void CoordinateSegmentationClient::reportLoad(ServerID sid, const BoundingBox3f& bbox, uint32 load) {
  Sirikata::Protocol::CSeg::CSegMessage csegMessage;  

  csegMessage.mutable_load_report_message().set_load_value(load);
  csegMessage.mutable_load_report_message().set_bbox(bbox);
  csegMessage.mutable_load_report_message().set_server(sid);

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    std::cout << "Error connecting to CSEG server for load reporting\n";
    return ;
  }

  writeCSEGMessage(socket, csegMessage);
  
  readCSEGMessage(socket, csegMessage);
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
  {
    boost::mutex::scoped_lock cachelock(mCacheMutex);
    for (uint32 i=0 ; i<mLookupCache.size(); i++) {
      if (mLookupCache[i].bbox.contains(pos)) {
        
        assert(mLookupCache[i].sid >= 1 && mLookupCache[i].sid <= 5);
        
        //std::cout << "pos=" << pos   <<" , mLookupCache[i].sid: " << mLookupCache[i].sid << "\n";
        //fflush(stdout);
        
        return mLookupCache[i].sid;
      }
    }
  }
  

  Sirikata::Protocol::CSeg::CSegMessage csegMessage;

  csegMessage.mutable_lookup_request_message().set_x(pos.x);
  csegMessage.mutable_lookup_request_message().set_y(pos.y);
  csegMessage.mutable_lookup_request_message().set_z(pos.z);
  
  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    assert(false);
    return 0;
  }
  
  writeCSEGMessage(socket, csegMessage);  

  readCSEGMessage(socket, csegMessage);  

  ServerID retval = csegMessage.lookup_response_message().server_id();

  if (retval != 0 && csegMessage.lookup_response_message().has_server_bbox()) {
    boost::mutex::scoped_lock cachelock(mCacheMutex);
    
    mLookupCache.push_back( LookupCacheEntry(retval, csegMessage.lookup_response_message().server_bbox()) );    
  }
  
  return retval;
}

BoundingBoxList CoordinateSegmentationClient::serverRegion(const ServerID& server)
{
  boost::mutex::scoped_lock cachelock(mCacheMutex);
  if (mServerRegionCache.find(server) != mServerRegionCache.end()) {
    // Returning cached serverRegion...

    //std::cout << server  << " : " << mServerRegionCache[server][0] << "\n";fflush(stdout);
    return mServerRegionCache[server];
  } 
  cachelock.unlock();

  BoundingBoxList boundingBoxList;

  Sirikata::Protocol::CSeg::CSegMessage csegMessage;
  csegMessage.mutable_server_region_request_message().set_server_id(server);

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    assert(false);
    return boundingBoxList;
  }

  
  writeCSEGMessage(socket, csegMessage);
  //Sent over serverRegion request to cseg server;
  
  readCSEGMessage(socket, csegMessage);
  //Received reply from cseg server for server region

  
  for (int i=0; i < csegMessage.server_region_response_message().bbox_list_size(); i++) {
    BoundingBox3f bbox = csegMessage.server_region_response_message().bbox_list(i);

    boundingBoxList.push_back(bbox);
  }
  
  cachelock.lock();
  mServerRegionCache[server] = boundingBoxList;
  cachelock.unlock();
 
  //std::cout << server  << " : " << boundingBoxList[0] << "\n";fflush(stdout);

  return boundingBoxList;
}

BoundingBox3f CoordinateSegmentationClient::region()  {
  boost::mutex::scoped_lock cachelock(mCacheMutex);

  if ( mTopLevelRegion.mBoundingBox.min().x  != mTopLevelRegion.mBoundingBox.max().x ) {
    //Returning cached region
    return mTopLevelRegion.mBoundingBox;
  }

  cachelock.unlock();

  //Going to server for region

  Sirikata::Protocol::CSeg::CSegMessage csegMessage;
  csegMessage.mutable_region_request_message().set_filler(1);

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    return mTopLevelRegion.mBoundingBox;
  }

  writeCSEGMessage(socket, csegMessage);

  //Sent over region request to cseg server

  
  readCSEGMessage(socket, csegMessage);
  //Received reply from cseg server for region  

  BoundingBox3f bbox = csegMessage.region_response_message().bbox();

  cachelock.lock();
  mTopLevelRegion.mBoundingBox = bbox;
  cachelock.unlock();

  return bbox;
}

uint32 CoordinateSegmentationClient::numServers()  {
  boost::mutex::scoped_lock cachelock(mCacheMutex);
  
  if (mAvailableServersCount > 0) {
    //Returning cached numServers 
    return mAvailableServersCount;
  }

  cachelock.unlock();

  //Going to server for numServers
  Sirikata::Protocol::CSeg::CSegMessage csegMessage;
  csegMessage.mutable_num_servers_request_message().set_filler(1);
  

  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    return 0;
  }

  writeCSEGMessage(socket, csegMessage);

  // Sent over numservers request to cseg server

  readCSEGMessage(socket, csegMessage);

  // Received reply from cseg server for numservers

  uint32 retval = csegMessage.num_servers_response_message().num_servers();

  cachelock.lock();
  mAvailableServersCount = retval;
  cachelock.unlock();

  return retval;
}

std::vector<ServerID> CoordinateSegmentationClient::lookupBoundingBox(const BoundingBox3f& bbox) {
  std::vector<ServerID> serverList;

  //Serialize and send out the message.
  Sirikata::Protocol::CSeg::CSegMessage csegMessage;
  csegMessage.mutable_lookup_bbox_request_message().set_bbox(bbox);
  
  boost::mutex::scoped_lock scopedLock(mMutex);
  boost::shared_ptr<TCPSocket> socket = getLeasedSocket();

  if (socket == boost::shared_ptr<TCPSocket>()) {
    assert(false);    
  }
  
  writeCSEGMessage(socket, csegMessage);

  //Read back the response
  readCSEGMessage(socket, csegMessage);

  //Return the response  
 
  for (int i=0; i<csegMessage.lookup_bbox_response_message().server_list_size(); i++) {
    serverList.push_back(csegMessage.lookup_bbox_response_message().server_list(i) );
  }  

  return serverList;
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

void CoordinateSegmentationClient::writeCSEGMessage(boost::shared_ptr<tcp::socket> socket, 
                                                    Sirikata::Protocol::CSeg::CSegMessage& csegMessage)
{
  std::string buffer = serializePBJMessage(csegMessage);

  uint32 length = htonl(buffer.size());

  buffer = std::string( (char*)&length, sizeof(uint32)) + buffer;

  boost::asio::write(*socket,
                     boost::asio::buffer((void*) buffer.data(), buffer.size()),
                     boost::asio::transfer_all() );
}

void CoordinateSegmentationClient::readCSEGMessage(boost::shared_ptr<tcp::socket> socket, 
                                                        Sirikata::Protocol::CSeg::CSegMessage& csegMessage)
{
  //should have a timeout in case of cseg server failure, but
  //assuming no failures for now...
  uint32 length;
  boost::asio::read(*socket, boost::asio::buffer( (void*)(&length), sizeof(uint32) ), boost::asio::transfer_all());
  length = ntohl(length);
  
  uint8* buf = new uint8[length];
  boost::asio::read(*socket, boost::asio::buffer(buf, length), boost::asio::transfer_all());

  std::string str = std::string((char*) buf, length);

  bool val = parsePBJMessage(&csegMessage, str);

  assert(val);

  delete buf;
}



} // namespace Sirikata
