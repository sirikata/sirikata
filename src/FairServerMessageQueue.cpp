#include "Network.hpp"
#include "Server.hpp"
#include "FairServerMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "Statistics.hpp"

namespace CBR{

FairServerMessageQueue::FairServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, Listener* listener, uint32 send_bytes_per_second)
 : ServerMessageQueue(ctx, net, sidmap, listener),
   mServerQueues(),
   mLastServiceTime(ctx->time),
   mRate(send_bytes_per_second),
   mRemainderSendBytes(0),
   mLastSendEndTime(ctx->simTime())
{
}

bool FairServerMessageQueue::canAddMessage(const Message* msg) {
    ServerID destinationServer = msg->dest_server();

    // We won't handle routing to ourselves, the layer above us should handle this
    assert (mContext->id() != destinationServer);

    uint32 offset = msg->serializedSize();

    size_t size = mServerQueues.size(destinationServer);
    size_t maxsize = mServerQueues.maxSize(destinationServer);
    if (size+offset<=maxsize) return true;

    if (offset > maxsize) SILOG(queue,fatal,"Checked push message that's too large on to FairServerMessageQueue: " << offset << " > " << maxsize);


    return false;
}

bool FairServerMessageQueue::addMessage(Message* msg){
    ServerID destinationServer = msg->dest_server();

    // We won't handle routing to ourselves, the layer above us should handle this
    assert (mContext->id() != destinationServer);

    // Otherwise, store for push to network
    bool success = mServerQueues.push(destinationServer,msg)==QueueEnum::PushSucceeded;
    return success;
}

void FairServerMessageQueue::service(){
    mProfiler->started();

    Time tcur = mContext->simTime();
    Duration since_last = tcur - mLastServiceTime;
    uint64 send_bytes = since_last.toSeconds() * mRate + mRemainderSendBytes;

    // Send

    Message* next_msg = NULL;
    ServerID sid;
    bool save_bytes = true;
    while( send_bytes > 0 && (next_msg = mServerQueues.front(&send_bytes,&sid)) != NULL ) {
        Address4* addy = mServerIDMap->lookupInternal(next_msg->dest_server());
        assert(addy != NULL);

        Network::Chunk serialized;
        next_msg->serialize(&serialized);
        bool sent_success = mNetwork->send(*addy, serialized);
        if (!sent_success)
            break;

        // Pop the message
        Message* next_msg_popped = mServerQueues.pop(&send_bytes);
        assert(next_msg == next_msg_popped);
        save_bytes = false;

        // Record trace send times
        uint32 packet_size = serialized.size();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;
        mContext->trace()->serverDatagramSent(start_time, end_time, getServerWeight(next_msg->dest_server()),
            next_msg->dest_server(), next_msg->id(), serialized.size());

        mListener->serverMessageSent(next_msg);
        // Get rid of the message
        delete next_msg;
    }

    if (mServerQueues.empty()) {
        mRemainderSendBytes = 0;
        mLastSendEndTime = tcur;
    }
    else {
        // NOTE: we used to just leave mLastSendEndTime at the last time recorded since the leftover
        // bytes should be used starting at that time. However, now when we exit the loop we can't tell
        // if it was due to a) not having enough bytes for a message or b) not being able to send the
        // message on the network.  Therefore, we're conservative and make time progress.  This could
        // screw up the statistics a little bit, but only by the size of the largest packet. Further, if
        // we don't do this, then when we consistently exit the loop due to not being able to push onto
        // the network (which starts happening a lot when a queue gets backed up), then we stop accounting
        // for time correctly and things get improperly recored.
        if (save_bytes) {
            mRemainderSendBytes = send_bytes;
            //mLastSendTime = already saved
        }
        else {
            mRemainderSendBytes = 0;
            mLastSendEndTime = tcur;
        }
    }

    mLastServiceTime = tcur;

    mProfiler->finished();
}

void FairServerMessageQueue::setServerWeight(ServerID sid, float weight) {
    // send weight
    if (!mServerQueues.hasQueue(sid)) {
        mServerQueues.addQueue(new Queue<Message*>(GetOption(SERVER_QUEUE_LENGTH)->as<uint32>()),sid,weight);
    }
    else
        mServerQueues.setQueueWeight(sid, weight);
}

float FairServerMessageQueue::getServerWeight(ServerID sid) {
    if (mServerQueues.hasQueue(sid))
        return mServerQueues.getQueueWeight(sid);

    return 0;
}

void FairServerMessageQueue::reportQueueInfo(const Time& t) const {
    for(FairSendQueue::const_iterator it = mServerQueues.keyBegin(); it != mServerQueues.keyEnd(); it++) {
        uint32 tx_size = mServerQueues.maxSize(*it), tx_used = mServerQueues.size(*it);
        float tx_weight = mServerQueues.getQueueWeight(*it);
        mContext->trace()->serverDatagramQueueInfo(t, *it, tx_size, tx_used, tx_weight);
    }
}

void FairServerMessageQueue::getQueueInfo(std::vector<QueueInfo>& queue_info) const  {
    queue_info.clear();

    for(FairSendQueue::const_iterator it = mServerQueues.keyBegin(); it != mServerQueues.keyEnd(); it++) {
        uint32 tx_size = mServerQueues.maxSize(*it), tx_used = mServerQueues.size(*it);
        float tx_weight = mServerQueues.getQueueWeight(*it);

	QueueInfo qInfo(tx_size, tx_used, tx_weight);
	queue_info.push_back(qInfo);
    }
}

}
