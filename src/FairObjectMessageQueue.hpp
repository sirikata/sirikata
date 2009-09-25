#ifndef _CBR_FAIROBJECTMESSAGEQUEUE_HPP
#define _CBR_FAIROBJECTMESSAGEQUEUE_HPP
#include "ObjectMessageQueue.hpp"
#include "ServerMessageQueue.hpp"
#include "PartiallyOrderedList.hpp"
#include "ServerProtocolMessagePair.hpp"
namespace CBR {

template <class TQueue=Queue<ObjectMessageQueue::ServerMessagePair*> >
class FairObjectMessageQueue : public ObjectMessageQueue {
protected:
    /** Predicate for FairQueue which checks if the network will be able to send the message. */
    struct HasDestServerCanSendPredicate {
    public:
        HasDestServerCanSendPredicate(ServerMessageQueue* _fq) : fq(_fq) {}
        bool operator()(const UUID& key, const typename TQueue::Type msg) const;
    private:
        ServerMessageQueue* fq;
    };

    FairQueue<ServerProtocolMessagePair,UUID,TQueue, HasDestServerCanSendPredicate > mClientQueues;
    uint32 mRate;
    uint32 mRemainderBytes;
    OutgoingMessage*mFront;
    void getFront() const {
        uint64 bytes;
        UUID keyfront;
        ServerProtocolMessagePair *mp =const_cast<FairObjectMessageQueue<TQueue>*>(this)->mClientQueues.front(&bytes,&keyfront);
        Network::Chunk s;

        
        mp->data().serialize(s,0);                             
        const_cast<FairObjectMessageQueue<TQueue>*>(this)->mFront= new OutgoingMessage(s,mp->dest());
    }
public:
    typedef OutgoingMessage*ElementType;
    ElementType& front() {
        if (mFront) {return mFront;}
        getFront();
        return mFront;
    }
    OutgoingMessage* pop() {
        if (mFront) {return mFront;}
        getFront();
        uint64 bytes;
        mClientQueues.pop(&bytes);
        OutgoingMessage*front=mFront;
        mFront=NULL;
        return front;
    }
    const ElementType& front()const {
        if (mFront) {return mFront;}
        getFront();
        return mFront;
    }
    QueueEnum::PushResult push(const ElementType&msg) {
        
        NOT_IMPLEMENTED();
        return QueueEnum::PushExceededMaximumSize;
    }
    bool empty()const {
        return mClientQueues.empty();
    }
    FairObjectMessageQueue(SpaceContext* ctx, Forwarder* sm, uint32 bytes_per_second, ServerMessageQueue *smq);

    virtual void registerClient(const UUID& oid,float weight);
    virtual void unregisterClient(const UUID& oid);

    virtual bool beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend& );
    virtual void endSend(const ObjMessQBeginSend&, ServerID dest_server_id);


    virtual void service();

protected:
    virtual void aggregateLocationMessages() { }

};
}
#endif
