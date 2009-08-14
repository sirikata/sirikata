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
    virtual bool send(CBR::Protocol::Object::ObjectMessage* msg);
    virtual void service(const Time& t);

    virtual void registerClient(UUID oid,float weight=1);

private:
    class ServerMessagePair {
    private:
        std::pair<ServerID,Network::Chunk> mPair;
        UniqueMessageID mID;
    public:
        ServerMessagePair(const ServerID&sid, const Network::Chunk&data, UniqueMessageID id):mPair(sid,data),mID(id){}
        //destructively modifies the data chunk to quickly place it in the queue
        ServerMessagePair(const ServerID&sid, Network::Chunk&data,UniqueMessageID id):mPair(sid,Network::Chunk()),mID(id){
            mPair.second.swap(data);
        }
        explicit ServerMessagePair(size_t size):mPair(0,Network::Chunk(size)),mID(0){

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

        UniqueMessageID id() const {
            return mID;
        }
    };

    FIFOQueue<ServerMessagePair,UUID> mQueue;
    Time mLastTime;
    uint32 mRate;
    uint32 mRemainderBytes;
};
}
#endif
