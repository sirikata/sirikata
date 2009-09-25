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

    // Gross, but we need it to handle const-ness for front()
    FairObjectMessageQueue<TQueue>* unconstThis() const {
        return const_cast<FairObjectMessageQueue<TQueue>*>(this);
    }
    // We maintain both of these because mClientQueues.front(bytes,key) could return different values
    // after the server message queue is serviced.  As long as you call front() and pop() without servicing
    // the server message queue, they are guaranteed to be the same, but in order to cache the front() result
    // we need to be able to sanity check this
    ServerProtocolMessagePair* mFrontInput; // The internal message used to generate the current front output
    OutgoingMessage* mFrontOutput; // The outgoing message generated for the object.
    void getFrontInput() const {
        uint64 bytes = 10000000; // We don't care how big it is, the can send predicate will take care of it and we have unlimited bandwidth at this point
        UUID keyfront;
        ServerProtocolMessagePair *mp = unconstThis()->mClientQueues.front(&bytes,&keyfront);

        // Check if cached value is fine
        if (mFrontInput == mp) return;

        // If we have a cache and its wrong, delete the generated output
        if (mFrontOutput != NULL) {
            delete mFrontOutput;
            unconstThis()->mFrontOutput = NULL;
        }

        unconstThis()->mFrontInput = mp;
    }
    void createFrontOutput() const {
        if (mFrontInput == NULL || mFrontOutput != NULL)
            return;

        Network::Chunk s;
        mFrontInput->data().serialize(s,0);
        unconstThis()->mFrontOutput = new OutgoingMessage(s, mFrontInput->dest());
    }

public:
    typedef OutgoingMessage*ElementType;
    ElementType& front() {
        getFrontInput();
        createFrontOutput();
        return mFrontOutput;
    }
    const ElementType& front() const {
        getFrontInput();
        createFrontOutput();
        return mFrontOutput;
    }
    OutgoingMessage* pop() {
        // Might not have called front, force it here
        if (mFrontInput == NULL) {
            this->front();
        }

        // If there is a front item, get it off the internal queue and delete the internal storage
        if (mFrontInput != NULL) {
            assert(mFrontOutput != NULL);
            uint64 bytes = 10000000; // We don't care how big it is, the can send predicate will take care of it and we have unlimited bandwidth at this point
            ServerProtocolMessagePair *mp = this->mClientQueues.pop(&bytes);
            assert(mp == mFrontInput);
            delete mp;
        }


        OutgoingMessage* result = mFrontOutput;

        // No matter what, these should be cleared out at the end of this method
        mFrontInput = NULL;
        mFrontOutput = NULL;

        return result;
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
