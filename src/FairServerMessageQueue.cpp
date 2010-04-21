#include "FairServerMessageQueue.hpp"
#include "Network.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "Statistics.hpp"

namespace CBR{


FairServerMessageQueue::SenderAdapterQueue::SenderAdapterQueue(Sender* sender, ServerID sid)
        : mSender(sender),
          mDestServer(sid),
          mFront(NULL)
{
}

Message* FairServerMessageQueue::SenderAdapterQueue::front() {
    if (mFront != NULL)
        return mFront;

    mFront = mSender->serverMessagePull(mDestServer);
    return mFront;
}

Message* FairServerMessageQueue::SenderAdapterQueue::pop() {
    Message* result = front();
    mFront = NULL;
    return result;
}

bool FairServerMessageQueue::SenderAdapterQueue::empty() {
    return
        (mFront == NULL &&
            mSender->serverMessageEmpty(mDestServer)
        );
}


FairServerMessageQueue::FairServerMessageQueue(SpaceContext* ctx, Network* net, Sender* sender, uint32 send_bytes_per_second)
        : ServerMessageQueue(ctx, net, sender),
          mServerQueues(),
          mLastServiceTime(ctx->simTime()),
          mRate(send_bytes_per_second),
          mRemainderSendBytes(0),
          mLastSendEndTime(ctx->simTime()),
          mServiceScheduled(false),
          mAccountedTime(Duration::seconds(0)),
          mBytesDiscardedBlocked(0),
          mBytesDiscardedUnderflow(0),
          mBytesUsed(0)
{
}

FairServerMessageQueue::~FairServerMessageQueue() {
    SILOG(fsmq,info,
        "FSMQ: Accounted time: " << mAccountedTime <<
        ", used: " << mBytesUsed <<
        ", discarded (underflow): " << mBytesDiscardedUnderflow <<
        ", discarded (blocked): " << mBytesDiscardedBlocked <<
        ", remaining: " << mRemainderSendBytes
    );
}

void FairServerMessageQueue::scheduleServicing() {
    if (!mServiceScheduled.read()) {
        mServiceScheduled = true;
        mSenderStrand->post(
            std::tr1::bind(&FairServerMessageQueue::service, this)
        );
    }
}

void FairServerMessageQueue::service() {
    mProfiler->started();

    // Unmark scheduling
    mServiceScheduled = false;

    Time tcur = mContext->simTime();
    Duration since_last = tcur - mLastServiceTime;
    mAccountedTime += since_last;
    uint64 new_send_bytes = since_last.toSeconds() * mRate;
    uint64 send_bytes = new_send_bytes + mRemainderSendBytes;

    // Send

    Message* next_msg = NULL;
    ServerID sid;
    bool ran_out_of_bytes = false;
    while( send_bytes > 0 && (next_msg = mServerQueues.front(&sid)) != NULL ) {
        uint32 packet_size = next_msg->serializedSize();
        if (packet_size > send_bytes) {
            ran_out_of_bytes = true;
            break;
        }

        bool sent_success = trySend(sid, next_msg);
        if (!sent_success) {
            disableDownstream(sid);
            continue;
        }

        // Pop the message
        Message* next_msg_popped = mServerQueues.pop();
        assert(next_msg == next_msg_popped);

        // Record trace send times
        Duration send_duration = Duration::seconds((float)packet_size / (float)mRate);
        Time start_time = mLastSendEndTime;
        Time end_time = mLastSendEndTime + send_duration;
        mLastSendEndTime = end_time;
        CONTEXT_TRACE_NO_TIME(serverDatagramSent, start_time, end_time, mServerQueues.getQueueWeight(next_msg->dest_server()),
            next_msg->dest_server(), next_msg->id(), packet_size);

        send_bytes -= packet_size;
        mBytesUsed += packet_size;

        // Get rid of the message
        delete next_msg;
    }

    // Update capacity estimate.
    // Since we need capacity rather than accepted rate, we do this from the
    // total rate calculation.  If we remove the send limitation, then we'd need
    // a different solution to this.
    mCapacityEstimator.estimate_rate(tcur, new_send_bytes);

    if (ran_out_of_bytes) {
        // We had a message but couldn't send it.
        mRemainderSendBytes = send_bytes;
        //mLastSendTime = already saved

        // We reschedule only if we still have data to send
        scheduleServicing();
    }
    else {
        // There are 2 ways we could get here:
        // 1. mServerQueues.empty() == true: We ran out of input messages to
        // send.  We have to discard the bytes we didn't have input for
        // 2. Otherwise, we had enough bytes, there were items in the queue,
        // but we couldn't get a front item.  This means all downstream
        // queues were blocking us from sending, so we should discard the
        // bytes so we don't artificially collect extra, old bandwidth over
        // time.
        if (mServerQueues.empty())
            mBytesDiscardedUnderflow += send_bytes;
        else
            mBytesDiscardedBlocked += send_bytes;
        mRemainderSendBytes = 0;
        mLastSendEndTime = tcur;
    }

    mLastServiceTime = tcur;

    mProfiler->finished();
}


void FairServerMessageQueue::messageReady(ServerID sid) {
    mSenderStrand->post(
        std::tr1::bind(&FairServerMessageQueue::handleMessageReady, this, sid)
    );
}

void FairServerMessageQueue::handleMessageReady(ServerID sid) {
    // Make sure we are setup for this server. We don't have a weight for this
    // server yet, so we must use a default. We use the average of all known
    // weights, and hope to get an update soon.
    if (!mServerQueues.hasQueue(sid)) {
        addInputQueue(
            sid,
            getAverageServerWeight()
        );
    }

    // Notify and service
    mServerQueues.notifyPushFront(sid);
    scheduleServicing();
}

float FairServerMessageQueue::getAverageServerWeight() const {
    return mServerQueues.avg_weight();
}

void FairServerMessageQueue::networkReadyToSend(const ServerID& from) {
    // The connection is ready to send again, enable the input queue associated
    // with it.
    mSenderStrand->post(
        std::tr1::bind(&FairServerMessageQueue::enableDownstream, this, from)
                               );
}

void FairServerMessageQueue::addInputQueue(ServerID sid, float weight) {
    assert( !mServerQueues.hasQueue(sid) );
    SILOG(fairsender,info,"Adding input queue for " << sid);
    mServerQueues.addQueue(new SenderAdapterQueue(mSender,sid),sid,weight);
}

void FairServerMessageQueue::handleUpdateReceiverStats(ServerID sid, double total_weight, double used_weight) {
    assert( mServerQueues.hasQueue(sid) );
    mServerQueues.setQueueWeight(sid, used_weight);
}

void FairServerMessageQueue::removeInputQueue(ServerID sid) {
    assert( mServerQueues.hasQueue(sid) );
    mServerQueues.removeQueue(sid);
}

void FairServerMessageQueue::enableDownstream(ServerID sid) {
    // Ignore this stream unless we already know about it -- if we don't know
    // about it then we don't have anything to send to it.
    if (!mServerQueues.hasQueue(sid))
        return;

    // Otherwise make sure we're tracking it, enable the queue, and take this
    // opportunity to service
    if (mDownstreamReady.find(sid) == mDownstreamReady.end())
        mDownstreamReady.insert(sid);

    mServerQueues.enableQueue(sid);

    scheduleServicing();
}

void FairServerMessageQueue::disableDownstream(ServerID sid) {
    mServerQueues.disableQueue(sid);
}

}
