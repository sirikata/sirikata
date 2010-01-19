#include "Network.hpp"
#include "Server.hpp"
#include "FairServerMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "Statistics.hpp"

namespace CBR{


FairServerMessageQueue::SenderAdapterQueue::SenderAdapterQueue(Sender* sender, ServerID sid)
        : mSender(sender),
          mDestServer(sid)
{
}


FairServerMessageQueue::FairServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, Sender* sender, uint32 send_bytes_per_second)
        : ServerMessageQueue(ctx, net, sidmap, sender),
          mServerQueues(),
          mLastServiceTime(ctx->time),
          mRate(send_bytes_per_second),
          mRemainderSendBytes(0),
          mLastSendEndTime(ctx->simTime())
{
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

        bool sent_success = trySend(*addy, next_msg);
        if (!sent_success) {
            disableDownstream(sid);
            continue;
        }

        // Pop the message
        Message* next_msg_popped = mServerQueues.pop(&send_bytes);
        assert(next_msg == next_msg_popped);
        save_bytes = false;

        // Record trace send times
        uint32 packet_size = next_msg->serializedSize();
        Duration send_duration = Duration::seconds((float)packet_size / (float)mRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;
        mContext->trace()->serverDatagramSent(start_time, end_time, mServerQueues.getQueueWeight(next_msg->dest_server()),
            next_msg->dest_server(), next_msg->id(), packet_size);

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


void FairServerMessageQueue::messageReady(ServerID sid) {
    mServerQueues.notifyPushFront(sid);

    service();
}

void FairServerMessageQueue::networkReadyToSend(const Address4& from) {
    // The connection is ready to send again, enable the input queue associated
    // with it.
    ServerID* sid = mServerIDMap->lookupInternal(from);
    assert(sid != NULL);

    mContext->mainStrand->post(
        std::tr1::bind(&FairServerMessageQueue::enableDownstream, this, *sid)
                               );
}

void FairServerMessageQueue::addInputQueue(ServerID sid, float weight) {
    assert( !mServerQueues.hasQueue(sid) );
    SILOG(fairsender,info,"Adding input queue for " << sid);
    mServerQueues.addQueue(new SenderAdapterQueue(mSender,sid),sid,weight);
}

void FairServerMessageQueue::updateInputQueueWeight(ServerID sid, float weight) {
    assert( mServerQueues.hasQueue(sid) );
    mServerQueues.setQueueWeight(sid, weight);
}

void FairServerMessageQueue::removeInputQueue(ServerID sid) {
    assert( mServerQueues.hasQueue(sid) );
    mServerQueues.removeQueue(sid);
}

void FairServerMessageQueue::enableDownstream(ServerID sid) {
    if (mDownstreamReady.find(sid) == mDownstreamReady.end())
        mDownstreamReady.insert(sid);

    mServerQueues.enableQueue(sid);

    service();
}

void FairServerMessageQueue::disableDownstream(ServerID sid) {
    mServerQueues.disableQueue(sid);
}

}
