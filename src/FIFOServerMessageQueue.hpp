#ifndef _CBR_FIFOSENDQUEUE_HPP
#define _CBR_FIFOSENDQUEUE_HPP

#include "FIFOQueue.hpp"
#include "ServerMessageQueue.hpp"
#include "ServerMessagePair.hpp"
#include "FairQueue.hpp"
#include "NetworkQueueWrapper.hpp"

namespace CBR {
class FIFOServerMessageQueue:public ServerMessageQueue {

    FIFOQueue<ServerMessagePair, ServerID> mQueue;
    // It seems weird that we're using a FairQueue, but we do so to split bandwidth evenly.
    // Doing round robin would be an alternative.
    FairQueue<ServerMessagePair, ServerID, NetworkQueueWrapper > mReceiveQueues;

    uint32 mSendRate;
    uint32 mRecvRate;
    uint32 mRemainderSendBytes;
    uint32 mRemainderRecvBytes;
    Time mLastSendEndTime; // the time at which the last send ended, if there are messages that are too big left in the queue
    Time mLastReceiveEndTime; // the time at which the last receive ended, if there are messages that are too big left in the queue

    typedef std::set<ServerID> ReceiveServerList;
    ReceiveServerList mSourceServers;
    struct ChunkSourcePair {
        Network::Chunk* chunk;
        ServerID source;
    };
    std::queue<ChunkSourcePair> mReceiveQueue;
public:
    FIFOServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, uint32 send_bytes_per_second, uint32 recv_bytes_per_second);

    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg);
    virtual bool receive(Network::Chunk** chunk_out, ServerID* source_server_out);
    virtual void service();

    virtual void setServerWeight(ServerID sid, float weight);

    virtual void reportQueueInfo(const Time& t) const;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const;
};
}
#endif
