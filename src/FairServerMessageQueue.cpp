#include "Network.hpp"
#include "Server.hpp"
#include "ServerNetworkImpl.hpp"
#include "FairServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR{
FairServerMessageQueue::FairServerMessageQueue(Network* net, uint32 bytes_per_second, bool renormalizeWeights, const ServerID& sid, ServerIDMap* sidmap, Trace* trace)
 : ServerMessageQueue(net, sid, sidmap, trace),
   mServerQueues(0,renormalizeWeights),
   mLastTime(0),
   mRate(bytes_per_second),
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

    return mServerQueues.queueMessage(destinationServer,new ServerMessagePair(destinationServer,with_header))==QueueEnum::PushSucceeded;
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
    uint64 leftover_bytes = 0;
    std::vector<ServerMessagePair*> finalSendMessages = mServerQueues.tick(bytes, &leftover_bytes);
    for (std::vector<ServerMessagePair*>::iterator i=finalSendMessages.begin(),ie=finalSendMessages.end();
         i!=ie;
         ++i) {
        Address4* addy = mServerIDMap->lookup((*i)->dest());
        assert(addy != NULL);
        mNetwork->send(*addy,(*i)->data(),false,true,1);

        uint32 packet_size = (*i)->data().size();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;

        mTrace->packetSent(start_time, end_time, (*i)->dest(), (*i)->data());
    }
    finalSendMessages.resize(0);

    if (mServerQueues.empty()) {
        mRemainderBytes = 0;
        mLastSendEndTime = t;
    }
    else {
        mRemainderBytes = leftover_bytes;
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

void FairServerMessageQueue::setServerWeight(ServerID sid, float weight) {
    if (!mServerQueues.hasQueue(sid)) {
        mServerQueues.addQueue(new Queue<ServerMessagePair*>(65536),sid,weight);
    }
    else {
        mServerQueues.setQueueWeight(sid, weight);
    }
}
void FairServerMessageQueue::removeServer(ServerID sid) {
    mServerQueues.removeQueue(sid);
}

}
