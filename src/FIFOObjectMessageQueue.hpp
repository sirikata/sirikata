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

private:
    // Gross, but we need it to handle const-ness for front()
    FIFOObjectMessageQueue* unconstThis() const {
        return const_cast<FIFOObjectMessageQueue*>(this);
    }

    // We maintain both of these because mQueue.front(bytes,key) could return different values
    // after the server message queue is serviced.  As long as you call front() and pop() without servicing
    // the server message queue, they are guaranteed to be the same, but in order to cache the front() result
    // we need to be able to sanity check this
    ServerProtocolMessagePair* mFrontInput; // The internal message used to generate the current front output
    OutgoingMessage* mFrontOutput; // The outgoing message generated for the object.
    void getFrontInput() const {
        uint64 bytes = 10000000; // We don't care how big it is, the can send predicate will take care of it and we have unlimited bandwidth at this point
        ServerProtocolMessagePair *mp = unconstThis()->mQueue.front(&bytes);

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
            ServerProtocolMessagePair *mp = this->mQueue.pop(&bytes);
            assert(mp == mFrontInput);
            delete mp;
        }


        OutgoingMessage* result = mFrontOutput;

        // No matter what, these should be cleared out at the end of this method
        mFrontInput = NULL;
        mFrontOutput = NULL;

        return result;
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
