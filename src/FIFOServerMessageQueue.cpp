#include "Utility.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"
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

bool FIFOServerMessageQueue::canAddMessage(const Message* msg){
    ServerID destinationServer = msg->destServer();

    // If its just coming back here, we'll always accept it
    if (mContext->id() == destinationServer) {
        return true;
    }
    assert(destinationServer!=mContext->id());

    uint32 offset = msg->serializedSize();

    size_t size = mQueue.size(destinationServer);
    size_t maxsize = mQueue.maxSize(destinationServer);
    if (size+offset<=maxsize) return true;

    if (offset > maxsize) SILOG(queue,fatal,"Checked push message that's too large on to FIFOServerMessageQueue: " << offset << " > " << maxsize);

    return false;
}

bool FIFOServerMessageQueue::addMessage(Message* msg){
    ServerID destinationServer = msg->destServer();

    // If its just coming back here, skip routing and just push the payload onto the receive queue
    if (mContext->id() == destinationServer) {
        mReceiveQueue.push(msg);
        return true;
    }

    // Otherwise, store for push to network
    bool success = mQueue.push(destinationServer,msg)==QueueEnum::PushSucceeded;
    return success;
}

bool FIFOServerMessageQueue::receive(Message** msg_out) {
    if (mReceiveQueue.empty()) {
        *msg_out = NULL;
        return false;
    }

    *msg_out = mReceiveQueue.front();
    mReceiveQueue.pop();
    return true;
}

void FIFOServerMessageQueue::service(){
    uint64 send_bytes = mContext->sinceLast.toSeconds() * mSendRate + mRemainderSendBytes;
    uint64 recv_bytes = mContext->sinceLast.toSeconds() * mRecvRate + mRemainderRecvBytes;

    // Send

    Message* next_msg = NULL;
    bool sent_success = true;
    while( send_bytes > 0 && (next_msg = mQueue.front(&send_bytes)) != NULL ) {
        Address4* addy = mServerIDMap->lookupInternal(next_msg->destServer());
        assert(addy != NULL);

        Network::Chunk serialized;
        next_msg->serialize(&serialized);

        sent_success = mNetwork->send(*addy,serialized,false,true,1);

        if (!sent_success) break;

        Message* next_msg_popped = mQueue.pop(&send_bytes);
        assert(next_msg_popped == next_msg);

        uint32 packet_size = serialized.size();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mSendRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;

        mContext->trace()->serverDatagramSent(start_time, end_time, 1, next_msg->destServer(), next_msg->id(), serialized.size());

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
    mReceiveQueues.service(); // FIXME this shouldn't be necessary if NetworkQueueWrapper could notify the FairQueue
    ServerID sid;
    Message* next_recv_msg = NULL;
    while( recv_bytes > 0 && (next_recv_msg = mReceiveQueues.front(&recv_bytes,&sid)) != NULL ) {
        Message* next_recv_msg_popped = mReceiveQueues.pop(&recv_bytes);
        assert(next_recv_msg_popped == next_recv_msg);

        uint32 packet_size = next_recv_msg->serializedSize();
        Duration recv_duration = Duration::seconds((float)packet_size / (float)mRecvRate);
        Time start_time = mLastReceiveEndTime;
        Time end_time = mLastReceiveEndTime + recv_duration;
        mLastReceiveEndTime = end_time;

        /*
           FIXME at some point we should record this here instead of in Server.cpp
        mContext->trace()->serverDatagramReceived();
        */
        mReceiveQueue.push(next_recv_msg);
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
