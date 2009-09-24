#ifndef _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "Statistics.hpp"
#include "ObjectMessageQueue.hpp"
#include "FIFOQueue.hpp"
#include "ServerProtocolMessagePair.hpp"
namespace CBR{
class FIFOObjectMessageQueue:public ObjectMessageQueue {
public:

    FIFOObjectMessageQueue(SpaceContext* ctx, Forwarder* sm, uint32 bytes_per_second);
    virtual ~FIFOObjectMessageQueue(){}

    virtual bool beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend& );
    virtual void endSend(const ObjMessQBeginSend&, ServerID dest_server_id);

    virtual void service();

    virtual void registerClient(const UUID& oid,float weight=1);
    virtual void unregisterClient(const UUID& oid);
    typedef OutgoingMessage*ElementType;
    ElementType mFront;
    void getFront() const {
        uint64 bytes;
        ServerProtocolMessagePair *mp =const_cast<FIFOObjectMessageQueue*>(this)->mQueue.front(&bytes);
        Network::Chunk s;

        
        mp->data().serialize(s,0);                             
        const_cast<FIFOObjectMessageQueue*>(this)->mFront= new OutgoingMessage(s,mp->dest());
    }

    ElementType& front() {
        if (mFront) {return mFront;}
        getFront();
        return mFront;
    }
    OutgoingMessage* pop() {
        if (mFront) {return mFront;}
        getFront();
        uint64 bytes;
        mQueue.pop(&bytes);
        OutgoingMessage*front=mFront;
        mFront=NULL;
        return front;
    }
    const ElementType& front()const {
        if (mFront) {return mFront;}
        getFront();
        return mFront;
    }
    bool empty()const {
        return mQueue.empty();
    }    
    QueueEnum::PushResult push(const ElementType&msg) {
        
        NOT_IMPLEMENTED();
        return QueueEnum::PushExceededMaximumSize;
    }
private:
    FIFOQueue<ServerProtocolMessagePair,UUID> mQueue;
    uint32 mRate;
    uint32 mRemainderBytes;
};
}
#endif
