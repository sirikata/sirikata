/*  Sirikata
 *  CoordinateSegmentationClient.hpp
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

#ifndef _SIRIKATA_COORDINATE_SEGMENTATION_CLIENT_HPP_
#define _SIRIKATA_COORDINATE_SEGMENTATION_CLIENT_HPP_



#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/space/CoordinateSegmentation.hpp>
#include <sirikata/space/SegmentedRegion.hpp>

#include "Protocol_CSeg.pbj.hpp"


namespace Sirikata {

typedef boost::asio::ip::tcp tcp;

class ServerIDMap;

/** Distributed BSP-tree based implementation of CoordinateSegmentation. */
class CoordinateSegmentationClient : public CoordinateSegmentation {
public:
    CoordinateSegmentationClient(SpaceContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim,
				ServerIDMap* sidmap );
    virtual ~CoordinateSegmentationClient();

    virtual ServerID lookup(const Vector3f& pos) ;
    virtual BoundingBoxList serverRegion(const ServerID& server) ;
    virtual BoundingBox3f region() ;
    virtual uint32 numServers() ;
    virtual std::vector<ServerID> lookupBoundingBox(const BoundingBox3f& bbox);

    // From MessageRecipient
    virtual void receiveMessage(Message* msg);

    virtual void reportLoad(ServerID, const BoundingBox3f& bbox, uint32 loadValue);

    virtual void migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo );

private:
    virtual void service();
    
    void csegChangeMessage(Sirikata::Protocol::CSeg::ChangeMessage* ccMsg);

    void downloadUpdatedBSPTree();
    
    bool mBSPTreeValid;

    Trace::Trace* mTrace;    

    typedef struct LookupCacheEntry {
      LookupCacheEntry(ServerID sid, const BoundingBox3f& bbox) {
        this->bbox = bbox;
        this->sid = sid;
      }

      BoundingBox3f bbox;
      ServerID sid;

    } LookupCacheEntry;

    boost::mutex mCacheMutex;
    std::vector<LookupCacheEntry> mLookupCache;
    uint16 mAvailableServersCount;
    std::map<ServerID, BoundingBoxList> mServerRegionCache;
    SegmentedRegion mTopLevelRegion;

    Network::IOService* mIOService;  //creates an io service
    boost::shared_ptr<Network::TCPListener> mAcceptor;
    boost::shared_ptr<Network::TCPSocket> mSocket;

    ServerIDMap *  mSidMap;

    boost::mutex mMutex;
    boost::shared_ptr<Network::TCPSocket> mLeasedSocket;
    Time mLeaseExpiryTime;

    void startAccepting();
    void accept_handler();

    void sendSegmentationListenMessage();

    boost::shared_ptr<Network::TCPSocket> getLeasedSocket();

    void writeCSEGMessage(boost::shared_ptr<tcp::socket> socket, 
                          Sirikata::Protocol::CSeg::CSegMessage& csegMessage);


    void readCSEGMessage(boost::shared_ptr<tcp::socket> socket, 
                         Sirikata::Protocol::CSeg::CSegMessage& csegMessage);


}; // class CoordinateSegmentation

} // namespace Sirikata

#endif
