#ifndef _CBR_FAIRSENDQUEUE_HPP
#define _CBR_FAIRSENDQUEUE_HPP

#include "FairQueue.hpp"
#include "ServerMessageQueue.hpp"
#include "NetworkQueueWrapper.hpp"

namespace CBR {
class FairServerMessageQueue:public ServerMessageQueue {
protected:

    FairQueue<Message, ServerID, Queue<Message*> > mServerQueues;
    FairQueue<Message, ServerID, NetworkQueueWrapper > mReceiveQueues;

    uint32 mRate;
    uint32 mRecvRate;
    uint32 mRemainderSendBytes;
    uint32 mRemainderReceiveBytes;
    Time mLastSendEndTime; // last packet send finish time, if there are still messages waiting
    Time mLastReceiveEndTime; // last packet receive finish time, if there are still messages waiting

    typedef std::set<ServerID> ReceiveServerSet;
    ReceiveServerSet mReceiveSet;
    std::queue<Message*> mReceiveQueue;
public:

    FairServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, uint32 send_bytes_per_second, uint32 recv_bytes_per_second);

    virtual bool addMessage(Message* msg);
    virtual bool canAddMessage(const Message* msg);
    virtual bool receive(Message** msg_out);
    virtual void service();

    virtual void setServerWeight(ServerID, float weight);

    virtual void reportQueueInfo(const Time& t) const;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const;

    virtual KnownServerIterator knownServersBegin();
    virtual KnownServerIterator knownServersEnd();

protected:
    float getServerWeight(ServerID);

    virtual void aggregateLocationMessages() { }
};
}
#endif
