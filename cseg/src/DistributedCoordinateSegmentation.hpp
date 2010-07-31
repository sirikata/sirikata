/*  Sirikata
 *  DistributedCoordinateSegmentation.hpp
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

#ifndef _SIRIKATA_DISTRIBUTED_COORDINATE_SEGMENTATION_HPP_
#define _SIRIKATA_DISTRIBUTED_COORDINATE_SEGMENTATION_HPP_

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/queue/SizedThreadSafeQueue.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/space/SegmentedRegion.hpp>
#include "CSegContext.hpp"
#include "LoadBalancer.hpp"

#include "Protocol_CSeg.pbj.hpp"

typedef boost::asio::ip::tcp tcp;

namespace Sirikata {

class ServerIDMap;

typedef struct SegmentationChangeListener {
  char host[255];
  uint16 port;
} SegmentationChangeListener;

typedef boost::shared_ptr<tcp::socket> SocketPtr;
typedef struct SocketContainer {
  SocketContainer() {

  }

  SocketContainer(SocketPtr socket) {
    mSocket = socket;
  }
  
  size_t size() {
    return 1;
  }
  
  SocketPtr socket() {
    return mSocket;
  }
  
  SocketPtr mSocket;

} SocketContainer;
  
typedef boost::shared_ptr<Sirikata::SizedThreadSafeQueue<SocketContainer> > SocketQueuePtr;
  
/** Distributed kd-tree based implementation of CoordinateSegmentation. */
class DistributedCoordinateSegmentation : public PollingService {
public:
    DistributedCoordinateSegmentation(CSegContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim, int, ServerIDMap * );
    virtual ~DistributedCoordinateSegmentation();

    virtual ServerID lookup(const Vector3f& pos) ;
    virtual BoundingBoxList serverRegion(const ServerID& server);
    virtual BoundingBox3f region() ;
    virtual uint32 numServers() ;
    virtual std::vector<ServerID> lookupBoundingBox(const BoundingBox3f& bbox);

    virtual void poll();
    virtual void stop();

private:
    void service();    
 
    CSegContext* mContext;

    SegmentedRegion mTopLevelRegion;
    Time mLastUpdateTime;
  
    LoadBalancer mLoadBalancer;

    std::vector<SegmentationChangeListener> mSpacePeers;
    

    boost::asio::io_service mIOService;  //creates an io service
    boost::asio::io_service mLLIOService;  //creates an io service

    boost::shared_ptr<tcp::acceptor> mAcceptor;
    boost::shared_ptr<tcp::socket> mSocket;
    boost::unordered_map<uint8, uint32> mMessageSizes;
    

    boost::shared_ptr<tcp::acceptor> mLLTreeAcceptor;
    boost::shared_ptr<tcp::socket> mLLTreeAcceptorSocket;

    std::map<String, SegmentedRegion*> mHigherLevelTrees;
    std::map<String, SegmentedRegion*> mLowerLevelTrees;

    int mAvailableCSEGServers;

    void csegChangeMessage(Sirikata::Protocol::CSeg::ChangeMessage* ccMsg);
    void handleLoadReport(LoadReportMessage *message);
    void notifySpaceServersOfChange(const std::vector<SegmentationInfo> segInfoVector);

    /* Start listening for and accepting incoming connections.  */
    void startAccepting();
    void startAcceptingLLRequests();

    /* Handlers for incoming connections */
    void accept_handler();
    void acceptLLTreeRequestHandler();

   /* Functions to do the initial kd-tree partitioning of the virtual world into regions, and
      divide the kd-tree into upper and lower trees.     
   */
    void generateHierarchicalTrees(SegmentedRegion* region, int depth, int& numLLTreesSoFar);
    void subdivideTopLevelRegion(SegmentedRegion* region,
				 Vector3ui32 perdim,
				 int& numServersAssigned);



    /* Functions to contact another CSEG server, create sockets to them and/or forward calls to them.
       These should go away later when calls to CSEG no longer remain recursive. */    
    
    ServerID callLowerLevelCSEGServer(ServerID, const Vector3f& searchVec, const BoundingBox3f& boundingBox);
    void callLowerLevelCSEGServersForServerRegions(ServerID server_id, BoundingBoxList&);
    void sendLoadReportToLowerLevelCSEGServer(ServerID, const Vector3f& searchVec, 
                                            const BoundingBox3f& boundingBox,
                                            LoadReportMessage* message);
    void callLowerLevelCSEGServersForLookupBoundingBoxes(const BoundingBox3f& bbox,
                                                         const std::map<ServerID, std::vector<SegmentedRegion*> >&,
                                                         std::vector<ServerID>& );


    /* Functions run in separate threads to listen for incoming packets */
    void ioServicingLoop();
    void llIOServicingLoop();

    /* Functions to send buffers to all CSEG or all space servers  */
    void sendToAllCSEGServers(uint8* buffer, int buflen);
    void sendToAllSpaceServers(uint8* buffer, int buflen);

    uint32 getAvailableServerIndex();

    void getRandomLeafParentSibling(SegmentedRegion** randomLeaf,
				    SegmentedRegion** sibling,
				    SegmentedRegion** parent);

    

    
    /* Functions to read from a socket.  */
    bool fullMessageReceived(uint8* dataReceived, uint32 bytesReceived);
    void asyncRead(boost::shared_ptr<tcp::socket> socket, 
		   uint8* asyncBufferArray,
		   const boost::system::error_code& ec,
		   std::size_t bytes_transferred);
     void asyncLLRead(boost::shared_ptr<tcp::socket> socket,
              uint8* asyncBufferArray,
              const boost::system::error_code& ec,
              std::size_t bytes_transferred);

    uint32 readFromSocket(boost::shared_ptr<tcp::socket> socket,
			  uint8** dataReceived, bool readTillEOF,
			  uint8 bytesReceivedAlready=0);

    /* Functions to serialize the whole CSEG tree. */
    void serializeBSPTree(SerializedBSPTree* serializedBSPTree);
    void traverseAndStoreTree(SegmentedRegion* region, uint32& idx,
			      SerializedBSPTree* serializedTree);
    

    ServerIDMap *  mSidMap;

    /* Key value maps for fast lookup of bounding boxes managed by space servers. */
    std::map<ServerID, BoundingBoxList > mWholeTreeServerRegionMap;
    std::map<ServerID, BoundingBoxList > mLowerTreeServerRegionMap;

    boost::shared_mutex mCSEGReadWriteMutex;


    

    boost::shared_mutex _access;
    std::map<ServerID, SocketQueuePtr > mLeasedSocketsToCSEGServers;

    friend class LoadBalancer;

    
  SocketContainer getSocketToCSEGServer(ServerID server_id);
}; // class CoordinateSegmentation

} // namespace Sirikata

#endif
