#include "Utility.hpp"
#include "Network.hpp"
#include "ServerNetworkImpl.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR {

FIFOServerMessageQueue::FIFOServerMessageQueue(Network* net, uint32 bytes_per_second, const ServerID& sid, ServerIDMap* sidmap, Trace* trace)
 : ServerMessageQueue(net, sid, sidmap, trace),
   mRate(bytes_per_second),
   mRemainderBytes(0),
   mLastTime(0)
{
}

bool FIFOServerMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    // If its just coming back here, skip routing and just push the payload onto the receive queue
    if (mSourceServer == destinationServer) {
        ChunkSourcePair csp;
        csp.chunk = new Network::Chunk(msg);
        csp.source = mSourceServer;

        mReceiveQueue.push(csp);
        return true;
    }

    // Otherwise, attach the header and push it to the network
    uint32 offset = 0;
    Network::Chunk with_header;
    ServerMessageHeader server_header(mSourceServer, destinationServer);
    offset = server_header.serialize(with_header, offset);
    with_header.insert( with_header.end(), msg.begin(), msg.end() );
    offset += msg.size();

    mQueue.push(ServerMessagePair(destinationServer,with_header));

    return true;
}

bool FIFOServerMessageQueue::receive(Network::Chunk** chunk_out, ServerID* source_server_out) {
    if (mReceiveQueue.empty()) {
        *chunk_out = NULL;
        return false;
    }

    *chunk_out = mReceiveQueue.front().chunk;
    *source_server_out = mReceiveQueue.front().source;
    mReceiveQueue.pop();

    return true;
}

void FIFOServerMessageQueue::service(const Time& t){
    Duration sinceLast = t - mLastTime;
    uint32 free_bytes = mRemainderBytes + (uint32)(sinceLast.seconds() * mRate);

    while(!mQueue.empty() && mQueue.front().data().size() <= free_bytes) {
        Address4* addy = mServerIDMap->lookup(mQueue.front().dest());
        assert(addy != NULL);
        bool ok=mNetwork->send(*addy,mQueue.front().data(),false,true,1);
        free_bytes -= mQueue.front().data().size();
        assert(ok&&"Network Send Failed");
        mTrace->packetSent(t, mQueue.front().dest(), mQueue.front().data());
        mQueue.pop();
    }

    mRemainderBytes = mQueue.empty() ? 0 : free_bytes;

    mLastTime = t;

    // no limit on receive bandwidth
    while( Network::Chunk* c = mNetwork->receiveOne() ) {
        uint32 offset = 0;
        ServerMessageHeader hdr = ServerMessageHeader::deserialize(*c, offset);
        assert(hdr.destServer() == mSourceServer);
        Network::Chunk* payload = new Network::Chunk;
        payload->insert(payload->begin(), c->begin() + offset, c->end());
        delete c;

        ChunkSourcePair csp;
        csp.chunk = payload;
        csp.source = hdr.sourceServer();

        mReceiveQueue.push(csp);
    }
}

void FIFOServerMessageQueue::setServerWeight(ServerID sid, float weight) {
}



}
