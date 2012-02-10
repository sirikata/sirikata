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
#include <sirikata/core/network/IOStrandImpl.hpp>

#include <sirikata/core/network/ServerIDMap.hpp>

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#define snprintf _snprintf
#endif

namespace Sirikata {

template<typename T>
T clamp(T val, T minval, T maxval) {
  if (val < minval) return minval;
  if (val > maxval) return maxval;
  return val;
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
  }

  String retval = out;
  free(out);

  return retval;
}

static String sha1_bbox(const BoundingBox3f& bb) {
    char buf[24];
    *((float32*)buf) = bb.min().x;
    *((float32*)(buf+4)) = bb.min().y;
    *((float32*)(buf+8)) = bb.min().z;
    *((float32*)(buf+12)) = bb.max().x;
    *((float32*)(buf+16)) = bb.max().y;
    *((float32*)(buf+20)) = bb.max().z;
    return sha1(buf, 24);
}

/*
 Recursively constructs the kd-tree dividing the given region according to the
 split specified in 'perdim'. 'numServersAssigned' is just an integer to keep
 track of how many servers (initially equal to # of leaf regions) have been
 assigned.
*/
void DistributedCoordinateSegmentation::subdivideTopLevelRegion(SegmentedRegion* region,
								Vector3ui32 perdim,
								int& numServersAssigned)
{
  assert( isPowerOfTwo(perdim.x) );
  assert( isPowerOfTwo(perdim.y) );
  assert( isPowerOfTwo(perdim.z) );

  if (perdim.x == 1 && perdim.y == 1 && perdim.z == 1) {
    region->mServer =  (++numServersAssigned);   // FIXME: Set to 1 if only there is only one server
                                                 //and multiple regions need to be assigned.
    std::cout << "Server " << region->mServer << " assigned: " << region->mBoundingBox << "\n";
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

    region->mLeftChild->mSplitAxis = region->mRightChild->mSplitAxis = SegmentedRegion::X;

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

    region->mLeftChild->mSplitAxis = region->mRightChild->mSplitAxis = SegmentedRegion::Y;

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

    region->mLeftChild->mSplitAxis = region->mRightChild->mSplitAxis = SegmentedRegion::Z;

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

/*
  Callback which provides the port number at which this CSEG server is listening as specified in the
  filename given in "--cseg-servermap-options". CSEG starts listening for requests from other
  CSEG servers at that port number+10000. This function also starts off threads for the IOServices
  that handle requests from space servers and other CSEG servers.
*/
void DistributedCoordinateSegmentation::handleSelfLookup(Address4 my_addr) {
  uint16 cseg_server_ll_port = my_addr.getPort()+10000;

  std::cout << "Listening on  LL: " << cseg_server_ll_port << "\n";

  mLLTreeAcceptor = boost::shared_ptr<tcp::acceptor>(new tcp::acceptor(mLLIOService,tcp::endpoint(tcp::v4(), cseg_server_ll_port)));

  startAcceptingLLRequests();

  Thread thrd(boost::bind(&DistributedCoordinateSegmentation::ioServicingLoop, this));
  Thread thrd2(boost::bind(&DistributedCoordinateSegmentation::llIOServicingLoop, this));
}


DistributedCoordinateSegmentation::DistributedCoordinateSegmentation(CSegContext* ctx, const BoundingBox3f& region,
                                                                     const Vector3ui32& perdim, int nservers, ServerIDMap * sidmap)
 : PollingService(ctx->mainStrand, "DistributedCoordinateSegmentation Poll", Duration::milliseconds((int64)1000)),
   mContext(ctx),
   mTopLevelRegion(NULL),
   mLastUpdateTime(Time::null()),
   mLoadBalancer(this, nservers, perdim),
   mAvailableCSEGServers(GetOptionValue<uint16>("num-cseg-servers")),
   mUpperTreeCSEGServers(GetOptionValue<uint16>("num-upper-tree-cseg-servers")),
   mSidMap(sidmap)
{
  std::cout << mAvailableCSEGServers << " : " << mUpperTreeCSEGServers  << "\n";

  assert(mAvailableCSEGServers >= mUpperTreeCSEGServers);

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

  /* Upper tree servers: start listening for requests! */
  if ((int)ctx->id() <= mUpperTreeCSEGServers) {
      mAcceptor = boost::shared_ptr<tcp::acceptor>(
                    new tcp::acceptor(mIOService,
                      tcp::endpoint(tcp::v4(), atoi( GetOptionValue<String>("cseg-service-tcp-port").c_str() ))));
    startAccepting();
  }

  /* Lower tree servers will start listening once they know the port on which to listen */
  sidmap->lookupInternal(ctx->id(),
                         ctx->mainStrand->wrap(
                         std::tr1::bind(&DistributedCoordinateSegmentation::handleSelfLookup, this,
                                        std::tr1::placeholders::_1)
      ) );
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

/**
 Look up server IDs that cover the bounding region specified by bbox. The resulting list
 will eventually be written to the socket specified.
*/
bool DistributedCoordinateSegmentation::handleLookupBBox(const BoundingBox3f& bbox, boost::shared_ptr<tcp::socket> clientSocket) {
  std::vector<ServerID> serverList;

  std::vector<SegmentedRegion*> segmentedRegionsList;
  mTopLevelRegion.lookupBoundingBox(bbox, segmentedRegionsList);

  std::map<ServerID, std::vector<SegmentedRegion*> > otherCSEGServers;

  for (uint32 i = 0; i < segmentedRegionsList.size(); i++) {
    SegmentedRegion* segRegion = segmentedRegionsList[i];
    ServerID topLevelIdx = segRegion->mServer;

    if (topLevelIdx == mContext->id()) {
      //find servers locally with intersecting bounding boxes
      ServerID sid = 0;

      String bbox_hash = sha1_bbox(segRegion->mBoundingBox);

      std::map<String, SegmentedRegion*>::iterator it=  mHigherLevelTrees.find(bbox_hash);

      std::vector<SegmentedRegion*> regionList;

      if (it != mHigherLevelTrees.end()) {
        (*it).second->lookupBoundingBox(bbox, regionList);
      }
      else {
        it =  mLowerLevelTrees.find(bbox_hash);

        if (it != mLowerLevelTrees.end()) {
          (*it).second->lookupBoundingBox(bbox, regionList);
        }
      }

      for (uint32 j = 0; j < regionList.size(); j++) {
        serverList.push_back(regionList[j]->mServer);
      }
    }
    else {
      //add to list of CSEG servers that need to be remotely invoked.
      if (otherCSEGServers.find(topLevelIdx) ==otherCSEGServers.end()) {
        std::vector<SegmentedRegion*> v;
        v.push_back(segRegion);
        otherCSEGServers[topLevelIdx] = v;
      }
      else {
        otherCSEGServers[topLevelIdx].push_back(segRegion);
      }
    }
  }

  if (otherCSEGServers.size() == 0) {
    writeLookupBBoxResponse(clientSocket, serverList);
    return true;
  }

  //call lookupBoundingBox on the other CSeg servers.
  callLowerLevelCSEGServersForLookupBoundingBoxes(clientSocket, bbox, otherCSEGServers, serverList);

  return false ;
}

void DistributedCoordinateSegmentation::writeServerRegionResponse( boost::shared_ptr<tcp::socket> socket,
                                                                  BoundingBoxList boundingBoxList)
{
  if (!socket) {
    return;
  }

  Sirikata::Protocol::CSeg::CSegMessage csegResponseMessage;
  for (uint32 i=0; i < boundingBoxList.size() ; i++) {
    BoundingBox3f bbox = boundingBoxList[i];

    BoundingBox3d3f bboxd = bbox;
    csegResponseMessage.mutable_server_region_response_message().add_bbox_list(bboxd);
  }

  writeCSEGMessage(socket, csegResponseMessage);

  uint8* asyncBufferArray = new uint8[1];
  socket->async_read_some( boost::asio::buffer(asyncBufferArray, 1),
         std::tr1::bind(&DistributedCoordinateSegmentation::asyncRead, this,
                           socket, asyncBufferArray, _1, _2)  );
}

void DistributedCoordinateSegmentation::writeLookupResponse(boost::shared_ptr<tcp::socket> socket, BoundingBox3f& bbox, ServerID sid) {
    if (!socket) {
      return;
    }

    Sirikata::Protocol::CSeg::CSegMessage csegResponseMessage;

    csegResponseMessage.mutable_lookup_response_message().set_server_id(sid);
    csegResponseMessage.mutable_lookup_response_message().set_server_bbox(bbox);

    writeCSEGMessage(socket, csegResponseMessage);
}

void DistributedCoordinateSegmentation::writeLookupBBoxResponse(boost::shared_ptr<tcp::socket> clientSocket, std::vector<ServerID> serverList)
{
   if (!clientSocket) {
     return;
   }

   //remove duplicate Space Server IDs from serverList
    serverList.erase(std::unique(serverList.begin(), serverList.end()),
                   serverList.end());

    Sirikata::Protocol::CSeg::CSegMessage csegResponseMessage;
    for (uint32 i=0; i < serverList.size() ; i++) {
      csegResponseMessage.mutable_lookup_bbox_response_message().add_server_list(serverList[i]);
    }

    writeCSEGMessage(clientSocket, csegResponseMessage);
}


bool DistributedCoordinateSegmentation::handleLookup(Vector3f pos, boost::shared_ptr<tcp::socket> socket) {
  ServerID sid;
  BoundingBox3f server_bbox;

  Vector3f searchVec = pos;
  BoundingBox3f region = mTopLevelRegion.mBoundingBox;

  int i=0;

  (searchVec.z > region.max().z) ? searchVec.z = region.max().z : (i=0);
  (searchVec.x > region.max().x) ? searchVec.x = region.max().x : (i=0);
  (searchVec.y > region.max().y) ? searchVec.y = region.max().y : (i=0);

  (searchVec.z < region.min().z) ? searchVec.z = region.min().z : (i=0);
  (searchVec.x < region.min().x) ? searchVec.x = region.min().x : (i=0);
  (searchVec.y < region.min().y) ? searchVec.y = region.min().y : (i=0);

  const SegmentedRegion* segRegion = mTopLevelRegion.lookup(searchVec);
  ServerID topLevelIdx = segRegion->mServer;

  //std::cout << "Returned remote CSEG: " << topLevelIdx << " for vector " << pos <<"\n";

  if (topLevelIdx == mContext->id())
  {
    String bbox_hash = sha1_bbox(segRegion->mBoundingBox);

    std::map<String, SegmentedRegion*>::iterator it=  mHigherLevelTrees.find(bbox_hash);

    if (it != mHigherLevelTrees.end()) {
      segRegion = (*it).second->lookup(searchVec);

      //printf("local lookup returned %d\n", segRegion->mServer);

      sid = segRegion->mServer;

      server_bbox = segRegion->mBoundingBox;
    }
    else {
      it =  mLowerLevelTrees.find(bbox_hash);

      if (it != mLowerLevelTrees.end()) {
        segRegion = (*it).second->lookup(searchVec);

        //printf("local lookup returned %d\n", segRegion->mServer);

        sid = segRegion->mServer;

        server_bbox = segRegion->mBoundingBox;
      }
      else {
          sid = 0;
      }
    }
  }
  else {
    //remote function call to the relevant server
    callLowerLevelCSEGServer(socket, topLevelIdx, searchVec, segRegion->mBoundingBox, server_bbox);
    return false;
  }

  writeLookupResponse(socket, server_bbox, sid);
  return true;
}

/* Gets the server region associated with ServerID 'server' and eventually writes the result
   to the socket specified as the second argument. If the socket is NULL, it does everything
   exactly the same except it doesnt write the result to the socket. */
void DistributedCoordinateSegmentation::getServerRegionUncached(const ServerID& server,
                                                      boost::shared_ptr<tcp::socket> socket)
{
  //FIXME: Deal with caching properly...
  if (mWholeTreeServerRegionMap.find(server) != mWholeTreeServerRegionMap.end()) {
    writeServerRegionResponse(socket, mWholeTreeServerRegionMap[server]);
    return;
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

  callLowerLevelCSEGServersForServerRegions(socket, server, boundingBoxList);
}

/*
 Return locally cached information about what bounding boxes are covered by 'server'
*/
BoundingBoxList DistributedCoordinateSegmentation::serverRegionCached(const ServerID& server)
{
  if (mWholeTreeServerRegionMap.find(server) != mWholeTreeServerRegionMap.end()) {
    return mWholeTreeServerRegionMap[server];
  }

  return BoundingBoxList();
}

BoundingBox3f DistributedCoordinateSegmentation::region()  {
  return mTopLevelRegion.mBoundingBox;
}

uint32 DistributedCoordinateSegmentation::numServers() {
  return mLoadBalancer.numAvailableServers();

  //int count = mTopLevelRegion.countServers();
  //return count;
}

void DistributedCoordinateSegmentation::handleLoadReport(boost::shared_ptr<tcp::socket> socket,
                                                         Sirikata::Protocol::CSeg::LoadReportMessage* message)
{
  ServerID sid = message->server();
  BoundingBox3f bbox = message->bbox();

  Vector3f searchVec = Vector3f( (bbox.min().x+bbox.max().x)/2.0, (bbox.min().y+bbox.max().y)/2,
                                 (bbox.min().z+bbox.max().z)/2.0 );

  SegmentedRegion* segRegion = mTopLevelRegion.lookup(searchVec);
  ServerID topLevelIdx = segRegion->mServer;

  if (topLevelIdx == mContext->id())
  {
    String bbox_hash = sha1_bbox(segRegion->mBoundingBox);

    std::map<String, SegmentedRegion*>::iterator it=  mHigherLevelTrees.find(bbox_hash);

    if (it != mHigherLevelTrees.end()) {

      segRegion = (*it).second->lookup(searchVec);

      // deal with the value for this region's load;
      if (sid == segRegion->mServer && bbox == segRegion->mBoundingBox) {
        segRegion->mLoadValue = message->load_value();

        mLoadBalancer.reportRegionLoad(segRegion, sid, segRegion->mLoadValue);
      }
    }
    else {
      it =  mLowerLevelTrees.find(bbox_hash);

      if (it != mLowerLevelTrees.end()) {
        segRegion = (*it).second->lookup(searchVec);

        // deal with the value for this region's load.
        if (sid == segRegion->mServer && bbox == segRegion->mBoundingBox) {
          segRegion->mLoadValue = message->load_value();
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
    sendLoadReportToLowerLevelCSEGServer(socket, topLevelIdx, segRegion->mBoundingBox, message);
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
  Sirikata::Protocol::CSeg::CSegMessage csegMessage;

  int count = 0;

  /* Fill in the fields in the message */
  for (unsigned int i=0; i<segInfoVector.size(); i++) {
    ServerID serverID= segInfoVector[i].server;

    for (unsigned int j=0; j<segInfoVector[i].region.size(); j++) {
      csegMessage.mutable_change_message().add_region();
      csegMessage.mutable_change_message().mutable_region(count).set_id(serverID);
      csegMessage.mutable_change_message().mutable_region(count).set_bounds(segInfoVector[i].region[j]);

      count++;
    }
  }

  /* Send to CSEG servers connected to this server.  */
  sendToAllCSEGServers(csegMessage);

  /* Send to space servers connected to this server.  */
  sendToAllSpaceServers(csegMessage);

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

void DistributedCoordinateSegmentation::accept_handler()
{
  uint8* asyncBufferArray = new uint8[1];

  mSocket->async_read_some( boost::asio::buffer(asyncBufferArray, 1),
			    std::tr1::bind(&DistributedCoordinateSegmentation::asyncRead, this,
					   mSocket, asyncBufferArray, _1, _2)  );

  startAccepting();
}

/*
 Handle requests from space servers. asyncLLRead handles requests from other CSEG servers.
*/
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

  Sirikata::Protocol::CSeg::CSegMessage csegMessage;
  Sirikata::Protocol::CSeg::CSegMessage csegResponseMessage;

  readCSEGMessage(socket, csegMessage, asyncBufferArray, 1); //at least one byte is guaranteed to be read

   /* Deal with the request included in the received data */

  boost::shared_lock<boost::shared_mutex> mCSEGExclusiveWriteLock(mCSEGReadWriteMutex);

  if (csegMessage.has_lookup_request_message()) {
    std::cout << "Handling lookup request message\n";

    bool responseWritten = handleLookup(Vector3f(csegMessage.lookup_request_message().x(),
                          csegMessage.lookup_request_message().y(),
                          csegMessage.lookup_request_message().z()
                          ),
                        socket);

    if (!responseWritten) {
      delete asyncBufferArray;

      return;
    }
  }
  else if (csegMessage.has_num_servers_request_message()) {
    csegResponseMessage.mutable_num_servers_response_message().set_num_servers(numServers());

    writeCSEGMessage(socket, csegResponseMessage);
  }
  else if (csegMessage.has_region_request_message()) {
    csegResponseMessage.mutable_region_response_message().set_bbox(region());

    writeCSEGMessage(socket, csegResponseMessage);
  }
  else if (csegMessage.has_server_region_request_message()) {
    getServerRegionUncached(csegMessage.server_region_request_message().server_id(), socket);

    delete asyncBufferArray;

    return;
  }
  else if (csegMessage.has_segmentation_listen_message()) {
    SegmentationChangeListener sl;
    memcpy(sl.host, csegMessage.segmentation_listen_message().host().c_str(), 255);
    sl.port = csegMessage.segmentation_listen_message().port();

    std::cout << "Listening: " << sl.host << " : " << sl.port <<"\n";
    mSpacePeers.push_back(sl);
  }
  else if (csegMessage.has_load_report_message()) {
    Sirikata::Protocol::CSeg::LoadReportMessage message = csegMessage.load_report_message();

    handleLoadReport(socket, &message );

    delete asyncBufferArray;

    return;
  }
  else if (csegMessage.has_lookup_bbox_request_message()) {
    bool responseWritten = handleLookupBBox(csegMessage.lookup_bbox_request_message().bbox(), socket);

    if (!responseWritten) {
      delete asyncBufferArray;

      return;
    }
  }

  socket->async_read_some( boost::asio::buffer(asyncBufferArray, 1),
			   std::tr1::bind(&DistributedCoordinateSegmentation::asyncRead, this,
                           socket, asyncBufferArray, _1, _2)  );
}

/*
 Handle requests from other CSEG servers. asyncRead handles requests from other space servers.
*/
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

  Sirikata::Protocol::CSeg::CSegMessage csegMessage;
  Sirikata::Protocol::CSeg::CSegMessage csegResponseMessage;
  readCSEGMessage(socket, csegMessage, asyncBufferArray, 1);

  boost::shared_lock<boost::shared_mutex> mCSEGExclusiveWriteLock(mCSEGReadWriteMutex);

  if ( csegMessage.has_ll_lookup_request_message() )   {
    BoundingBox3f bbox = csegMessage.ll_lookup_request_message().bbox();
    Vector3f vect = csegMessage.ll_lookup_request_message().lookup_vector();

    String bbox_hash = sha1_bbox(bbox);
    std::map<String, SegmentedRegion*>::iterator it=  mLowerLevelTrees.find(bbox_hash);

    ServerID retval = 0;
    BoundingBox3f leaf_bbox;
    if (it != mLowerLevelTrees.end()) {
      const SegmentedRegion*segRegion = (*it).second->lookup(vect);

      retval = segRegion->mServer;
      leaf_bbox = segRegion->mBoundingBox;
    }

    csegResponseMessage.mutable_ll_lookup_response_message().set_server_id(retval);
    csegResponseMessage.mutable_ll_lookup_response_message().set_leaf_bbox(leaf_bbox);

    writeCSEGMessage(socket, csegResponseMessage);
  }
  else if (csegMessage.has_ll_server_region_request_message() ) {
    ServerID serverID = csegMessage.ll_server_region_request_message().server_id();
    BoundingBoxList boundingBoxList;

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

    for (uint32 i = 0; i < boundingBoxList.size(); i++) {
      csegResponseMessage.mutable_ll_server_region_response_message().add_bboxes(boundingBoxList[i]);
    }

    writeCSEGMessage(socket, csegResponseMessage);
  }
  else if (csegMessage.has_change_message() ) {
    std::cout << "csegChangeMessage received\n";

    mWholeTreeServerRegionMap.clear();

    mLoadBalancer.handleSegmentationChange( csegMessage.change_message() );

    sendToAllSpaceServers(csegMessage);
  }
  else if (csegMessage.has_ll_load_report_message() ) {
    std::cout << "LL Load report\n";
    BoundingBox3f lowerTreeRootBox = csegMessage.ll_load_report_message().lower_root_box();
    String bbox_hash = sha1_bbox(lowerTreeRootBox);
    std::map<String, SegmentedRegion*>::iterator it=  mLowerLevelTrees.find(bbox_hash);

    BoundingBox3f leafBBox = csegMessage.ll_load_report_message().load_report_message().bbox();
    Vector3f vect( (leafBBox.min().x+leafBBox.max().x)/2.0, (leafBBox.min().y+leafBBox.max().y)/2,
                   (leafBBox.min().z+leafBBox.max().z)/2.0 );

    if (it != mLowerLevelTrees.end()) {
      SegmentedRegion* segRegion = (*it).second->lookup(vect);

      if (segRegion->mServer == csegMessage.ll_load_report_message().load_report_message().server()
          && segRegion->mBoundingBox == leafBBox)
      {
        //deal with the load from the space server
        segRegion->mLoadValue = csegMessage.ll_load_report_message().load_report_message().load_value();

        mLoadBalancer.reportRegionLoad(segRegion, segRegion->mServer, segRegion->mLoadValue);
      }
    }
    else {
      assert(false);
    }

    csegResponseMessage.mutable_load_report_ack_message().set_ack(1);
    writeCSEGMessage(socket, csegResponseMessage);
  }
  else if (csegMessage.has_ll_lookup_bbox_request_message()) {
    SerializedBBox serialBox;
    BoundingBox3f bbox = csegMessage.ll_lookup_bbox_request_message().bbox();
    serialBox.serialize(bbox);

    std::vector<ServerID> serverList;
    for (int i=0; i < csegMessage.ll_lookup_bbox_request_message().candidate_boxes_size(); i++) {
      BoundingBox3f candidateBbox = csegMessage.ll_lookup_bbox_request_message().candidate_boxes(i);
      String bbox_hash = sha1_bbox(candidateBbox);

      //do the lookups
      std::map<String, SegmentedRegion*>::iterator it=  mLowerLevelTrees.find(bbox_hash);
      if (it != mLowerLevelTrees.end()) {
        std::vector<SegmentedRegion*> vect;
        (*it).second->lookupBoundingBox(bbox, vect);

        for (uint32 j = 0; j < vect.size(); j++) {
          serverList.push_back(vect[j]->mServer);
        }
      }
    }

    //eliminate unique elements
    serverList.erase(std::unique(serverList.begin(), serverList.end()),
                   serverList.end());

    for (uint32 i = 0; i < serverList.size(); i++) {
      ServerID server_id = serverList[i];
      csegResponseMessage.mutable_ll_lookup_bbox_response_message().add_server_id_list(server_id);
    }

    writeCSEGMessage(socket, csegResponseMessage);
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

  mLLTreeAcceptorSocket->async_read_some( boost::asio::buffer(asyncBufferArray, 1),
          std::tr1::bind(&DistributedCoordinateSegmentation::asyncLLRead, this,
             mLLTreeAcceptorSocket, asyncBufferArray, _1, _2)  );

  startAcceptingLLRequests();
}

void DistributedCoordinateSegmentation::generateHierarchicalTrees(SegmentedRegion* region, int depth, int& numLLTreesSoFar) {
  int cutOffDepth = 2;

  if (depth>=cutOffDepth) {
    if (mAvailableCSEGServers > mUpperTreeCSEGServers) {
      //if we have lower-tree servers available, then the lower-trees
      //should be divided amongst them.

      region->mServer = 1+(numLLTreesSoFar % (mAvailableCSEGServers-mUpperTreeCSEGServers))+mUpperTreeCSEGServers;

      std::cout << "region->mServer: "<< region->mServer << "\n";
    }
    else {
      //if we only have upper-tree servers available, then they should
      //all keep a copy of the whole tree.
      region->mServer = mContext->id();
    }

    if ( region->mServer == mContext->id()) {
      // Assigned region->mBoundingBox to me.
      SegmentedRegion* segRegion = new SegmentedRegion(NULL);
      segRegion->mBoundingBox = region->mBoundingBox;
      segRegion->mLeftChild = region->mLeftChild;
      segRegion->mRightChild = region->mRightChild;

      assert(region->mParent == NULL || region->mSplitAxis != SegmentedRegion::UNDEFINED);

      segRegion->mSplitAxis = region->mSplitAxis;

      if (region->mLeftChild == NULL && region->mRightChild == NULL) {
        segRegion->mServer = region->mServer;
      }

      String bbox_hash = sha1_bbox(segRegion->mBoundingBox);

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

    assert(region->mParent == NULL || region->mSplitAxis != SegmentedRegion::UNDEFINED);
    segRegion->mSplitAxis = region->mSplitAxis;

    String bbox_hash = sha1_bbox(segRegion->mBoundingBox);
    mHigherLevelTrees[bbox_hash] = segRegion;
    return;
  }

  generateHierarchicalTrees(region->mLeftChild, depth+1, numLLTreesSoFar);
  generateHierarchicalTrees(region->mRightChild, depth+1, numLLTreesSoFar);
}

SocketContainer DistributedCoordinateSegmentation::getSocketToCSEGServer(ServerID server_id) {
  // get upgradable access
  boost::upgrade_lock<boost::shared_mutex> lock(mSocketsToCSEGServersMutex);

  std::map<ServerID, SocketQueuePtr >::iterator it = mLeasedSocketsToCSEGServers.find(server_id);
  if (it != mLeasedSocketsToCSEGServers.end()) {
    SocketContainer socket;

    bool retval = it->second->pop(socket);

    if (retval) {
      return socket;
    }
  }

  return SocketContainer();
}

/*
  Callback invoked when a ServerMap request completes so that a new socket can be created.
  The address 'addy' corresponding to server 'server_id' is passed to the callback.
  server_id_list contains the list of CSEG Server IDs to which new sockets need to be created. socketMapPtr
  contains sockets to ALL CSEG servers that will have to be eventually contacted,
  whether sockets to them already exist or need to be created. When sockets to all
  servers specified in server_id_list have been created, the function func() is called.
 */
void DistributedCoordinateSegmentation::doSocketCreation(ServerID server_id,
                                                         std::vector<ServerID> server_id_list,
                                                         std::map<ServerID, SocketContainer>* socketMapPtr,
                                                         ResponseCompletionFunction func,
                                                         Address4 addy
                                                         )
{
  std::map<ServerID, SocketContainer>& socketContainerMap = *socketMapPtr;

  struct in_addr ip_addr;
  ip_addr.s_addr = addy.ip;

  char* addr = inet_ntoa(ip_addr);
  char port_str[20];
  sprintf(port_str,"%d", (addy.port+10000));

  tcp::resolver resolver(mIOService);

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

  std::map<ServerID, SocketQueuePtr >::iterator it = mLeasedSocketsToCSEGServers.find(server_id);

  if (it == mLeasedSocketsToCSEGServers.end()) {
    SocketQueuePtr socketList( new Sirikata::SizedThreadSafeQueue<SocketContainer>(Sirikata::SizedResourceMonitor(32)) );
    mLeasedSocketsToCSEGServers[server_id] = socketList;
  }
  else {
    //check that adding this socket to the map doesn't break the size limit.
    assert(it->second->push(socketContainer, false));

    it->second->pop(socketContainer);
  }

  socketContainerMap[server_id] = socketContainer;

  bool allSocketsCreated = true;

  for (uint32 i=0 ; i < server_id_list.size() && allSocketsCreated; i++) {
    if (socketContainerMap.find(server_id_list[i]) == socketContainerMap.end()) {
      allSocketsCreated = false;
    }
  }

  // Call func() if all sockets have been created or retrieved.
  if (allSocketsCreated) {
    func(socketContainerMap);

    delete  socketMapPtr;
  }

}

/*
  Create sockets to CSEG servers that need to be contacted to complete
  a request. When all sockets have been created, call func(). server_id_list
  contains the list of CSEG Server IDs to which new sockets need to be created. socketMapPtr
  contains sockets to ALL CSEG servers that will have to be eventually contacted,
  whether sockets to them already exist or need to be created.
 */
void DistributedCoordinateSegmentation::createSocketContainers(std::vector<ServerID> server_id_list,
                                                               std::map<ServerID, SocketContainer>* socketMap,
                                                               ResponseCompletionFunction func
                                                               )
{
  if (server_id_list.size() == 0) {
    func(*socketMap);
  }

  for (uint32 i = 0; i < server_id_list.size(); i++) {
    ServerID server_id = server_id_list[i];

    mSidMap->lookupInternal(server_id,
                            mContext->mainStrand->wrap(
                               std::tr1::bind(&DistributedCoordinateSegmentation::doSocketCreation, this,
                               server_id, server_id_list, socketMap, func, std::tr1::placeholders::_1)
                          ));

  }
}

void DistributedCoordinateSegmentation::callLowerLevelCSEGServersForLookupBoundingBoxes(
                            boost::shared_ptr<tcp::socket> socket,
                            const BoundingBox3f& lookedUpBbox,
                            const std::map<ServerID, std::vector<SegmentedRegion*> >& csegServers,
                            std::vector<ServerID>& spaceServers)
{
  std::map< ServerID, SocketContainer >* socketList = new std::map<ServerID, SocketContainer>();

  std::vector<ServerID> ids_without_sockets;

  /* Try to get previously created sockets to each of the CSEG servers in csegServers.
   */
  for (std::map<ServerID, std::vector<SegmentedRegion*> >::const_iterator it = csegServers.begin();
       it != csegServers.end(); it++)
  {
    ServerID i = it->first;

    SocketContainer socketContainer = getSocketToCSEGServer(i);
    SocketPtr socket = socketContainer.socket();

    if (!socket || socket.get() == 0) {
      ids_without_sockets.push_back(i);
      continue;
    }

    (*socketList)[i] = socketContainer;
  }

  /*
    For CSEG servers to which sockets dont already exist, create new sockets.
   */
  createSocketContainers(ids_without_sockets, socketList,
                        std::tr1::bind(&DistributedCoordinateSegmentation::lookupBBoxOnSocket, this,
                        socket, lookedUpBbox, spaceServers, csegServers, _1));

}

void DistributedCoordinateSegmentation::callLowerLevelCSEGServer( boost::shared_ptr<tcp::socket> clientSocket,
                                                                  ServerID server_id,
                                                                  const Vector3f& searchVec,
                                                                  const BoundingBox3f& boundingBox,
                                                                  BoundingBox3f& leafBBox)
{
  std::map< ServerID, SocketContainer >* socketList = new std::map<ServerID, SocketContainer>();

  std::vector<ServerID> ids_without_sockets;

  if (server_id == mContext->id()) return;

  SocketContainer socketContainer = getSocketToCSEGServer(server_id);
  SocketPtr socket = socketContainer.socket();

  if (!socket || socket.get() == 0) {
      ids_without_sockets.push_back(server_id);
  }

  (*socketList)[server_id] = socketContainer;

  createSocketContainers(ids_without_sockets, socketList,
                         std::tr1::bind(&DistributedCoordinateSegmentation::lookupOnSocket,this,
                                        clientSocket, searchVec,  boundingBox, _1));
}

void DistributedCoordinateSegmentation::lookupBBoxOnSocket(boost::shared_ptr<tcp::socket> clientSocket,
               const BoundingBox3f boundingBox, std::vector<ServerID> server_ids,
               std::map<ServerID, std::vector<SegmentedRegion*> > otherCSEGServers,
               std::map<ServerID, SocketContainer> socketList)
{
  //send the request to other CSEG servers.
  for (std::map<ServerID, SocketContainer >::iterator it = socketList.begin();
       it != socketList.end(); it++
      )
  {
    ServerID server_id = it->first;

    SocketContainer& socketContainer = it->second;
    SocketPtr socket = socketContainer.socket();

    if (socket.get() == 0) {
      continue;
    }

    Sirikata::Protocol::CSeg::CSegMessage csegMessage;
    csegMessage.mutable_ll_lookup_bbox_request_message().set_bbox(boundingBox);

    std::vector<SegmentedRegion*>& segRegionList = otherCSEGServers[server_id];
    for (uint32 j = 0; j < segRegionList.size(); j++) {
      csegMessage.mutable_ll_lookup_bbox_request_message().add_candidate_boxes(segRegionList[j]->mBoundingBox);
    }

    writeCSEGMessage(socket, csegMessage);
  }

  //collect the responses...
  for (std::map<ServerID, SocketContainer>::iterator it = socketList.begin();
       it != socketList.end();
       it++)
  {
    SocketPtr socket = it->second.socket();

    Sirikata::Protocol::CSeg::CSegMessage csegMessage;
    readCSEGMessage(socket, csegMessage);

    for (int32 j=0; j < csegMessage.ll_lookup_bbox_response_message().server_id_list_size(); j++) {
      ServerID server_id = csegMessage.ll_lookup_bbox_response_message().server_id_list(j);

      server_ids.push_back(server_id);
    }
  }

  //release the sockets to the socket pool.
  boost::upgrade_lock<boost::shared_mutex> lock(mSocketsToCSEGServersMutex);
  for (std::map<ServerID, SocketContainer>::iterator it = socketList.begin();
       it != socketList.end();
       it++)
  {
    mLeasedSocketsToCSEGServers[it->first]->push(it->second, false);
  }

  writeLookupBBoxResponse(clientSocket, server_ids);

  uint8* asyncBufferArray = new uint8[1];
  clientSocket->async_read_some( boost::asio::buffer(asyncBufferArray, 1),
         std::tr1::bind(&DistributedCoordinateSegmentation::asyncRead, this,
                           clientSocket, asyncBufferArray, _1, _2)  );

}


void DistributedCoordinateSegmentation::lookupOnSocket(boost::shared_ptr<tcp::socket> clientSocket, const Vector3f searchVec,
                                                       const BoundingBox3f boundingBox, std::map<ServerID, SocketContainer> socketList)
{
  assert(socketList.size() > 0);

  ServerID cseg_server_id = socketList.begin()->first;
  SocketContainer socketContainer = socketList.begin()->second;

  SocketPtr socket= socketContainer.socket();

  if (socket.get() == 0) {
    assert(false);
    return;
  }

  Sirikata::Protocol::CSeg::CSegMessage csegMessage;
  csegMessage.mutable_ll_lookup_request_message().set_lookup_vector(searchVec);
  csegMessage.mutable_ll_lookup_request_message().set_bbox(boundingBox);


  //Sending to server with ID 'server_id'
  writeCSEGMessage(socket, csegMessage);

  //Read reply from 'server_id'
  readCSEGMessage(socket, csegMessage);

  ServerID server_id = csegMessage.ll_lookup_response_message().server_id();

  BoundingBox3f leafBBox;
  if (csegMessage.ll_lookup_response_message().has_leaf_bbox()) {
    leafBBox = csegMessage.ll_lookup_response_message().leaf_bbox();
  }

  boost::upgrade_lock<boost::shared_mutex> lock(mSocketsToCSEGServersMutex);

  mLeasedSocketsToCSEGServers[cseg_server_id]->push(socketContainer, false);

  writeLookupResponse(clientSocket, leafBBox, server_id);

  //Start listening on the socket for further requests.
  uint8* asyncBufferArray = new uint8[1];
  clientSocket->async_read_some( boost::asio::buffer(asyncBufferArray, 1),
         std::tr1::bind(&DistributedCoordinateSegmentation::asyncRead, this,
                           clientSocket, asyncBufferArray, _1, _2)  );

}

void DistributedCoordinateSegmentation::requestServerRegionsOnSockets(boost::shared_ptr<tcp::socket> socket, ServerID server_id,  BoundingBoxList bbList,  std::map<ServerID, SocketContainer> socketList)
{

  for (std::map<ServerID, SocketContainer>::iterator it = socketList.begin();
       it != socketList.end();
       it++)
  {
    Sirikata::Protocol::CSeg::CSegMessage csegMessage;
    csegMessage.mutable_ll_server_region_request_message().set_server_id(server_id);
    writeCSEGMessage(it->second.socket(), csegMessage);
  }

  for (std::map<ServerID, SocketContainer>::iterator it = socketList.begin();
       it != socketList.end();
       it++)
  {
    uint8* dataReceived = NULL;
    boost::shared_ptr<tcp::socket> sock = it->second.socket();

    Sirikata::Protocol::CSeg::CSegMessage csegMessage;
    readCSEGMessage(sock, csegMessage);

    for (int j=0; j < csegMessage.ll_server_region_response_message().bboxes_size(); j++) {
      BoundingBox3f bbox = csegMessage.ll_server_region_response_message().bboxes(j);

      bbList.push_back(bbox);
    }
  }

  boost::upgrade_lock<boost::shared_mutex> lock(mSocketsToCSEGServersMutex);
  for (std::map<ServerID, SocketContainer>::iterator it = socketList.begin();
       it != socketList.end();
       it++)
  {
    mLeasedSocketsToCSEGServers[it->first]->push(it->second, false);
  }

  if (bbList.size() > 0) {
    mWholeTreeServerRegionMap[server_id] = bbList;
  }
  else if (bbList.size() == 0) {
    bbList.push_back(BoundingBox3f(Vector3f(0,0,0), Vector3f(0,0,0)));
    mWholeTreeServerRegionMap[server_id] = bbList;
  }

  writeServerRegionResponse(socket, bbList);
}

void DistributedCoordinateSegmentation::callLowerLevelCSEGServersForServerRegions(
                                                                        boost::shared_ptr<tcp::socket> socket,
 									ServerID server_id,
									BoundingBoxList& bbList)
{
  std::map< ServerID, SocketContainer >* socketList = new std::map<ServerID, SocketContainer>();

  std::vector<ServerID> ids_without_sockets;

  for (uint32 i = mUpperTreeCSEGServers+1; i <= (uint32)mAvailableCSEGServers; i++) {
    if (i == mContext->id()) continue;

    SocketContainer socketContainer = getSocketToCSEGServer(i);
    SocketPtr socket = socketContainer.socket();

    if (!socket || socket.get() == 0) {
      ids_without_sockets.push_back(i);
      continue;
    }

    (*socketList)[i] = socketContainer;
  }

  createSocketContainers(ids_without_sockets, socketList,
                         std::tr1::bind(&DistributedCoordinateSegmentation::requestServerRegionsOnSockets,this,
                                        socket, server_id, bbList, _1));
}

void DistributedCoordinateSegmentation::sendLoadReportToLowerLevelCSEGServer(boost::shared_ptr<tcp::socket> clientSocket, ServerID cseg_server_id, const BoundingBox3f& boundingBox, Sirikata::Protocol::CSeg::LoadReportMessage* message)
{
  std::map< ServerID, SocketContainer >* socketList = new std::map<ServerID, SocketContainer>();

  std::vector<ServerID> ids_without_sockets;

  if (cseg_server_id == mContext->id()) return;

  SocketContainer socketContainer = getSocketToCSEGServer(cseg_server_id);
  SocketPtr socket = socketContainer.socket();

  if (!socket || socket.get() == 0) {
      ids_without_sockets.push_back(cseg_server_id);
  }

  (*socketList)[cseg_server_id] = socketContainer;
  createSocketContainers(ids_without_sockets, socketList,
                         std::tr1::bind(&DistributedCoordinateSegmentation::sendLoadReportOnSocket,this,
                                        clientSocket, boundingBox, *message, _1));
}

void DistributedCoordinateSegmentation::sendLoadReportOnSocket(boost::shared_ptr<tcp::socket> clientSocket, BoundingBox3f boundingBox, Sirikata::Protocol::CSeg::LoadReportMessage message, std::map< ServerID, SocketContainer > socketList)
{
  assert(socketList.size() > 0);

  ServerID cseg_server_id = socketList.begin()->first;
  SocketContainer socketContainer = socketList.begin()->second;

  SocketPtr socket= socketContainer.socket();

  if (socket.get() == 0) {
    assert(false);
    return;
  }

  Sirikata::Protocol::CSeg::CSegMessage csegMessage;
  csegMessage.mutable_ll_load_report_message().set_lower_root_box(boundingBox);
  csegMessage.mutable_ll_load_report_message().mutable_load_report_message().set_server(message.server());
  csegMessage.mutable_ll_load_report_message().mutable_load_report_message().set_load_value(message.load_value());
  csegMessage.mutable_ll_load_report_message().mutable_load_report_message().set_bbox(message.bbox());

  writeCSEGMessage(socket, csegMessage);
  //read ack message and discard
  readCSEGMessage(socket, csegMessage);

  //send ack to space server
  Sirikata::Protocol::CSeg::CSegMessage csegResponseMessage;
  csegResponseMessage.mutable_load_report_ack_message().set_ack(1);
  writeCSEGMessage(clientSocket, csegResponseMessage);

  uint8* asyncBufferArray = new uint8[1];
  clientSocket->async_read_some( boost::asio::buffer(asyncBufferArray, 1),
         std::tr1::bind(&DistributedCoordinateSegmentation::asyncRead, this,
                           clientSocket, asyncBufferArray, _1, _2)  );

  boost::upgrade_lock<boost::shared_mutex> lock(mSocketsToCSEGServersMutex);
  mLeasedSocketsToCSEGServers[cseg_server_id]->push(socketContainer, false);
}

void DistributedCoordinateSegmentation::sendToAllCSEGServers(Sirikata::Protocol::CSeg::CSegMessage& csegMessage){
  /* Send to other CSEG servers so they can forward the segmentation change message to
     space servers connected to them. */
  std::map< ServerID, SocketContainer >* socketList = new std::map<ServerID, SocketContainer>();

  std::vector<ServerID> ids_without_sockets;

  for (uint32 i = 1; i <= (uint32)mAvailableCSEGServers; i++) {
    if (i == mContext->id()) continue;

    SocketContainer socketContainer = getSocketToCSEGServer(i);
    SocketPtr socket = socketContainer.socket();

    if (!socket || socket.get() == 0) {
      ids_without_sockets.push_back(i);
      continue;
    }

    (*socketList)[i] = socketContainer;
  }

  createSocketContainers(ids_without_sockets, socketList,
                         std::tr1::bind(&DistributedCoordinateSegmentation::sendOnAllSockets,this,
                                        csegMessage, _1));
}

void DistributedCoordinateSegmentation::sendOnAllSockets(Sirikata::Protocol::CSeg::CSegMessage csegMessage, std::map< ServerID, SocketContainer > socketList)
{
    //send the request to other CSEG servers.
  for (std::map<ServerID, SocketContainer >::iterator it = socketList.begin();
       it != socketList.end(); it++
      )
  {
    ServerID server_id = it->first;

    SocketContainer& socketContainer = it->second;
    SocketPtr socket = socketContainer.socket();

    if (socket.get() == 0)
      continue;

    writeCSEGMessage(socket, csegMessage);

    boost::upgrade_lock<boost::shared_mutex> lock(mSocketsToCSEGServersMutex);
    mLeasedSocketsToCSEGServers[server_id]->push(socketContainer, false);

  }
}

void DistributedCoordinateSegmentation::sendToAllSpaceServers(Sirikata::Protocol::CSeg::CSegMessage& csegMessage) {
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
    boost::shared_ptr<tcp::socket> socket = boost::shared_ptr<tcp::socket>(new tcp::socket(mIOService));
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
      {
	socket->close();
	socket->connect(*endpoint_iterator++, error);
      }
    if (error) {
      std::cout << "Connection refused to " << addr << ":"<<port_str <<"\n";
      continue;
    }

    writeCSEGMessage(socket, csegMessage);

    socket->close();
  }
}

void DistributedCoordinateSegmentation::writeCSEGMessage(boost::shared_ptr<tcp::socket> socket,
                                                         Sirikata::Protocol::CSeg::CSegMessage& csegMessage)
{
  std::string buffer = serializePBJMessage(csegMessage);

  uint32 length = htonl(buffer.size());

  buffer = std::string( (char*) &length, sizeof(length))  +  buffer;

  boost::asio::write(*socket,
                     boost::asio::buffer((void*) buffer.data(), buffer.size()),
                     boost::asio::transfer_all() );
}

void DistributedCoordinateSegmentation::readCSEGMessage(boost::shared_ptr<tcp::socket> socket,
                                                        Sirikata::Protocol::CSeg::CSegMessage& csegMessage,
                                                        uint8* bufferSoFar, uint32 bufferSoFarSize
                                                        )
{
  assert(bufferSoFarSize <= 4);

  uint8 lengthBuf[4];
  if (bufferSoFarSize != 0) {
    lengthBuf[0] = bufferSoFar[0];
  }

  boost::asio::read(*socket, boost::asio::buffer( (void*)(lengthBuf+bufferSoFarSize), sizeof(uint32)-bufferSoFarSize ),
                    boost::asio::transfer_all());

  uint32* length = (uint32*) lengthBuf;
  *length = ntohl(*length);


  uint8* buf = new uint8[*length];
  boost::asio::read(*socket, boost::asio::buffer(buf, *length), boost::asio::transfer_all());

  std::string str = std::string((char*) buf, *length);

  bool val = parsePBJMessage(&csegMessage, str);

  assert(val);

  delete buf;
}

void DistributedCoordinateSegmentation::readCSEGMessage(boost::shared_ptr<tcp::socket> socket,
                                                        Sirikata::Protocol::CSeg::CSegMessage& csegMessage)
{
  return readCSEGMessage(socket, csegMessage, NULL, 0);
}



} // namespace Sirikata
