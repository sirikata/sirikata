#ifndef _CBR_FIFOSENDQUEUE_HPP
#define _CBR_FIFOSENDQUEUE_HPP
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

    std::queue<ServerMessagePair> mQueue;
    Network * mNetwork;
    uint32 mRate;
    uint32 mRemainderBytes;
    Time mLastTime;
public:
    FIFOServerMessageQueue(Network* net, uint32 bytes_per_second, Trace* trace);
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg);
    virtual Network::Chunk* receive();
    virtual void service(const Time& t);

    virtual void setServerWeight(ServerID sid, float weight);
};
}
#endif
