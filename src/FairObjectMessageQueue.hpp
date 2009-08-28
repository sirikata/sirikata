#ifndef _CBR_FAIROBJECTMESSAGEQUEUE_HPP
#define _CBR_FAIROBJECTMESSAGEQUEUE_HPP
#include "ObjectMessageQueue.hpp"
#include "ServerMessageQueue.hpp"
#include "PartiallyOrderedList.hpp"
namespace CBR {

template <class TQueue=Queue<ObjectMessageQueue::ServerMessagePair*> >
class FairObjectMessageQueue : public ObjectMessageQueue {
protected:
    /** Predicate for FairQueue which checks if the network will be able to send the message. */
    struct HasDestServerCanSendPredicate {
    public:
        HasDestServerCanSendPredicate(FairObjectMessageQueue<TQueue>* _fq) : fq(_fq) {}
        bool operator()(const UUID& key, const ServerMessagePair* msg) const {
            return msg->dest() != NullServerID;
            // && fq->canSend(msg);
        }
    private:
        FairObjectMessageQueue<TQueue>* fq;
    };

    FairQueue<ServerMessagePair,UUID,TQueue, HasDestServerCanSendPredicate > mClientQueues;
    Time mLastTime;
    uint32 mRate;
    uint32 mRemainderBytes;
public:

    FairObjectMessageQueue(ServerMessageQueue* sm, Trace* trace, uint32 bytes_per_second);

    virtual void registerClient(const UUID& oid,float weight);
    virtual void unregisterClient(const UUID& oid);

    virtual bool beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend& );
    virtual void endSend(const ObjMessQBeginSend&, ServerID dest_server_id);


    virtual void service(const Time&t);

protected:
    virtual void aggregateLocationMessages() { }

};
}
#endif
