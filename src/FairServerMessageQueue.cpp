#include "Network.hpp"
#include "Server.hpp"
#include "ServerNetwork.hpp"
#include "FairServerMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "Statistics.hpp"

namespace CBR{

bool FairServerMessageQueue::CanSendPredicate::operator()(const ServerID& key, const Message* msg) const {
    return fq->canSend(msg);
}


FairServerMessageQueue::FairServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, uint32 send_bytes_per_second, uint32 recv_bytes_per_second)
 : ServerMessageQueue(ctx, net, sidmap),
   mServerQueues( CanSendPredicate(this) ),
   mReceiveQueues(),
   mRate(send_bytes_per_second),
   mRecvRate(recv_bytes_per_second),
   mRemainderSendBytes(0),
   mRemainderReceiveBytes(0),
   mLastSendEndTime(ctx->time),
   mLastReceiveEndTime(ctx->time)
{
}

bool FairServerMessageQueue::canAddMessage(const Message* msg) {
    ServerID destinationServer = msg->destServer();

    // If its just coming back here, we'll always accept it
    if (mContext->id() == destinationServer) {
        return true;
    }
    assert(destinationServer!=mContext->id());

    uint32 offset = msg->serializedSize();

    size_t size = mServerQueues.size(destinationServer);
    size_t maxsize = mServerQueues.maxSize(destinationServer);
    if (size+offset<=maxsize) return true;

    if (offset > maxsize) SILOG(queue,fatal,"Checked push message that's too large on to FairServerMessageQueue: " << offset << " > " << maxsize);


    return false;
}

bool FairServerMessageQueue::addMessage(Message* msg){
    ServerID destinationServer = msg->destServer();

    // If its just coming back here, skip routing and just push the payload onto the receive queue
    if (mContext->id() == destinationServer) {
        ChunkSourcePair csp;
        csp.chunk = new Network::Chunk();
        msg->serialize(csp.chunk);
        csp.source = mContext->id();

        mReceiveQueue.push(csp);
        delete msg;

        return true;
    }

    // Otherwise, store for push to network
    bool success = mServerQueues.push(destinationServer,msg)==QueueEnum::PushSucceeded;
    return success;
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


// This is the callback for pop() operations, which ensures we push to the network at a safe
// time for the FairQueue.
static void push_to_network(Network* net, Address4* addr, Message* expected, Message* msg) {
    assert(expected == msg);
    Network::Chunk serialized;
    msg->serialize(&serialized);
    bool sent_success = net->send(*addr,serialized,false,true,1);
    if (!sent_success)
        SILOG(fairsmq,fatal,"Failed to push to network, even though predicate succeeded.");
}

void FairServerMessageQueue::service(){
    uint64 send_bytes = mContext->sinceLast.toSeconds() * mRate + mRemainderSendBytes;
    uint64 recv_bytes = mContext->sinceLast.toSeconds() * mRecvRate + mRemainderReceiveBytes;

    // Send

    Message* next_msg = NULL;
    ServerID sid;
    bool save_bytes = true;
    while( send_bytes > 0 && (next_msg = mServerQueues.front(&send_bytes,&sid)) != NULL ) {
        Address4* addy = mServerIDMap->lookupInternal(next_msg->destServer());
        assert(addy != NULL);

        Message* next_msg_popped = mServerQueues.pop(
            &send_bytes,
            std::tr1::bind(
                push_to_network,
                mNetwork,
                addy,
                next_msg,
                std::tr1::placeholders::_1
            )
        );
        assert(next_msg_popped == next_msg);
        save_bytes = false;

        uint32 packet_size = next_msg->serializedSize();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;

        mContext->trace()->serverDatagramSent(start_time, end_time, getServerWeight(next_msg->destServer()),
            next_msg->destServer(), next_msg->id(), next_msg->serializedSize());
        delete next_msg;
    }

    if (mServerQueues.empty()) {
        mRemainderSendBytes = 0;
        mLastSendEndTime = mContext->time;
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
            mLastSendEndTime = mContext->time;
        }
    }


    // Receive
    ServerMessagePair* next_recv_msg = NULL;
    mReceiveQueues.service(); // FIXME this shouldn't be necessary if NetworkQueueWrapper could notify the FairQueue
    while( recv_bytes > 0 && (next_recv_msg = mReceiveQueues.front(&recv_bytes,&sid)) != NULL ) {
        ServerMessagePair* next_recv_msg_popped = mReceiveQueues.pop(&recv_bytes);
        assert(next_recv_msg_popped == next_recv_msg);

        uint32 packet_size = next_recv_msg->data().size();
        Duration recv_duration = Duration::seconds((float)packet_size / (float)mRecvRate);
        Time start_time = mLastReceiveEndTime;
        Time end_time = mLastReceiveEndTime + recv_duration;
        mLastReceiveEndTime = end_time;

        /*
           FIXME at some point we should record this here instead of in Server.cpp
        mContext->trace()->serverDatagramReceived();
        */
        ChunkSourcePair csp;
        csp.chunk = new Network::Chunk(next_recv_msg->data());
        csp.source = next_recv_msg->dest();
        mReceiveQueue.push( csp );

        delete next_recv_msg;
    }

    if (mReceiveQueues.empty()) {
        mRemainderReceiveBytes = 0;
        mLastReceiveEndTime = mContext->time;
    }
    else {
        mRemainderReceiveBytes = recv_bytes;
        //mLastReceiveEndTime = already recorded, last end receive time
    }

}

void FairServerMessageQueue::setServerWeight(ServerID sid, float weight) {
    // send weight
    if (!mServerQueues.hasQueue(sid)) {
        mServerQueues.addQueue(new Queue<Message*>(GetOption(SERVER_QUEUE_LENGTH)->as<uint32>()),sid,weight);
    }
    else
        mServerQueues.setQueueWeight(sid, weight);

    // receive weight
    if (!mReceiveQueues.hasQueue(sid)) {
        mReceiveQueues.addQueue(new NetworkQueueWrapper(sid, mNetwork, mServerIDMap),sid,weight);
    }
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
        mContext->trace()->serverDatagramQueueInfo(t, *it, tx_size, tx_used, tx_weight, rx_size, rx_used, rx_weight);
    }
}

void FairServerMessageQueue::getQueueInfo(std::vector<QueueInfo>& queue_info) const  {
    queue_info.clear();

    for(ReceiveServerSet::const_iterator it = mReceiveSet.begin(); it != mReceiveSet.end(); it++) {
        uint32 tx_size = mServerQueues.maxSize(*it), tx_used = mServerQueues.size(*it);
        float tx_weight = mServerQueues.getQueueWeight(*it);
        uint32 rx_size = mReceiveQueues.maxSize(*it), rx_used = mReceiveQueues.size(*it);
        float rx_weight = mReceiveQueues.getQueueWeight(*it);


	QueueInfo qInfo(tx_size, tx_used, tx_weight, rx_size, rx_used, rx_weight);
	queue_info.push_back(qInfo);
    }
}

}
