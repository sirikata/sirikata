#ifndef _CBR_FAIRSENDQUEUE_HPP
#define _CBR_FAIRSENDQUEUE_HPP

#include "FairQueue.hpp"
#include "ServerMessageQueue.hpp"

namespace CBR {
class FairServerMessageQueue:public ServerMessageQueue {
protected:
    struct SenderAdapterQueue {
        SenderAdapterQueue(Sender* sender, ServerID sid);

        Message* front() {
            return mSender->serverMessageFront(mDestServer);
        }

        Message* pop() {
            return mSender->serverMessagePull(mDestServer);
        }

        bool empty() {
            return front() == NULL;
        }
      private:
        Sender* mSender;
        ServerID mDestServer;
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

  public:
    FairServerMessageQueue(SpaceContext* ctx, Network* net, Sender* sender, ServerWeightCalculator* swc, uint32 send_bytes_per_second);

    virtual void addInputQueue(ServerID sid, float weight);
    virtual void updateInputQueueWeight(ServerID sid, float weight);
    virtual void removeInputQueue(ServerID sid);

  protected:
    virtual void messageReady(ServerID sid);

    virtual void networkReadyToSend(const ServerID& from);

    float getServerWeight(ServerID);

    void enableDownstream(ServerID sid);
    void disableDownstream(ServerID sid);

    // Services the queue, will be called in response to network ready events
    // and message ready events since one of those conditions must have changed
    // in order to make any additional progress.
    void service();
};

}

#endif
