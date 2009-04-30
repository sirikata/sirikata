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
   mRemainderSendBytes(0),
   mRemainderReceiveBytes(0),
   mLastSendEndTime(0),
   mLastReceiveEndTime(0)
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
    assert(destinationServer!=mSourceServer);
    uint32 offset = 0;
    Network::Chunk with_header;
    ServerMessageHeader server_header(mSourceServer, destinationServer);
    offset = server_header.serialize(with_header, offset);
    with_header.insert( with_header.end(), msg.begin(), msg.end() );
    offset += msg.size();

    return mServerQueues.push(destinationServer,new ServerMessagePair(destinationServer,with_header))==QueueEnum::PushSucceeded;
}

bool FairServerMessageQueue::receive(Network::Chunk** chunk_out, ServerID* source_server_out) {
    if (!mReceiveQueue.empty()) {
        ChunkSourcePair csp = mReceiveQueue.front();
        *chunk_out = csp.chunk;
        *source_server_out = csp.source;
        mReceiveQueue.pop();
        return true;
    }

    *chunk_out = NULL;
    return false;
}

void FairServerMessageQueue::service(const Time&t){
    uint64 send_bytes = (t - mLastTime).seconds() * mRate + mRemainderSendBytes;
    uint64 recv_bytes = (t - mLastTime).seconds() * mRecvRate + mRemainderReceiveBytes;

    // Send

    ServerMessagePair* next_msg = NULL;
    ServerID sid;

    while( send_bytes > 0 && (next_msg = mServerQueues.front(&send_bytes,&sid)) != NULL ) {
        Address4* addy = mServerIDMap->lookup(next_msg->dest());
        assert(addy != NULL);
        bool sent_success = mNetwork->send(*addy,next_msg->data(),false,true,1);

        if (!sent_success) break;

        ServerMessagePair* next_msg_popped = mServerQueues.pop(&send_bytes);
        assert(next_msg_popped == next_msg);

        uint32 packet_size = next_msg->data().size();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;

        mTrace->serverDatagramSent(start_time, end_time, getServerWeight(next_msg->dest()),
                                   next_msg->dest(), next_msg->data());

        delete next_msg;
    }

    if (mServerQueues.empty()) {
        mRemainderSendBytes = 0;
        mLastSendEndTime = t;
    }
    else {
        mRemainderSendBytes = send_bytes;
        //mLastSendEndTime = already recorded, last end send time
    }


    // Receive

    while( recv_bytes > 0 && (next_msg = mReceiveQueues.front(&recv_bytes,&sid)) != NULL ) {
        ServerMessagePair* next_msg_popped = mReceiveQueues.pop(&recv_bytes);
        assert(next_msg_popped == next_msg);

        uint32 packet_size = next_msg->data().size();
        Duration recv_duration = Duration::seconds((float)packet_size / (float)mRecvRate);
        Time start_time = mLastReceiveEndTime;
        Time end_time = mLastReceiveEndTime + recv_duration;
        mLastReceiveEndTime = end_time;

        /*
           FIXME at some point we should record this here instead of in Server.cpp
        mTrace->serverDatagramReceived();
        */
        uint32 offset = 0;
        ServerMessageHeader hdr = ServerMessageHeader::deserialize(next_msg->data(), offset);
        assert(hdr.destServer() == mSourceServer);
        Network::Chunk* payload = new Network::Chunk;
        Network::Chunk smp_data = next_msg->data(); // BEWARE we unfortunately copy this here because ServerMessagePair won't return a reference, not sure why its that way
        payload->insert(payload->begin(), smp_data.begin() + offset, smp_data.end());
        assert( payload->size() == next_msg->data().size() - offset );

        ChunkSourcePair csp;
        csp.chunk = payload;
        csp.source = next_msg->dest();
        mReceiveQueue.push( csp );

        delete next_msg;
    }

    if (mReceiveQueues.empty()) {
        mRemainderReceiveBytes = 0;
        mLastReceiveEndTime = t;
    }
    else {
        mRemainderReceiveBytes = recv_bytes;
        //mLastReceiveEndTime = already recorded, last end receive time
    }



    mLastTime = t;
}

void FairServerMessageQueue::setServerWeight(ServerID sid, float weight) {
    // send weight
    if (!mServerQueues.hasQueue(sid))
        mServerQueues.addQueue(new Queue<ServerMessagePair*>(1024*1024)/*FIXME*/,sid,weight);
    else
        mServerQueues.setQueueWeight(sid, weight);

    // receive weight
    if (!mReceiveQueues.hasQueue(sid))
        mReceiveQueues.addQueue(new NetworkQueueWrapper(sid, mNetwork, mServerIDMap),sid,weight);
    else
        mReceiveQueues.setQueueWeight(sid, weight);

    // add to the receive set
    mReceiveSet.insert(sid);
}

float FairServerMessageQueue::getServerWeight(ServerID sid) {
    if (mServerQueues.hasQueue(sid))
        return mServerQueues.getQueueWeight(sid);

    return 0;
}

void FairServerMessageQueue::reportQueueInfo(const Time& t) const {
    for(ReceiveServerSet::const_iterator it = mReceiveSet.begin(); it != mReceiveSet.end(); it++) {
        uint32 tx_size = mServerQueues.maxSize(*it), tx_used = mServerQueues.size(*it);
        float tx_weight = mServerQueues.getQueueWeight(*it);
        uint32 rx_size = mReceiveQueues.maxSize(*it), rx_used = mReceiveQueues.size(*it);
        float rx_weight = mReceiveQueues.getQueueWeight(*it);
        mTrace->serverDatagramQueueInfo(t, *it, tx_size, tx_used, tx_weight, rx_size, rx_used, rx_weight);
    }
}

}
