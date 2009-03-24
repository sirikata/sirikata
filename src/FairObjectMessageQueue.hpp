#ifndef _CBR_FAIROBJECTMESSAGEQUEUE_HPP
#define _CBR_FAIROBJECTMESSAGEQUEUE_HPP
#include "ObjectMessageQueue.hpp"
#include "ServerMessageQueue.hpp"
namespace CBR {
class FairObjectMessageQueue:public ObjectMessageQueue {
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
    FairQueue<ServerMessagePair,UUID> mClientQueues;
    Network * mNetwork;
public:

    FairObjectMessageQueue(ServerMessageQueue* sm, LocationService* loc, CoordinateSegmentation* cseg, uint32 bytes_per_second, Trace* trace);

    void setServerWeight(ServerID, float weight);
    void removeServer(ServerID);
    void registerClient(UUID,float weight);
    void removeClient(UUID);


    virtual bool send(ObjectToObjectMessage* msg);
    virtual void service(const Time&t);

protected:
    virtual void aggregateLocationMessages() { }

};
}
#endif
