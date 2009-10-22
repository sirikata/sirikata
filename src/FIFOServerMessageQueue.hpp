#ifndef _CBR_FIFOSENDQUEUE_HPP
#define _CBR_FIFOSENDQUEUE_HPP

#include "FIFOQueue.hpp"
#include "ServerMessageQueue.hpp"
#include "FairQueue.hpp"
#include "NetworkQueueWrapper.hpp"

namespace CBR {
class FIFOServerMessageQueue:public ServerMessageQueue {
    FIFOQueue<Message, ServerID> mQueue;
    // It seems weird that we're using a FairQueue, but we do so to split bandwidth evenly.
    // Doing round robin would be an alternative.
    FairQueue<Message, ServerID, NetworkQueueWrapper > mReceiveQueues;

    uint32 mSendRate;
    uint32 mRecvRate;
    uint32 mRemainderSendBytes;
    uint32 mRemainderRecvBytes;
    Time mLastSendEndTime; // the time at which the last send ended, if there are messages that are too big left in the queue
    Time mLastReceiveEndTime; // the time at which the last receive ended, if there are messages that are too big left in the queue

    typedef std::set<ServerID> ReceiveServerList;
    ReceiveServerList mSourceServers;
    std::queue<Message*> mReceiveQueue;
public:
    FIFOServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, uint32 send_bytes_per_second, uint32 recv_bytes_per_second);

    virtual bool addMessage(Message* msg);
    virtual bool canAddMessage(const Message* msg);
    virtual bool receive(Message** msg_out);
    virtual void service();

    virtual void setServerWeight(ServerID sid, float weight);

    virtual void reportQueueInfo(const Time& t) const;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const;

    virtual KnownServerIterator knownServersBegin();
    virtual KnownServerIterator knownServersEnd();

};
}
#endif
