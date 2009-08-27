#ifndef _CBR_FAIROBJECTMESSAGEQUEUE_HPP
#define _CBR_FAIROBJECTMESSAGEQUEUE_HPP
#include "ObjectMessageQueue.hpp"
#include "ServerMessageQueue.hpp"
#include "PartiallyOrderedList.hpp"
namespace CBR {
namespace FairObjectMessageNamespace {
    class ServerMessagePair {
    private:
        std::pair<ServerID,Network::Chunk> mPair;
        UniqueMessageID mID;
    public:
        ServerMessagePair(const ServerID&sid, const Network::Chunk&data,UniqueMessageID id):mPair(sid,data),mID(id){}
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
}

template <class TQueue=Queue<FairObjectMessageNamespace::ServerMessagePair*> >
class FairObjectMessageQueue : public ObjectMessageQueue {
protected:
    typedef FairObjectMessageNamespace::ServerMessagePair ServerMessagePair;
    FairQueue<ServerMessagePair,UUID,TQueue > mClientQueues;
    Time mLastTime;
    uint32 mRate;
    uint32 mRemainderBytes;
public:

    FairObjectMessageQueue(ServerMessageQueue* sm, ObjectSegmentation* cseg, uint32 bytes_per_second, Trace* trace);

    virtual void registerClient(const UUID& oid,float weight);
    virtual void unregisterClient(const UUID& oid);

  //  virtual bool send(CBR::Protocol::Object::ObjectMessage* msg);
    virtual bool beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend& );
    virtual bool endSend(const ObjMessQBeginSend&, ServerID dest_server_id);

  
    virtual void service(const Time&t);

protected:
    virtual void aggregateLocationMessages() { }

};
}
#endif
