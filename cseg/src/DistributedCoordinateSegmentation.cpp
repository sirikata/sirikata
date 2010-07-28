/*  Sirikata
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

#include <sirikata/core/util/Platform.hpp>
#include "DistributedCoordinateSegmentation.hpp"

#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/bind.hpp>


#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/network/Message.hpp>
#include <sirikata/core/util/Hash.hpp>
#include "WorldPopulationBSPTree.hpp"
#include <sirikata/core/network/ServerIDMap.hpp>


namespace Sirikata {

template<typename T>
T clamp(T val, T minval, T maxval) {
  if (val < minval) return minval;
  if (val > maxval) return maxval;
  return val;
}

void memdump(uint8* buffer, int len) {
  for (int i=0; i<len; i++) {
    uint8* val = buffer+i;
    printf("at addr %p, val=%d\n",  val, *val);
  }
  printf("\n");
  fflush(stdout);
}

bool isPowerOfTwo(double n) {
  if (n < 1.0) return false;

  if (n == 1.0) return true;

  return isPowerOfTwo(n/2.0);
}


String sha1(void* data, size_t len) {
  uint32 hash_len_quad = 5;
  uint32 hash_len = hash_len_quad * 4;
  uint32 digest[5];
  Sirikata::Sha1(data, len, digest);

  char *out = (char *) malloc( sizeof(char) * ((hash_len*2)+1) );
  memset(out, 0, sizeof(char) * ((hash_len*2)+1) ); 
  
  /* Convert each byte to its 2 digit ascii
   * hex representation and place in out */
  char *p = out;
  for ( uint32 i = 0; i < hash_len_quad; i++, p += 8 ) {
    snprintf ( p, 9, "%08x", digest[i] );
    fflush(stdout);
  }

  String retval = out;
  free(out);

  return retval;
}

void DistributedCoordinateSegmentation::subdivideTopLevelRegion(SegmentedRegion* region,
								Vector3ui32 perdim,
								int& numServersAssigned)
{
  assert( isPowerOfTwo(perdim.x) );
  assert( isPowerOfTwo(perdim.y) );
  assert( isPowerOfTwo(perdim.z) );

  if (perdim.x == 1 && perdim.y == 1 && perdim.z == 1) {
    region->mServer = (++numServersAssigned);
    std::cout << "Server " << numServersAssigned << " assigned: " << region->mBoundingBox << "\n";
    return;
  }

  float minX = region->mBoundingBox.min().x; float maxX = region->mBoundingBox.max().x;
  float minY = region->mBoundingBox.min().y; float maxY = region->mBoundingBox.max().y;
  float minZ = region->mBoundingBox.min().z; float maxZ = region->mBoundingBox.max().z;

  region->mLeftChild = new SegmentedRegion(region);
  region->mRightChild = new SegmentedRegion(region);

  if (perdim.x > 1) {
    region->mLeftChild->mBoundingBox = BoundingBox3f( region->mBoundingBox.min(),
						      Vector3f( (minX+maxX)/2, maxY, maxZ) );
    region->mRightChild->mBoundingBox = BoundingBox3f( Vector3f( (minX+maxX)/2, minY, minZ),
						       region->mBoundingBox.max() );

    subdivideTopLevelRegion(region->mLeftChild,
			    Vector3ui32(perdim.x/2, perdim.y, perdim.z),
			    numServersAssigned
			   );
    subdivideTopLevelRegion(region->mRightChild,
			    Vector3ui32(perdim.x/2, perdim.y, perdim.z),
			    numServersAssigned
			   );
  }
  else if (perdim.y > 1) {
    region->mLeftChild->mBoundingBox = BoundingBox3f( region->mBoundingBox.min(),
						      Vector3f( maxX, (minY+maxY)/2, maxZ) );
    region->mRightChild->mBoundingBox = BoundingBox3f( Vector3f(minX,(minY+maxY)/2,minZ),
						       region->mBoundingBox.max() );

    subdivideTopLevelRegion(region->mLeftChild,
			    Vector3ui32(perdim.x, perdim.y/2, perdim.z),
			    numServersAssigned
			   );
    subdivideTopLevelRegion(region->mRightChild,
			    Vector3ui32(perdim.x, perdim.y/2, perdim.z),
			    numServersAssigned
			   );
  }
  else if (perdim.z > 1) {
    region->mLeftChild->mBoundingBox = BoundingBox3f( region->mBoundingBox.min(),
						      Vector3f( maxX, maxY, (minZ+maxZ)/2) );
    region->mRightChild->mBoundingBox = BoundingBox3f( Vector3f(minX, minY , (minZ+maxZ)/2),
						       region->mBoundingBox.max() );

    subdivideTopLevelRegion(region->mLeftChild,
			    Vector3ui32(perdim.x, perdim.y, perdim.z/2),
			    numServersAssigned
			   );
    subdivideTopLevelRegion(region->mRightChild,
			    Vector3ui32(perdim.x, perdim.y, perdim.z/2),
			    numServersAssigned
			   );
  }
}

DistributedCoordinateSegmentation::DistributedCoordinateSegmentation(CSegContext* ctx, const BoundingBox3f& region, 
                                                                     const Vector3ui32& perdim, int nservers, ServerIDMap * sidmap)
 : PollingService(ctx->mainStrand, Duration::milliseconds((int64)1000)),
   mContext(ctx),
   mTopLevelRegion(NULL),
   mLastUpdateTime(Time::null()),
   mLoadBalancer(this, nservers, perdim),
   mSidMap(sidmap)  
{
  mAvailableCSEGServers = GetOptionValue<uint16>("num-cseg-servers");

  assert (nservers >= (int)(perdim.x * perdim.y * perdim.z));

  if (GetOptionValue<bool>("cseg-uses-world-pop")) {
    WorldPopulationBSPTree wPopTree;
    wPopTree.constructBSPTree(mTopLevelRegion);
  }
  else {
    mTopLevelRegion.mBoundingBox = region;

    int numServersAssigned = 0;
    subdivideTopLevelRegion(&mTopLevelRegion, perdim, numServersAssigned);
  }

  int numLLTreesSoFar = 0;
  generateHierarchicalTrees(&mTopLevelRegion, 1, numLLTreesSoFar);
  

  if (ctx->id() == 1) {
      mAcceptor = boost::shared_ptr<tcp::acceptor>(new tcp::acceptor(mIOService,tcp::endpoint(tcp::v4(), atoi( GetOptionValue<String>("cseg-service-tcp-port").c_str() ))));
    startAccepting();
  }

  Address4* addy = sidmap->lookupInternal(ctx->id());
  uint16 cseg_server_ll_port = addy->getPort()+10000;
  mLLTreeAcceptor = boost::shared_ptr<tcp::acceptor>(new tcp::acceptor(mLLIOService,tcp::endpoint(tcp::v4(), cseg_server_ll_port)));

  startAcceptingLLRequests();

  mMessageSizes[LOOKUP_REQUEST] = sizeof(LookupRequestMessage);
  mMessageSizes[NUM_SERVERS_REQUEST] = sizeof(NumServersRequestMessage);
  mMessageSizes[SERVER_REGION_REQUEST] = sizeof(ServerRegionRequestMessage);
  mMessageSizes[REGION_REQUEST] = sizeof(RegionRequestMessage);
  mMessageSizes[SEGMENTATION_LISTEN] = sizeof(SegmentationListenMessage);
  mMessageSizes[LL_SERVER_REGION_REQUEST] = 1+sizeof(ServerID);
  mMessageSizes[LL_LOOKUP_REQUEST] = 1+sizeof(SerializedVector)+sizeof(SerializedBBox);
  mMessageSizes[LOAD_REPORT] = sizeof(LoadReportMessage);
  mMessageSizes[LL_LOAD_REPORT] = 1 + sizeof(SerializedBBox) + sizeof(LoadReportMessage);
  mMessageSizes[LL_LOOKUP_RESPONSE] = 1 + sizeof(ServerID);

  Thread thrd(boost::bind(&DistributedCoordinateSegmentation::ioServicingLoop, this));
  Thread thrd2(boost::bind(&DistributedCoordinateSegmentation::llIOServicingLoop, this));
}

DistributedCoordinateSegmentation::~DistributedCoordinateSegmentation() {
  //delete all the SegmentedRegion objects created with 'new'
  mTopLevelRegion.destroy();
}

void DistributedCoordinateSegmentation::ioServicingLoop() {
  mIOService.run();
}

void DistributedCoordinateSegmentation::llIOServicingLoop() {
  mLLIOService.run();
}

ServerID DistributedCoordinateSegmentation::lookup(const Vector3f& pos) {
  ServerID sid;

  Time start_time = Timer::now();

  Vector3f searchVec = pos;
  BoundingBox3f region = mTopLevelRegion.mBoundingBox;

  int i=0;
  (searchVec.z < region.min().z) ? searchVec.z = region.min().z : (i=0);
  (searchVec.x < region.min().x) ? searchVec.x = region.min().x : (i=0);
  (searchVec.y < region.min().y) ? searchVec.y = region.min().y : (i=0);

  (searchVec.z > region.max().z) ? searchVec.z = region.max().z : (i=0);
  (searchVec.x > region.max().x) ? searchVec.x = region.max().x : (i=0);
  (searchVec.y > region.max().y) ? searchVec.y = region.max().y : (i=0);

  const SegmentedRegion* segRegion = mTopLevelRegion.lookup(searchVec);
  ServerID topLevelIdx = segRegion->mServer;

  //std::cout << "Returned remote CSEG: " << topLevelIdx << " for vector " << pos <<"\n";

  if (topLevelIdx == mContext->id())
  {
    SerializedBBox serializedBBox;
    serializedBBox.serialize(segRegion->mBoundingBox);
    String bbox_hash = sha1(&serializedBBox, sizeof(serializedBBox));

    std::map<String, SegmentedRegion*>::iterator it=  mHigherLevelTrees.find(bbox_hash);

    if (it != mHigherLevelTrees.end()) {
      segRegion = (*it).second->lookup(searchVec);

      //printf("local lookup returned %d\n", segRegion->mServer);

      sid = segRegion->mServer;
    }
    else {
      it =  mLowerLevelTrees.find(bbox_hash);

      if (it != mLowerLevelTrees.end()) {
        segRegion = (*it).second->lookup(searchVec);

        //printf("local lookup returned %d\n", segRegion->mServer);

        sid = segRegion->mServer;
      }
      else {
        sid = 0;
      }
    }
  }
  else {
    //remote function call to the relevant server
    std::cout << "local_processing_time: " << (Timer::now() - start_time).toMicroseconds() << "\n";
    sid= callLowerLevelCSEGServer(topLevelIdx, searchVec, segRegion->mBoundingBox);
    std::cout << "Total_time: " << (Timer::now() - start_time).toMicroseconds() << "\n";
    fflush(stdout);
    return sid;
  }

  return sid;
}

BoundingBoxList DistributedCoordinateSegmentation::serverRegion(const ServerID& server)
{
  if (mWholeTreeServerRegionMap.find(server) != mWholeTreeServerRegionMap.end()) {
    return mWholeTreeServerRegionMap[server];
  }

  BoundingBoxList boundingBoxList;

  for(std::map<String, SegmentedRegion*>::const_iterator it = mHigherLevelTrees.begin();
      it != mHigherLevelTrees.end(); ++it)
  {
      it->second->serverRegion(server, boundingBoxList);
  }

  for(std::map<String, SegmentedRegion*>::const_iterator it = mLowerLevelTrees.begin();
      it != mLowerLevelTrees.end(); ++it)
  {
      it->second->serverRegion(server, boundingBoxList);
  }

  callLowerLevelCSEGServersForServerRegions(server, boundingBoxList);

  if (boundingBoxList.size() > 0) {
      mWholeTreeServerRegionMap[server] = boundingBoxList;
  }

  if (boundingBoxList.size() == 0) {
    boundingBoxList.push_back(BoundingBox3f(Vector3f(0,0,0), Vector3f(0,0,0)));
  }

  //printf("Finally, server regions=%d for server ID %d\n", boundingBoxList.size(), server);
  //std::cout << "boundingBox for server " << server << " : " << boundingBoxList[0] << "\n";

  return boundingBoxList;
}

BoundingBox3f DistributedCoordinateSegmentation::region()  {
  return mTopLevelRegion.mBoundingBox;
}

uint32 DistributedCoordinateSegmentation::numServers() {  
  return mLoadBalancer.numAvailableServers();

  //int count = mTopLevelRegion.countServers();
  //return count;
}

void DistributedCoordinateSegmentation::handleLoadReport(LoadReportMessage* message) {    
  ServerID sid = message->server;
  BoundingBox3f bbox ;
  message->bbox.deserialize(bbox);

  Vector3f searchVec = Vector3f( (bbox.min().x+bbox.max().x)/2.0, (bbox.min().y+bbox.max().y)/2,
                                 (bbox.min().z+bbox.max().z)/2.0 );  

  SegmentedRegion* segRegion = mTopLevelRegion.lookup(searchVec);
  ServerID topLevelIdx = segRegion->mServer;
  
  if (topLevelIdx == mContext->id())
  {
    SerializedBBox serializedBBox;
    serializedBBox.serialize(segRegion->mBoundingBox);   
    String bbox_hash = sha1(&serializedBBox, sizeof(serializedBBox));

    std::map<String, SegmentedRegion*>::iterator it=  mHigherLevelTrees.find(bbox_hash);

    if (it != mHigherLevelTrees.end()) {
      segRegion = (*it).second->lookup(searchVec);      

      // deal with the value for this region's load; 
      if (sid == segRegion->mServer && bbox == segRegion->mBoundingBox) {
        segRegion->mLoadValue = message->loadValue;

        mLoadBalancer.reportRegionLoad(segRegion, sid, segRegion->mLoadValue);
      }
    }
    else {
      it =  mLowerLevelTrees.find(bbox_hash);

      if (it != mLowerLevelTrees.end()) {
        segRegion = (*it).second->lookup(searchVec);

        // deal with the value for this region's load.
        if (sid == segRegion->mServer && bbox == segRegion->mBoundingBox) {
          segRegion->mLoadValue = message->loadValue;
          mLoadBalancer.reportRegionLoad(segRegion, sid, segRegion->mLoadValue);
        }
      }
      else {
        // you should never be here.
        assert(false);
      }
    }    
  }
  else {
    //remote function call to the relevant server
    sendLoadReportToLowerLevelCSEGServer(topLevelIdx, searchVec, segRegion->mBoundingBox, message);
  }
}

void DistributedCoordinateSegmentation::getRandomLeafParentSibling(SegmentedRegion** randomLeaf,
								   SegmentedRegion** sibling,
								   SegmentedRegion** parent)
{
  if (mLowerLevelTrees.size() == 0) {
    int numHigherLevelTrees = rand() % mHigherLevelTrees.size();
    int i=0;

    for (std::map<String, SegmentedRegion*>::iterator it=mHigherLevelTrees.begin();
	        it != mHigherLevelTrees.end();
	        it++, i++)
      {
      	if (i == numHigherLevelTrees) {

	       *randomLeaf = it->second->getRandomLeaf();

	       *sibling = it->second->getSibling(*randomLeaf);
	       *parent = it->second->getParent(*randomLeaf);

	        break;
	      }
      }
  }
  else {
    int numLowerLevelTrees = rand() % mLowerLevelTrees.size();
    int i=0;

    for (std::map<String, SegmentedRegion*>::iterator it=mLowerLevelTrees.begin();
	 it != mLowerLevelTrees.end();
	 it++, i++)
      {
	if (i == numLowerLevelTrees) {

	  *randomLeaf = it->second->getRandomLeaf();

	  *sibling = it->second->getSibling(*randomLeaf);
	  *parent = it->second->getParent(*randomLeaf);

	  break;
	}
      }
  }
}

void DistributedCoordinateSegmentation::stop() {
  mIOService.stop();

  mLLIOService.stop();
}

void DistributedCoordinateSegmentation::poll() {
    service();
}

void DistributedCoordinateSegmentation::service() {  
  boost::unique_lock<boost::shared_mutex> lock(mCSEGReadWriteMutex);

  mLoadBalancer.service();
}

void DistributedCoordinateSegmentation::notifySpaceServersOfChange(const std::vector<SegmentationInfo> segInfoVector)
{
  if (segInfoVector.size() == 0) {
    return;
  }

  /* Initialize the serialized message to send over the wire */
  SegmentationChangeMessage* segChangeMsg = new SegmentationChangeMessage();


  segChangeMsg->numEntries = (MAX_SERVER_REGIONS_CHANGED < segInfoVector.size())
                            ? MAX_SERVER_REGIONS_CHANGED : segInfoVector.size();

  uint32 msgSize = 1+1;

  /* Fill in the fields in the message */
  for (unsigned int i=0 ;i<MAX_SERVER_REGIONS_CHANGED && i<segInfoVector.size(); i++) {
    SerializedSegmentChange* segmentChange = &segChangeMsg->changedSegments[i];
    segmentChange->serverID = segInfoVector[i].server;
    msgSize += sizeof(ServerID);

    segmentChange->listLength = segInfoVector[i].region.size();
    msgSize += sizeof(uint32);

    for (unsigned int j=0; j<segInfoVector[i].region.size(); j++) {
      BoundingBox3f bbox = segInfoVector[i].region[j];

      segmentChange->bboxList[j].serialize(bbox);
    }

    msgSize += segInfoVector[i].region.size() * sizeof(SerializedBBox);
  }

  uint8* buffer=NULL;
  uint32 buflen = segChangeMsg->serialize(&buffer);

  /* Send to CSEG servers connected to this server.  */
  sendToAllCSEGServers(buffer, buflen);

  /* Send to space servers connected to this server.  */
  sendToAllSpaceServers(buffer, buflen);

  delete buffer;

  delete segChangeMsg;

  printf("Notified all space servers of change\n");
}

void DistributedCoordinateSegmentation::csegChangeMessage(Sirikata::Protocol::CSeg::ChangeMessage* ccMsg) {
}

void DistributedCoordinateSegmentation::serializeBSPTree(SerializedBSPTree* serializedBSPTree) {
  boost::shared_lock<boost::shared_mutex> mCSEGExclusiveWriteLock(mCSEGReadWriteMutex);

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

uint32 DistributedCoordinateSegmentation::readFromSocket(boost::shared_ptr<tcp::socket> socket,
							 uint8** dataReceived,
							 bool readTillEOF,
							 uint8 bytesReceivedAlready)
{
  uint32 bytesReceived = bytesReceivedAlready; 
  
  /* Read in the data from the socket. */
  for (;;)
  {
      if ( !readTillEOF && bytesReceived > 0 &&
	   fullMessageReceived((*dataReceived), bytesReceived) ) 
      {
      	break;
      }

      boost::array<uint8, 65536> buf;      
      boost::system::error_code error;

      // Reading from socket
      size_t len = socket->read_some(boost::asio::buffer(buf), error);      

      if (dataReceived == NULL) {
	      *dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
	      *dataReceived = (uint8*) realloc(*dataReceived, bytesReceived+len);
      }
      memcpy((*dataReceived)+bytesReceived, buf.c_array(), len);

      bytesReceived += len;

      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error) {
      	std::cout << "Error reading request from "
	          << socket->remote_endpoint().address().to_string() << "\n";

        throw boost::system::system_error(error); // Some other error.
      }
  }

  return bytesReceived;
}

void DistributedCoordinateSegmentation::accept_handler()
{
  uint8* asyncBufferArray = new uint8[1];
    
  mSocket->async_read_some( boost::asio::buffer(asyncBufferArray, 1) , 
			    std::tr1::bind(&DistributedCoordinateSegmentation::asyncRead, this, 
					   mSocket, asyncBufferArray, _1, _2)  );

  startAccepting();
}

void DistributedCoordinateSegmentation::asyncRead(boost::shared_ptr<tcp::socket> socket, 
						  uint8* asyncBufferArray,
						  const boost::system::error_code& ec,
						  std::size_t bytes_transferred)
{
  if (ec == boost::asio::error::eof) {
    // EOF RECEIVED; CLOSING CONNECTION AT SERVER
    delete asyncBufferArray;
    socket->close();    
    return;
  }
  

  uint8* dataReceived = (uint8*) malloc(1);
  dataReceived[0] = asyncBufferArray[0];
  uint32 bytesReceived = readFromSocket(socket, &dataReceived, false, 1);

  boost::shared_lock<boost::shared_mutex> mCSEGExclusiveWriteLock(mCSEGReadWriteMutex);

  /* Deal with the request included in the received data */  
  GenericMessage* genericMessage = (GenericMessage*) dataReceived;
  
  if (genericMessage->type == LOOKUP_REQUEST) {
    LookupRequestMessage* lookupMessage = (LookupRequestMessage*) dataReceived;

    LookupResponseMessage lookupResponseMessage;
    lookupResponseMessage.serverID =
      lookup(Vector3f(lookupMessage->x, lookupMessage->y, lookupMessage->z));

    boost::asio::write(*socket,
		       boost::asio::buffer( &lookupResponseMessage, sizeof(lookupResponseMessage)),
		       boost::asio::transfer_all() );
  }
  else if (genericMessage->type == NUM_SERVERS_REQUEST) {
    NumServersResponseMessage responseMessage;
    responseMessage.numServers = numServers();

    boost::asio::write(*socket,
		       boost::asio::buffer( &responseMessage, sizeof(responseMessage)),
		       boost::asio::transfer_all() );
  }
  else if (genericMessage->type == REGION_REQUEST) {
    RegionResponseMessage responseMessage;
    BoundingBox3f bbox = region();

    responseMessage.bbox.serialize(bbox);

    boost::asio::write(*socket,
		       boost::asio::buffer((void*) &responseMessage, sizeof(responseMessage)),
		       boost::asio::transfer_all() );
  }
  else if (genericMessage->type == SERVER_REGION_REQUEST) {
    ServerRegionRequestMessage* message = (ServerRegionRequestMessage*) dataReceived;

    BoundingBoxList bboxList = serverRegion(message->serverID);

    ServerRegionResponseMessage* responseMessage = new ServerRegionResponseMessage();
    uint32 i=0;
    for (i=0; i<MAX_BBOX_LIST_SIZE && i < bboxList.size() ; i++) {
      BoundingBox3f bbox = bboxList[i];
      responseMessage->bboxList[i].serialize(bbox);
    }

    responseMessage->listLength = bboxList.size();

    boost::asio::write(*socket,
		       boost::asio::buffer((void*) responseMessage, 1+4+bboxList.size()*sizeof(SerializedBBox)),
		       boost::asio::transfer_all() );
    delete responseMessage;
  }
  else if (genericMessage->type == SEGMENTATION_LISTEN) {
    SegmentationListenMessage* message = (SegmentationListenMessage*) dataReceived;
    SegmentationChangeListener sl;
    memcpy(sl.host, message->host, 255);
    printf("subscribing hostname=%s\n", message->host);
    sl.port = message->port;

    mSpacePeers.push_back(sl);
  }
  else if (genericMessage->type == LOAD_REPORT) {
    LoadReportMessage* message = (LoadReportMessage*) dataReceived;

    handleLoadReport(message);

    uint8 ack=0;
    boost::asio::write(*socket,
		       boost::asio::buffer((void*) &ack, 1),
		       boost::asio::transfer_all() );    
  }

  socket->async_read_some( boost::asio::buffer(asyncBufferArray, 1) , 
			   std::tr1::bind(&DistributedCoordinateSegmentation::asyncRead, this, 
					  socket, asyncBufferArray, _1, _2)  );

  free(dataReceived);
}

void DistributedCoordinateSegmentation::asyncLLRead(boost::shared_ptr<tcp::socket> socket,
              uint8* asyncBufferArray,
              const boost::system::error_code& ec,
              std::size_t bytes_transferred)
{
  Time start = Timer::now();

  if (ec == boost::asio::error::eof) {
    // EOF RECEIVED; CLOSING CONNECTION AT SERVER
    delete asyncBufferArray;
    socket->close();
    return;
  }
  
  uint8* dataReceived = (uint8*) malloc(1);
  dataReceived[0] = asyncBufferArray[0];
  uint32 bytesReceived = readFromSocket(socket, &dataReceived, false, 1);

  boost::shared_lock<boost::shared_mutex> mCSEGExclusiveWriteLock(mCSEGReadWriteMutex);

  // Now decode the packet from the received buffer
  uint8 packetType = dataReceived[0];

  if (packetType == LL_LOOKUP_REQUEST) {
    SerializedVector serialVector;
    SerializedBBox serialBox;
    memcpy(&serialVector, dataReceived+1, sizeof(serialVector));
    memcpy(&serialBox, dataReceived+1+sizeof(serialVector), sizeof(serialBox));

    BoundingBox3f bbox; Vector3f vect;
    serialBox.deserialize(bbox);
    serialVector.deserialize(vect);

    String bbox_hash = sha1(&serialBox, sizeof(serialBox));
    std::map<String, SegmentedRegion*>::iterator it=  mLowerLevelTrees.find(bbox_hash);

    ServerID retval = 0;
    if (it != mLowerLevelTrees.end()) {
      const SegmentedRegion*segRegion = (*it).second->lookup(vect);


      retval = segRegion->mServer;
    }

    uint8* buffer = new uint8[1+sizeof(ServerID)];
    buffer[0] = LL_LOOKUP_RESPONSE;
    memcpy(buffer+1, &retval, sizeof(retval));
    boost::asio::write(*socket,
		       boost::asio::buffer((const void*)buffer, 1+sizeof(ServerID)),
		       boost::asio::transfer_all() );

    delete buffer;
  }
  else if (packetType == LL_SERVER_REGION_REQUEST) {
    ServerID serverID;
    memcpy(&serverID, dataReceived + 1, sizeof(ServerID));
    BoundingBoxList boundingBoxList;

    printf("serverRegion being called on ll tree with server id=%d\n", serverID);
          fflush(stdout);

    if (mLowerTreeServerRegionMap.find(serverID) != mLowerTreeServerRegionMap.end()) {
      boundingBoxList = mLowerTreeServerRegionMap[serverID];
    }
    else {
      for(std::map<String, SegmentedRegion*>::const_iterator it = mLowerLevelTrees.begin();
          it != mLowerLevelTrees.end(); ++it)
    	{
          
          it->second->serverRegion(serverID, boundingBoxList);
	}
    }

    if (boundingBoxList.size()>0 && mLowerTreeServerRegionMap.find(serverID) == mLowerTreeServerRegionMap.end()) {
      mLowerTreeServerRegionMap[serverID] = boundingBoxList;
    }

    uint8* buffer = new uint8[1 + sizeof(uint32) + boundingBoxList.size() * sizeof(SerializedBBox)];
    buffer[0] = LL_SERVER_REGION_RESPONSE;
    uint32 numBBoxes = boundingBoxList.size();
    memcpy(buffer+1, &numBBoxes, sizeof(numBBoxes));
    for (uint32 i = 0; i < boundingBoxList.size(); i++) {
      SerializedBBox serializedBbox;
      serializedBbox.serialize(boundingBoxList[i]);

      uint8* bufferPos = buffer + 1 + sizeof(numBBoxes) + i * sizeof(SerializedBBox);
      memcpy(bufferPos, &serializedBbox , sizeof(SerializedBBox));
    }

    boost::asio::write(*socket,
		       boost::asio::buffer((const void*)buffer, 1 + sizeof(uint32) + boundingBoxList.size() * sizeof(SerializedBBox)),
		       boost::asio::transfer_all() );

    delete buffer;
  }
  else if (packetType == SEGMENTATION_CHANGE) {

    tcp::resolver resolver(mIOService);

    mWholeTreeServerRegionMap.clear();

    SegmentationChangeMessage* segChangeMessage = (SegmentationChangeMessage*) dataReceived;

    mLoadBalancer.handleSegmentationChange(segChangeMessage);

    sendToAllSpaceServers(dataReceived, bytesReceived);
  }
  else if (packetType == LL_LOAD_REPORT) {
    SerializedBBox lowerTreeRootBox;
    memcpy(&lowerTreeRootBox, dataReceived+1, sizeof(lowerTreeRootBox));
    String bbox_hash = sha1(&lowerTreeRootBox, sizeof(lowerTreeRootBox));
    std::map<String, SegmentedRegion*>::iterator it=  mLowerLevelTrees.find(bbox_hash);

    LoadReportMessage* loadReportMessage = (LoadReportMessage*) (dataReceived + 1 + sizeof(SerializedBBox));
    BoundingBox3f leafBBox;
    SerializedBBox serializedLeafBBox;
    memcpy(&serializedLeafBBox, &loadReportMessage->bbox, sizeof(serializedLeafBBox));
    serializedLeafBBox.deserialize(leafBBox);
    Vector3f vect( (leafBBox.min().x+leafBBox.max().x)/2.0, (leafBBox.min().y+leafBBox.max().y)/2,
                   (leafBBox.min().z+leafBBox.max().z)/2.0 );

    if (it != mLowerLevelTrees.end()) {
      SegmentedRegion* segRegion = (*it).second->lookup(vect);

      if (segRegion->mServer == loadReportMessage->server && segRegion->mBoundingBox == leafBBox) {
        //deal with the load from the space server
        segRegion->mLoadValue = loadReportMessage->loadValue;

        mLoadBalancer.reportRegionLoad(segRegion, segRegion->mServer, segRegion->mLoadValue);
      }
    }
    else {
      assert(false);
    }

    uint8 ack=0;
    boost::asio::write(*socket,
		       boost::asio::buffer((void*) &ack, 1),
		       boost::asio::transfer_all() );
  }

  if (dataReceived != NULL) {
    free(dataReceived);
  }

  socket->async_read_some( boost::asio::buffer(asyncBufferArray, 1) ,
         std::tr1::bind(&DistributedCoordinateSegmentation::asyncLLRead, this,
            socket, asyncBufferArray, _1, _2)  );

  std::cout << "LLCall:" << (Timer::now() -start).toMicroseconds() << "\n";
  fflush(stdout);
}

void DistributedCoordinateSegmentation::startAccepting() {
  mSocket = boost::shared_ptr<tcp::socket>(new tcp::socket(mIOService));
  mAcceptor->async_accept(*mSocket, boost::bind(&DistributedCoordinateSegmentation::accept_handler,this)); 
}

void DistributedCoordinateSegmentation::startAcceptingLLRequests() {
  mLLTreeAcceptorSocket = boost::shared_ptr<tcp::socket>(new tcp::socket(mLLIOService));
  mLLTreeAcceptor->async_accept(*mLLTreeAcceptorSocket, boost::bind(&DistributedCoordinateSegmentation::acceptLLTreeRequestHandler,this));
}

void DistributedCoordinateSegmentation::acceptLLTreeRequestHandler() {
  uint8* asyncBufferArray = new uint8[1];

  mLLTreeAcceptorSocket->async_read_some( boost::asio::buffer(asyncBufferArray, 1) ,
          std::tr1::bind(&DistributedCoordinateSegmentation::asyncLLRead, this,
             mLLTreeAcceptorSocket, asyncBufferArray, _1, _2)  );

  startAcceptingLLRequests();
}

void DistributedCoordinateSegmentation::generateHierarchicalTrees(SegmentedRegion* region, int depth, int& numLLTreesSoFar) {
  int cutOffDepth = 2;

  if (depth>=cutOffDepth) {
    region->mServer =  (numLLTreesSoFar % mAvailableCSEGServers)+1;

    if ( region->mServer == mContext->id()) {
      // Assigned region->mBoundingBox to me.
      SegmentedRegion* segRegion = new SegmentedRegion(NULL);
      segRegion->mBoundingBox = region->mBoundingBox;
      segRegion->mLeftChild = region->mLeftChild;
      segRegion->mRightChild = region->mRightChild;

      if (region->mLeftChild == NULL && region->mRightChild == NULL) {
	      segRegion->mServer = region->mServer;
      }

      SerializedBBox serializedBBox;
      serializedBBox.serialize(segRegion->mBoundingBox);
      String bbox_hash = sha1(&serializedBBox, sizeof(serializedBBox));
      mLowerLevelTrees[bbox_hash] = segRegion;
    }

    numLLTreesSoFar++;

    region->mLeftChild = NULL;
    region->mRightChild = NULL;

    return;
  }

  if (region->mLeftChild == NULL && region->mRightChild == NULL) {

    SegmentedRegion* segRegion = new SegmentedRegion(NULL);
    segRegion->mBoundingBox = region->mBoundingBox;
    segRegion->mServer = region->mServer;
    region->mServer = mContext->id();

    SerializedBBox serializedBBox;
    serializedBBox.serialize(segRegion->mBoundingBox);

    String bbox_hash = sha1(&serializedBBox, sizeof(serializedBBox));
    mHigherLevelTrees[bbox_hash] = segRegion;
    return;
  }

  generateHierarchicalTrees(region->mLeftChild, depth+1, numLLTreesSoFar);
  generateHierarchicalTrees(region->mRightChild, depth+1, numLLTreesSoFar);
}

SocketContainer DistributedCoordinateSegmentation::getSocketToCSEGServer(ServerID server_id) {
  // get upgradable access
  boost::upgrade_lock<boost::shared_mutex> lock(_access);

  std::map<ServerID, SocketQueuePtr >::iterator it = mLeasedSocketsToCSEGServers.find(server_id);
  if (it != mLeasedSocketsToCSEGServers.end()) {    
    SocketContainer socket;
    
    bool retval = it->second->pop(socket);

    if (retval) {
      return socket;
    }
  }

  // get exclusive access
  boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

  //Get a new socket
  tcp::resolver resolver(mIOService);

  Address4* addy = mSidMap->lookupInternal(server_id);
  struct in_addr ip_addr;
  ip_addr.s_addr = addy->ip;

  char* addr = inet_ntoa(ip_addr);
  char port_str[20];
  sprintf(port_str,"%d", (addy->port+10000));

  tcp::resolver::query query(tcp::v4(), addr, port_str);

  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

  tcp::resolver::iterator end;
  
  boost::shared_ptr<tcp::socket> socket = boost::shared_ptr<tcp::socket>(new tcp::socket (mIOService));
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && endpoint_iterator != end)
    {
      socket->close();
      socket->connect(*endpoint_iterator++, error);
    }

  if (error) {
    std::cout << "Error connecting to  " << addr << ":" << port_str
	      <<"\n";
    assert(false);
  } 

  
  //add the new socket to the list of leased sockets.
  SocketContainer socketContainer(socket);

  it = mLeasedSocketsToCSEGServers.find(server_id);

  if (it == mLeasedSocketsToCSEGServers.end()) {
    SocketQueuePtr socketList( new Sirikata::SizedThreadSafeQueue<SocketContainer>(Sirikata::SizedResourceMonitor(32)) );    
    mLeasedSocketsToCSEGServers[server_id] = socketList;
  }
  else {
    //check that adding this socket to the map doesn't break the size limit.
    assert(it->second->push(socketContainer, false));
    
    it->second->pop(socketContainer);
  }

  fflush(stdout);

  return socketContainer;
}

ServerID DistributedCoordinateSegmentation::callLowerLevelCSEGServer( ServerID server_id,
								      const Vector3f& searchVec,
								      const BoundingBox3f& boundingBox)
{
  SocketContainer socketContainer = getSocketToCSEGServer(server_id);
  
  SocketPtr socket= socketContainer.socket();

  if (socket.get() == 0) {
    assert(false);
    return 0;
  }

  SerializedVector serialVector;
  serialVector.serialize(searchVec);

  SerializedBBox serialBox;
  serialBox.serialize(boundingBox);

  uint16 dataSize = 1 + sizeof(serialVector) + sizeof(serialBox);
  uint8* buffer = new uint8[dataSize];
  buffer[0] = LL_LOOKUP_REQUEST;
  memcpy(buffer+1, &serialVector , sizeof(serialVector));
  memcpy(buffer+1+sizeof(serialVector), &serialBox, sizeof(serialBox));

  //Sending to server with ID 'server_id'

  boost::asio::write(*socket,
                     boost::asio::buffer((const void*)buffer,dataSize),
                     boost::asio::transfer_all() );

  uint8* dataReceived = NULL;
  uint32 bytesReceived = readFromSocket(socket, &dataReceived, false);

  //Received reply from 'server_id'

  ServerID retval = *(dataReceived+1);

  if (dataReceived != NULL) {
    free(dataReceived);
  }

  boost::upgrade_lock<boost::shared_mutex> lock(_access);
  mLeasedSocketsToCSEGServers[server_id]->push(socketContainer, false); 

  return retval;
}

void DistributedCoordinateSegmentation::callLowerLevelCSEGServersForServerRegions(
 									ServerID server_id,
									BoundingBoxList& bbList)
{
  std::map< ServerID, SocketContainer > socketList;
  bool sentToAll = false;

  for (uint32 i = 1; i <= (uint32)mAvailableCSEGServers; i++) {
    if (i == mContext->id()) continue;

    SocketContainer socketContainer = getSocketToCSEGServer(i);
    SocketPtr socket = socketContainer.socket();
    
    if (socket.get() == 0) {
      continue;
    }

    uint16 dataSize = 1 + sizeof(ServerID);
    uint8* buffer = new uint8[dataSize];
    buffer[0] = LL_SERVER_REGION_REQUEST;
    memcpy(buffer+1, &server_id , sizeof(ServerID));

    boost::asio::write(*socket,
		       boost::asio::buffer((const void*)buffer,dataSize),
		       boost::asio::transfer_all() );

    socketList[i] = socketContainer;
  }


  for (std::map<ServerID, SocketContainer>::iterator it = socketList.begin();
       it != socketList.end(); 
       it++)
  {
    
    uint8* dataReceived = NULL;
    boost::shared_ptr<tcp::socket> socket = it->second.socket();
    uint32 bytesReceived = readFromSocket(socket, &dataReceived, false);

    assert(bytesReceived >= 4);
    uint32 numBBoxesReturned;
    memcpy(&numBBoxesReturned, dataReceived+1, sizeof(uint32));

    assert( bytesReceived >= 4 + numBBoxesReturned * sizeof(SerializedBBox) );
    
    for (uint32 j=0; j < numBBoxesReturned; j++) {
      SerializedBBox serializedBbox;
      BoundingBox3f bbox;
      memcpy(&serializedBbox, dataReceived+1+sizeof(uint32)+sizeof(SerializedBBox)*j, sizeof(SerializedBBox));
      serializedBbox.deserialize(bbox);
      bbList.push_back(bbox);      
    }

    if (dataReceived != NULL) {
      free(dataReceived);
    }
  }

  boost::upgrade_lock<boost::shared_mutex> lock(_access);
  for (std::map<ServerID, SocketContainer>::iterator it = socketList.begin();
       it != socketList.end(); 
       it++)
  {
    mLeasedSocketsToCSEGServers[it->first]->push(it->second, false);
  }
}

void DistributedCoordinateSegmentation::sendLoadReportToLowerLevelCSEGServer(ServerID cseg_server_id, const Vector3f& searchVec, const BoundingBox3f& boundingBox, LoadReportMessage* message)
{
  SocketContainer socketContainer = getSocketToCSEGServer(cseg_server_id);
  SocketPtr socket = socketContainer.socket();

  if (socket.get() == 0) {
    assert(false);
  }

  SerializedBBox serialBox;
  serialBox.serialize(boundingBox);

  uint16 dataSize = 1 + sizeof(serialBox) + sizeof(LoadReportMessage);
  uint8* buffer = new uint8[dataSize];
  buffer[0] = LL_LOAD_REPORT;
  memcpy(buffer+1, &serialBox , sizeof(serialBox));
  memcpy( buffer+1+sizeof(serialBox), &message, sizeof(LoadReportMessage) );

  boost::asio::write(*socket,
                     boost::asio::buffer((const void*)buffer,dataSize),
                     boost::asio::transfer_all() );

  boost::system::error_code error;
  boost::array<uint8, 1> buf;

  socket->read_some(boost::asio::buffer(buf), error);
  
  boost::upgrade_lock<boost::shared_mutex> lock(_access);
  mLeasedSocketsToCSEGServers[cseg_server_id]->push(socketContainer, false);
}


void DistributedCoordinateSegmentation::sendToAllCSEGServers(uint8* buffer, int buflen) {
  /* Send to other CSEG servers so they can forward the segmentation change message to
     space servers connected to them. */
  for (int i = 1; i <= mAvailableCSEGServers; i++) {
    if ((ServerID)i == mContext->id()) continue;

    SocketContainer socketContainer = getSocketToCSEGServer(i);
    SocketPtr socket = socketContainer.socket();
    
    if (socket.get() == 0) {
      continue;
    }

    boost::asio::write(*socket,
		       boost::asio::buffer((void*) buffer, buflen),
		       boost::asio::transfer_all() );

    boost::upgrade_lock<boost::shared_mutex> lock(_access);
    mLeasedSocketsToCSEGServers[i]->push(socketContainer, false);
  }
}

void DistributedCoordinateSegmentation::sendToAllSpaceServers(uint8* buffer, int buflen) {
  tcp::resolver resolver(mIOService);

  for (std::vector<SegmentationChangeListener>::const_iterator it=mSpacePeers.begin();
       it!=mSpacePeers.end(); it++)
  {
    SegmentationChangeListener scl = *it;

    char* addr = scl.host;
    char port_str[20];
    sprintf(port_str,"%d", scl.port);

    tcp::resolver::query query(tcp::v4(), addr, port_str);

    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    tcp::resolver::iterator end;

    std::cout << "Calling " << addr << "@" << port_str << "!\n";
    tcp::socket socket(mIOService);
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
      {
	socket.close();
	socket.connect(*endpoint_iterator++, error);
      }
    if (error) {
      std::cout << "Connection refused to " << addr << ":"<<port_str <<"\n";
      continue;
    }

    boost::asio::write(socket,
		       boost::asio::buffer((void*) buffer, buflen),
		       boost::asio::transfer_all() );

    socket.close();
  }
}

bool DistributedCoordinateSegmentation::fullMessageReceived(uint8* dataReceived, uint32 bytesReceived){
  if (bytesReceived <= 0) {
    return false;
  }

  uint8 type = dataReceived[0];

  if ( mMessageSizes.find(type) == mMessageSizes.end()) {
    if (type != LL_SERVER_REGION_RESPONSE) {
      return false;
    }
    else {
      if (bytesReceived < 1 + sizeof(uint32)) {
        return false;
      }
      else {
        uint32 numBBoxesReturned = 0;
        memcpy(&numBBoxesReturned, dataReceived+1, sizeof(uint32));
        return (bytesReceived >= 1 + sizeof(uint32) + numBBoxesReturned * sizeof(SerializedBBox));
        
      }
    }
  }

  return ( bytesReceived >= mMessageSizes[type] );
}


} // namespace Sirikata
