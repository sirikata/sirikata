#include "Network.hpp"
#include "Server.hpp"
#include "ServerNetworkImpl.hpp"
#include "FairServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR{

bool FairServerMessageQueue::CanSendPredicate::operator()(const ServerID& key, const ServerMessagePair* msg) const {
    return fq->canSend(msg);
}


FairServerMessageQueue::FairServerMessageQueue(Network* net, uint32 send_bytes_per_second, uint32 recv_bytes_per_second, const ServerID& sid, ServerIDMap* sidmap, Trace* trace)
 : ServerMessageQueue(net, sid, sidmap, trace),
   mServerQueues( CanSendPredicate(this) ),
   mReceiveQueues(),
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

    ServerMessagePair* smp = new ServerMessagePair(destinationServer,with_header);
    bool success = mServerQueues.push(destinationServer,smp)==QueueEnum::PushSucceeded;
    if (!success) delete smp;
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

bool FairServerMessageQueue::canSend(const ServerMessagePair* next_msg) {
    Address4* addy = mServerIDMap->lookup(next_msg->dest());

    assert(addy != NULL);
    return mNetwork->canSend(*addy,next_msg->data(),false,true,1);
}

void FairServerMessageQueue::service(const Time&t){
    uint64 send_bytes = (t - mLastTime).seconds() * mRate + mRemainderSendBytes;
    uint64 recv_bytes = (t - mLastTime).seconds() * mRecvRate + mRemainderReceiveBytes;

    // Send

    ServerMessagePair* next_msg = NULL;
    ServerID sid;
    bool sent_success = true;
    bool save_bytes = true;
    while( send_bytes > 0 && (next_msg = mServerQueues.front(&send_bytes,&sid)) != NULL ) {
        Address4* addy = mServerIDMap->lookup(next_msg->dest());

        assert(addy != NULL);
        sent_success = mNetwork->send(*addy,next_msg->data(),false,true,1);

        if (!sent_success) break;

        save_bytes = false;

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

    if (!sent_success || mServerQueues.empty()) {
        mRemainderSendBytes = 0;
        mLastSendEndTime = t;
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
            mLastSendEndTime = t;
        }
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
        ChunkSourcePair csp;
        csp.chunk = new Network::Chunk(next_msg->data());
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
    if (!mServerQueues.hasQueue(sid)) {
        mServerQueues.addQueue(new Queue<ServerMessagePair*>(1024*1024)/*FIXME*/,sid,weight);
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
        mTrace->serverDatagramQueueInfo(t, *it, tx_size, tx_used, tx_weight, rx_size, rx_used, rx_weight);
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
