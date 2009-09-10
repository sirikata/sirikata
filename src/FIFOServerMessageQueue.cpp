#include "Utility.hpp"
#include "Network.hpp"
#include "ServerNetworkImpl.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "Statistics.hpp"

namespace CBR {

FIFOServerMessageQueue::FIFOServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, uint32 send_bytes_per_second, uint32 recv_bytes_per_second)
 : ServerMessageQueue(ctx, net, sidmap),
   mQueue(GetOption(SERVER_QUEUE_LENGTH)->as<uint32>() * 32), //FIXME * nobjects?
   mReceiveQueues(),
   mSendRate(send_bytes_per_second),
   mRecvRate(recv_bytes_per_second),
   mRemainderSendBytes(0),
   mRemainderRecvBytes(0),
   mLastSendEndTime(mContext->time),
   mLastReceiveEndTime(mContext->time)
{
}
bool FIFOServerMessageQueue::canAddMessage(ServerID destinationServer,const Network::Chunk&msg){
    // If its just coming back here, skip routing and just push the payload onto the receive queue
    if (mContext->id() == destinationServer) {
        return true;
    }
    uint32 offset = 0;
    Network::Chunk with_header;
    ServerMessageHeader server_header(mContext->id(), destinationServer);
    offset = server_header.serialize(with_header, offset);
    offset += msg.size();
    size_t size=mQueue.size(destinationServer);
    size_t msize=mQueue.maxSize(destinationServer);
    return size+offset<=msize;
}
bool FIFOServerMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    // If its just coming back here, skip routing and just push the payload onto the receive queue
    if (mContext->id() == destinationServer) {
        ChunkSourcePair csp;
        csp.chunk = new Network::Chunk(msg);
        csp.source = mContext->id();

        mReceiveQueue.push(csp);
        return true;
    }

    // Otherwise, attach the header and push it to the network
    uint32 offset = 0;
    Network::Chunk with_header;
    ServerMessageHeader server_header(mContext->id(), destinationServer);
    offset = server_header.serialize(with_header, offset);
    with_header.insert( with_header.end(), msg.begin(), msg.end() );
    offset += msg.size();

    ServerMessagePair* smp = new ServerMessagePair(destinationServer,with_header);
    bool success = mQueue.push(destinationServer,smp)==QueueEnum::PushSucceeded;
    if (!success) delete smp;
    return success;
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

void FIFOServerMessageQueue::service(){
    uint64 send_bytes = mContext->sinceLast.toSeconds() * mSendRate + mRemainderSendBytes;
    uint64 recv_bytes = mContext->sinceLast.toSeconds() * mRecvRate + mRemainderRecvBytes;

    // Send

    ServerMessagePair* next_msg = NULL;
    bool sent_success = true;
    while( send_bytes > 0 && (next_msg = mQueue.front(&send_bytes)) != NULL ) {
        Address4* addy = mServerIDMap->lookupInternal(next_msg->dest());
        assert(addy != NULL);
        sent_success = mNetwork->send(*addy,next_msg->data(),false,true,1);

        if (!sent_success) break;

        ServerMessagePair* next_msg_popped = mQueue.pop(&send_bytes);
        assert(next_msg_popped == next_msg);

        uint32 packet_size = next_msg->data().size();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mSendRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;

        mContext->trace()->serverDatagramSent(start_time, end_time, 1, next_msg->dest(), next_msg->data());

        delete next_msg;
    }

    if (!sent_success || mQueue.empty()) {
        mRemainderSendBytes = 0;
        mLastSendEndTime = mContext->time;
    }
    else {
        mRemainderSendBytes = send_bytes;
        //mLastSendEndTime = already recorded, last end send time
    }


    // Receive

    ServerID sid;
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
        mContext->trace()->serverDatagramReceived();
        */
        ChunkSourcePair csp;
        csp.chunk = new Network::Chunk(next_msg->data());
        csp.source = next_msg->dest();
        mReceiveQueue.push( csp );

        delete next_msg;
    }

    if (mReceiveQueues.empty()) {
        mRemainderRecvBytes = 0;
        mLastReceiveEndTime = mContext->time;
    }
    else {
        mRemainderRecvBytes = recv_bytes;
        //mLastReceiveEndTime = already recorded, last end receive time
    }
}

void FIFOServerMessageQueue::setServerWeight(ServerID sid, float weight) {
    // receive weight
    if (!mReceiveQueues.hasQueue(sid))
        mReceiveQueues.addQueue(new NetworkQueueWrapper(sid, mNetwork, mServerIDMap),sid,1.f);
}

void FIFOServerMessageQueue::reportQueueInfo(const Time& t) const {
    for(ReceiveServerList::const_iterator it = mSourceServers.begin(); it != mSourceServers.end(); it++) {
        uint32 tx_size = mQueue.maxSize(*it), tx_used = mQueue.size(*it);
        float tx_weight = 0;
        uint32 rx_size = 0, rx_used = 0; // no values make sense here since we're not limiting at all
        float rx_weight = 0;
        mContext->trace()->serverDatagramQueueInfo(t, *it, tx_size, tx_used, tx_weight, rx_size, rx_used, rx_weight);
    }
}

void FIFOServerMessageQueue::getQueueInfo(std::vector<QueueInfo>& queue_info) const  {
  queue_info.clear();

  for(ReceiveServerList::const_iterator it = mSourceServers.begin(); it != mSourceServers.end(); it++) {
    uint32 tx_size = mQueue.maxSize(*it), tx_used = mQueue.size(*it);
    float tx_weight = 0;
    uint32 rx_size = 0, rx_used = 0; // no values make sense here since we're not limiting at all
    float rx_weight = 0;

    QueueInfo qInfo(tx_size, tx_used, tx_weight, rx_size, rx_used, rx_weight);
    queue_info.push_back(qInfo);
  }
}

}
