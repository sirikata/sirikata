#ifndef _CBR_FIFOSENDQUEUE_HPP
#define _CBR_FIFOSENDQUEUE_HPP

#include "FIFOQueue.hpp"
#include "ServerMessageQueue.hpp"

namespace CBR {
class FIFOServerMessageQueue:public ServerMessageQueue {
    class ServerMessagePair {
    private:
        std::pair<ServerID,Network::Chunk> mPair;
    public:
        ServerMessagePair(const ServerID&sid, const Network::Chunk&data):mPair(sid,data){}
        //destructively modifies the data chunk to quickly place it in the queue
        ServerMessagePair(const ServerID&sid, Network::Chunk&data):mPair(sid,Network::Chunk()){
            mPair.second.swap(data);
        }
        unsigned int size()const {
            return mPair.second.size();
        }

        ServerID dest() const {
            return mPair.first;
        }

        const Network::Chunk data() const {
            return mPair.second;
        }
    };

    FIFOQueue<ServerMessagePair, ServerID> mQueue;
    uint32 mRate;
    uint32 mRemainderBytes;
    Time mLastTime;
    Time mLastSendEndTime; // the time at which the last send ended, if there are messages that are too big left in the queue

    typedef std::set<ServerID> ReceiveServerList;
    ReceiveServerList mSourceServers;
    struct ChunkSourcePair {
        Network::Chunk* chunk;
        ServerID source;
    };
    std::queue<ChunkSourcePair> mReceiveQueue;
public:
    FIFOServerMessageQueue(Network* net, uint32 bytes_per_second, const ServerID& sid, ServerIDMap* sidmap, Trace* trace);
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg);
    virtual bool receive(Network::Chunk** chunk_out, ServerID* source_server_out);
    virtual void service(const Time& t);

    virtual void setServerWeight(ServerID sid, float weight);

    virtual void reportQueueInfo(const Time& t) const;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const;
};
}
#endif
