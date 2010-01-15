#ifndef _CBR_FAIRSENDQUEUE_HPP
#define _CBR_FAIRSENDQUEUE_HPP

#include "FairQueue.hpp"
#include "ServerMessageQueue.hpp"
#include "NetworkQueueWrapper.hpp"

namespace CBR {
class FairServerMessageQueue:public ServerMessageQueue {
protected:
    typedef FairQueue<Message, ServerID, Queue<Message*> > FairSendQueue;
    FairSendQueue mServerQueues;

    Time mLastServiceTime;

    uint32 mRate;
    uint32 mRemainderSendBytes;
    Time mLastSendEndTime; // last packet send finish time, if there are still messages waiting
public:

    FairServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, Listener* listener, uint32 send_bytes_per_second);

    virtual bool addMessage(Message* msg);
    virtual bool canAddMessage(const Message* msg);
    virtual void service();

    virtual void setServerWeight(ServerID, float weight);

    virtual void reportQueueInfo(const Time& t) const;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const;

protected:
    virtual void networkReadyToSend(const Address4& from);

    float getServerWeight(ServerID);

    virtual void aggregateLocationMessages() { }
};
}
#endif
