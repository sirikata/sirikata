#include "Network.hpp"
#include "Server.hpp"
#include "ServerNetworkImpl.hpp"
#include "FairServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR{
FairServerMessageQueue::FairServerMessageQueue(Network* net, uint32 send_bytes_per_second, uint32 recv_bytes_per_second, bool renormalizeWeights, const ServerID& sid, ServerIDMap* sidmap, Trace* trace)
 : ServerMessageQueue(net, sid, sidmap, trace),
   mServerQueues(0,renormalizeWeights),
   mReceiveQueues(0,false),
   mLastTime(0),
   mRate(send_bytes_per_second),
   mRecvRate(recv_bytes_per_second),
   mRemainderBytes(0),
   mLastSendEndTime(0)
{
}

bool FairServerMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    // If its just coming back here, skip routing and just push the payload onto the receive queue
    if (mSourceServer == destinationServer) {
        ChunkSourcePair csp;
        csp.chunk = new Network::Chunk(msg);
        csp.source = mSourceServer;

        mReceiveQueue.push(csp);
        return true;
    }

    uint32 offset = 0;
    Network::Chunk with_header;
    ServerMessageHeader server_header(mSourceServer, destinationServer);
    offset = server_header.serialize(with_header, offset);
    with_header.insert( with_header.end(), msg.begin(), msg.end() );
    offset += msg.size();

    return mServerQueues.push(destinationServer,new ServerMessagePair(destinationServer,with_header))==QueueEnum::PushSucceeded;
}

bool FairServerMessageQueue::receive(Network::Chunk** chunk_out, ServerID* source_server_out) {
    if (mReceiveQueue.empty()) {
        *chunk_out = NULL;
        return false;
    }

    *chunk_out = mReceiveQueue.front().chunk;
    *source_server_out = mReceiveQueue.front().source;
    mReceiveQueue.pop();

    return true;
}

void FairServerMessageQueue::service(const Time&t){
    uint64 bytes = (t - mLastTime).seconds() * mRate + mRemainderBytes;

    ServerMessagePair* next_msg = NULL;
    ServerID sid;
    while( bytes > 0 && (next_msg = mServerQueues.front(&bytes,&sid)) != NULL ) {
        Address4* addy = mServerIDMap->lookup(next_msg->dest());
        assert(addy != NULL);
        bool sent_success = mNetwork->send(*addy,next_msg->data(),false,true,1);

        if (!sent_success) break;

        ServerMessagePair* next_msg_popped = mServerQueues.pop(&bytes);
        assert(next_msg_popped == next_msg);

        uint32 packet_size = next_msg->data().size();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;

        mTrace->serverDatagramSent(start_time, end_time, next_msg->dest(), next_msg->data());

        delete next_msg;
    }

    if (mServerQueues.empty()) {
        mRemainderBytes = 0;
        mLastSendEndTime = t;
    }
    else {
        mRemainderBytes = bytes;
        //mLastSendEndTime = already recorded, last end send time
    }


    // no limit on receive bandwidth
    for(ReceiveServerList::iterator it = mSourceServers.begin(); it != mSourceServers.end(); it++) {
        Address4* addr = mServerIDMap->lookup(it->first);
        assert(addr != NULL);
        int64 bytes = (int64)((t - mLastTime).seconds() * mRecvRate) + it->second.mRemainderBytes;
        if (bytes<=0) {
            it->second.mRemainderBytes=bytes;
        }else while( Network::Chunk* c = mNetwork->receiveOne(*addr, 1000000) ) {

            bytes-=c->size();
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
            it->second.mRemainderBytes=0;
            if (bytes<=0) {
                it->second.mRemainderBytes=bytes; 
                break;
            }
        }
    }
    mLastTime = t;
}

void FairServerMessageQueue::setServerWeight(ServerID sid, float weight) {
    if (!mServerQueues.hasQueue(sid)) {
        mServerQueues.addQueue(new Queue<ServerMessagePair*>(1024*1024)/*FIXME*/,sid,weight);
    }
    else {
        mServerQueues.setQueueWeight(sid, weight);
    }

    mSourceServers[sid];
}

}
