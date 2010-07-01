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

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

#include <sirikata/core/service/PollingService.hpp>

#include <sirikata/space/SegmentedRegion.hpp>
#include "CSegContext.hpp"

#include "CBR_CSeg.pbj.hpp"

typedef boost::asio::ip::tcp tcp;

namespace Sirikata {

class ServerIDMap;

typedef struct ServerAvailability  {
  ServerID mServer;
  bool mAvailable;

} ServerAvailability;



typedef struct SegmentationChangeListener {
  char host[255];
  uint16 port;
} SegmentationChangeListener;

/** Distributed BSP-tree based implementation of CoordinateSegmentation. */
class DistributedCoordinateSegmentation : public PollingService {
public:
    DistributedCoordinateSegmentation(CSegContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim, int, ServerIDMap * );
    virtual ~DistributedCoordinateSegmentation();

    virtual ServerID lookup(const Vector3f& pos) ;
    virtual BoundingBoxList serverRegion(const ServerID& server);
    virtual BoundingBox3f region() ;
    virtual uint32 numServers() ;

    virtual void poll();
    virtual void stop();

private:
    void service();

    void csegChangeMessage(Sirikata::Protocol::CSeg::ChangeMessage* ccMsg);

    void notifySpaceServersOfChange(const std::vector<SegmentationInfo> segInfoVector);

    void serializeBSPTree(SerializedBSPTree* serializedBSPTree);


    void traverseAndStoreTree(SegmentedRegion* region, uint32& idx,
			      SerializedBSPTree* serializedTree);

    void startAccepting();

    void startAcceptingLLRequests();

    CSegContext* mContext;
    //ServerID mCSEGServerID;

    SegmentedRegion mTopLevelRegion;
    Time mLastUpdateTime;

    std::vector<SegmentationChangeListener> mSpacePeers;
    std::vector<ServerAvailability> mAvailableServers;

    boost::asio::io_service mIOService;  //creates an io service
    boost::asio::io_service mLLIOService;  //creates an io service

    boost::shared_ptr<tcp::acceptor> mAcceptor;
    boost::shared_ptr<tcp::socket> mSocket;

    boost::shared_ptr<tcp::acceptor> mLLTreeAcceptor;
    boost::shared_ptr<tcp::socket> mLLTreeAcceptorSocket;

    std::map<String, SegmentedRegion*> mHigherLevelTrees;
    std::map<String, SegmentedRegion*> mLowerLevelTrees;

    int mAvailableCSEGServers;

    void accept_handler();

    void acceptLLTreeRequestHandler();

    void subdivideTopLevelRegion(SegmentedRegion* region,
				 Vector3ui32 perdim,
				 int& numServersAssigned);


    void generateHierarchicalTrees(SegmentedRegion* region, int depth, int& numLLTreesSoFar);

    ServerID callLowerLevelCSEGServer(ServerID, const Vector3f& searchVec, const BoundingBox3f& boundingBox);

    void callLowerLevelCSEGServersForServerRegions(ServerID server_id, BoundingBoxList&);

    void ioServicingLoop();
    void llIOServicingLoop();

    void sendToAllCSEGServers(uint8* buffer, int buflen);

    void sendToAllSpaceServers(uint8* buffer, int buflen);

    uint32 getAvailableServerIndex();

    void getRandomLeafParentSibling(SegmentedRegion** randomLeaf,
				    SegmentedRegion** sibling,
				    SegmentedRegion** parent);

    bool fullMessageReceived(uint8* dataReceived, uint32 bytesReceived);

    uint32 readFromSocket(boost::shared_ptr<tcp::socket> socket,
			  uint8** dataReceived, bool readTillEOF);

    boost::shared_ptr<tcp::socket> getSocketToCSEGServer(ServerID server_id);


    ServerIDMap *  mSidMap;

    std::map<ServerID, BoundingBoxList > mWholeTreeServerRegionMap;
    std::map<ServerID, BoundingBoxList > mLowerTreeServerRegionMap;
}; // class CoordinateSegmentation

} // namespace Sirikata

#endif
