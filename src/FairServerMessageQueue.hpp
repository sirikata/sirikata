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
    std::vector<ServerMessagePair*> mClientServerBuffer;
    FairQueue<ServerMessagePair, ServerID > mServerQueues;
    Network * mNetwork;
public:

    FairServerMessageQueue(Network*net, uint32 bytes_per_second, bool renormalizeWeights, Trace* trace);

    void setServerWeight(ServerID, float weight);
    void removeServer(ServerID);

    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg);
    virtual void service(const Time&t);

protected:
    virtual void aggregateLocationMessages() { }

};
}
#endif
