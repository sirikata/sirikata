#ifndef _CBR_FAIRSENDQUEUE_HPP
#define _CBR_FAIRSENDQUEUE_HPP

#include "FairQueue.hpp"
#include "ServerMessageQueue.hpp"

namespace CBR {
class FairServerMessageQueue:public ServerMessageQueue {
protected:

    class ServerMessagePair {
    private:
        std::pair<ServerID,Network::Chunk> mPair;
    public:
        ServerMessagePair(const ServerID&sid, const Network::Chunk&data):mPair(sid,data){}
        //destructively modifies the data chunk to quickly place it in the queue
        ServerMessagePair(const ServerID&sid, Network::Chunk&data):mPair(sid,Network::Chunk()){
            mPair.second.swap(data);
        }
        explicit ServerMessagePair(size_t size):mPair(0,Network::Chunk(size)){

        }
        unsigned int size()const {
            return mPair.second.size();
        }
        bool empty() const {
            return size()==0;
        }
        ServerID dest() const {
            return mPair.first;
        }

        const Network::Chunk data() const {
            return mPair.second;
        }
    };

    FairQueue<ServerMessagePair, ServerID, Queue<ServerMessagePair*> > mServerQueues;

    Time mLastTime;
    uint32 mRate;
    uint32 mRecvRate;
    uint32 mRemainderBytes;
    Time mLastSendEndTime; // last packet send finish time, if there are still messages waiting

    ///map from ServerID to RemainderBytes
    class ReceiveServerProperties {
    public:
        int32 mRemainderBytes;
        ReceiveServerProperties(){mRemainderBytes=0;}
    };
    typedef std::map<ServerID,ReceiveServerProperties> ReceiveServerList;
    ReceiveServerList mSourceServers;
    struct ChunkSourcePair {
        Network::Chunk* chunk;
        ServerID source;
    };
    std::queue<ChunkSourcePair> mReceiveQueue;
public:

    FairServerMessageQueue(Network*net, uint32 send_bytes_per_second, uint32 recv_bytes_per_second, bool renormalizeWeights, const ServerID& sid, ServerIDMap* sidmap, Trace* trace);

    void setServerWeight(ServerID, float weight);

    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg);
    virtual bool receive(Network::Chunk** chunk_out, ServerID* source_server_out);
    virtual void service(const Time&t);

protected:
    virtual void aggregateLocationMessages() { }

};
}
#endif
