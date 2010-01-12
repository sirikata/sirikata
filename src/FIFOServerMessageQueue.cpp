#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "Statistics.hpp"

namespace CBR {

FIFOServerMessageQueue::FIFOServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, Listener* listener, uint32 send_bytes_per_second)
 : ServerMessageQueue(ctx, net, sidmap, listener),
   mQueue(GetOption(SERVER_QUEUE_LENGTH)->as<uint32>() * 32), //FIXME * nobjects?
   mLastServiceTime(ctx->time),
   mSendRate(send_bytes_per_second),
   mRemainderSendBytes(0),
   mLastSendEndTime(mContext->time)
{
}

bool FIFOServerMessageQueue::canAddMessage(const Message* msg){
    ServerID destinationServer = msg->dest_server();

    // We won't handle routing to ourselves, the layer above us should handle this
    assert (mContext->id() != destinationServer);

    uint32 offset = msg->serializedSize();

    size_t size = mQueue.size(destinationServer);
    size_t maxsize = mQueue.maxSize(destinationServer);
    if (size+offset<=maxsize) return true;

    if (offset > maxsize) SILOG(queue,fatal,"Checked push message that's too large on to FIFOServerMessageQueue: " << offset << " > " << maxsize);

    return false;
}

bool FIFOServerMessageQueue::addMessage(Message* msg){
    ServerID destinationServer = msg->dest_server();

    // We won't handle routing to ourselves, the layer above us should handle this
    assert (mContext->id() != destinationServer);

    // Otherwise, store for push to network
    bool success = mQueue.push(destinationServer,msg)==QueueEnum::PushSucceeded;
    return success;
}

void FIFOServerMessageQueue::service(){
    Duration since_last = mContext->time - mLastServiceTime;
    uint64 send_bytes = since_last.toSeconds() * mSendRate + mRemainderSendBytes;

    // Send

    Message* next_msg = NULL;
    bool sent_success = true;
    while( send_bytes > 0 && (next_msg = mQueue.front(&send_bytes)) != NULL ) {
        Address4* addy = mServerIDMap->lookupInternal(next_msg->dest_server());
        assert(addy != NULL);

        Network::Chunk serialized;
        next_msg->serialize(&serialized);

        sent_success = mNetwork->send(*addy,serialized);

        if (!sent_success) break;

        Message* next_msg_popped = mQueue.pop(&send_bytes);
        assert(next_msg_popped == next_msg);

        uint32 packet_size = serialized.size();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mSendRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;

        mContext->trace()->serverDatagramSent(start_time, end_time, 1, next_msg->dest_server(), next_msg->id(), serialized.size());
        mListener->serverMessageSent(next_msg);

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

    mLastServiceTime = mContext->time;

    mProfiler->finished();
}

void FIFOServerMessageQueue::setServerWeight(ServerID sid, float weight) {
}

void FIFOServerMessageQueue::reportQueueInfo(const Time& t) const {
    for(FIFOSendQueue::const_iterator it = mQueue.keyBegin(); it != mQueue.keyEnd(); it++) {
        uint32 tx_size = mQueue.maxSize(*it), tx_used = mQueue.size(*it);
        float tx_weight = 0;
        mContext->trace()->serverDatagramQueueInfo(t, *it, tx_size, tx_used, tx_weight);
    }
}

void FIFOServerMessageQueue::getQueueInfo(std::vector<QueueInfo>& queue_info) const  {
  queue_info.clear();

  for(FIFOSendQueue::const_iterator it = mQueue.keyBegin(); it != mQueue.keyEnd(); it++) {
    uint32 tx_size = mQueue.maxSize(*it), tx_used = mQueue.size(*it);
    float tx_weight = 0;

    QueueInfo qInfo(tx_size, tx_used, tx_weight);
    queue_info.push_back(qInfo);
  }
}

}
