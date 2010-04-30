#ifndef _CBR_FAIRSENDQUEUE_HPP
#define _CBR_FAIRSENDQUEUE_HPP

#include "FairQueue.hpp"
#include "ServerMessageQueue.hpp"

namespace CBR {
class FairServerMessageQueue:public ServerMessageQueue {
protected:
    struct SenderAdapterQueue {
        SenderAdapterQueue(Sender* sender, ServerID sid);
        Message* front();
        Message* pop();
        bool empty();
      private:
        Sender* mSender;
        ServerID mDestServer;
        Message* mFront;
    };

    typedef FairQueue<Message, ServerID, SenderAdapterQueue> FairSendQueue;
    FairSendQueue mServerQueues;

    Time mLastServiceTime;

    uint32 mRate;
    uint32 mRemainderSendBytes;
    Time mLastSendEndTime; // last packet send finish time, if there are still
                           // messages waiting

    typedef std::set<ServerID> ServerSet;
    ServerSet mDownstreamReady;

    Sirikata::AtomicValue<bool> mServiceScheduled;

    Duration mAccountedTime;
    uint64 mBytesDiscardedBlocked;
    uint64 mBytesDiscardedUnderflow;
    uint64 mBytesUsed;

    // These must be recursive because networkReadyToSend callbacks may or may
    // not be triggered by this code.
    mutable boost::recursive_mutex mMutex;
    typedef boost::lock_guard<boost::recursive_mutex> MutexLock;
  public:
    FairServerMessageQueue(SpaceContext* ctx, Network* net, Sender* sender, uint32 send_bytes_per_second);
    ~FairServerMessageQueue();

  protected:
    // Must be thread safe:

    // Public ServerMessageQueue interface
    virtual void messageReady(ServerID sid);
    // Network::SendListener Interface
    virtual void networkReadyToSend(const ServerID& from);

    // Should always be happening inside ServerMessageQueue thread

    // ServerMessageReceiver Protected (Implementation) Interface
    virtual void handleUpdateReceiverStats(ServerID sid, double total_weight, double used_weight);

    // Internal methods
    void addInputQueue(ServerID sid, float weight);
    void removeInputQueue(ServerID sid);

    // Get average weight over all queues.  Used when we don't have a weight for
    // a new input queue yet.
    float getAverageServerWeight() const;

    void enableDownstream(ServerID sid);
    void disableDownstream(ServerID sid);

    // Schedules servicing to occur, but only if it isn't already currently scheduled.
    void scheduleServicing();
    // Services the queue, will be called in response to network ready events
    // and message ready events since one of those conditions must have changed
    // in order to make any additional progress.
    void service();
};

}

#endif
