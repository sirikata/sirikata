#ifndef _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "Statistics.hpp"
#include "ObjectMessageQueue.hpp"
#include "FIFOQueue.hpp"

namespace CBR{
class FIFOObjectMessageQueue:public ObjectMessageQueue {
public:

    FIFOObjectMessageQueue(ServerMessageQueue* sm, LocationService* loc, CoordinateSegmentation* cseg, uint32 bytes_per_second, Trace* trace);
    virtual ~FIFOObjectMessageQueue(){}
    virtual bool send(ObjectToObjectMessage* msg);
    virtual void service(const Time& t);

    virtual void registerClient(UUID oid,float weight=1);

private:
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

    FIFOQueue<ServerMessagePair,UUID> mQueue;
    Time mLastTime;
    uint32 mRate;
    uint32 mRemainderBytes;
};
}
#endif
