#include "Utility.hpp"
#include "Network.hpp"
#include "ServerNetworkImpl.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR {

FIFOServerMessageQueue::FIFOServerMessageQueue(Network* net, uint32 bytes_per_second, const ServerID& sid, ServerIDMap* sidmap, Trace* trace)
 : ServerMessageQueue(net, sid, sidmap, trace),
   mQueue(),
   mRate(bytes_per_second),
   mRemainderBytes(0),
   mLastTime(0),
   mLastSendEndTime(0)
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

    return mQueue.push(destinationServer, new ServerMessagePair(destinationServer,with_header)) == QueueEnum::PushSucceeded;
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
    uint64 bytes = (t - mLastTime).seconds() * mRate + mRemainderBytes;

    ServerMessagePair* next_msg = NULL;
    while( bytes > 0 && (next_msg = mQueue.front(&bytes)) != NULL ) {
        Address4* addy = mServerIDMap->lookup(next_msg->dest());
        assert(addy != NULL);
        bool sent_success = mNetwork->send(*addy,next_msg->data(),false,true,1);

        if (!sent_success) break;

        ServerMessagePair* next_msg_popped = mQueue.pop(&bytes);
        assert(next_msg_popped == next_msg);

        uint32 packet_size = next_msg->data().size();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;

        mTrace->packetSent(start_time, end_time, next_msg->dest(), next_msg->data());
    }

    if (mQueue.empty()) {
        mRemainderBytes = 0;
        mLastSendEndTime = t;
    }
    else {
        mRemainderBytes = bytes;
        //mLastSendEndTime = already recorded, last end send time
    }

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
