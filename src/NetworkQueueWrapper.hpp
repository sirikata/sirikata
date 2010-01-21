#ifndef _NETWORK_QUEUE_WRAPPER
#define _NETWORK_QUEUE_WRAPPER

#include "Network.hpp"
#include "Message.hpp"
#include "Statistics.hpp"

namespace CBR {
class NetworkQueueWrapper {
    Context* mContext;
    Network* mNetwork;
    ServerID mServerID;
    uint32 mMaxRecvSize;
    Message* mFront;
    Trace::MessagePath mPathTag;
    typedef Network::Chunk Chunk;

    Message* parse(Chunk* c) {
        Message* msg = Message::deserialize(*c);

        if (msg == NULL) {
            // FIXME if this happens we're probably going to never remove the chunk from the network...
            SILOG(net,warning,"[NET] Couldn't parse message.");
            return NULL;
        }

        if (msg->source_server() != mServerID) {
            // FIXME if this happens we're probably going to never remove the chunk from the network...
            SILOG(net,warning,"[NET] Message source doesn't match connection's ID");
            delete msg;
            return NULL;
        }

        TIMESTAMP_PAYLOAD(msg, mPathTag);

        return msg;
    }
public:
    typedef Message* ElementType;

    NetworkQueueWrapper(Context* ctx, ServerID sid, Network* net, Trace::MessagePath tag) {
        mContext = ctx;
        mServerID=sid;
        mNetwork=net;
        mMaxRecvSize=(1<<30);
        mFront = NULL;
        mPathTag = tag;
    }
    ~NetworkQueueWrapper(){}

    QueueEnum::PushResult push(const Message *msg){
        return QueueEnum::PushExceededMaximumSize;
    }

    Message* front() {
        if (mFront == NULL) {
            Chunk* c = mNetwork->front(mServerID, mMaxRecvSize);
            if (c != NULL)
                mFront = parse(c);
        }

        return mFront;
    }

    Message* pop(){
        Chunk* c = mNetwork->receiveOne(mServerID, mMaxRecvSize);

        if (c == NULL) {
            assert(mFront == NULL);
            return NULL;
        }

        Message* result = NULL;
        if (mFront != NULL) {
            result = mFront;
            mFront = NULL;
        }
        else {
            result = parse(c);
        }

        delete c;
        return result;
    }

    bool empty() const{
        return mNetwork->front(mServerID, mMaxRecvSize)==NULL;
    }

    uint32 size() const {
        return 0;
        //return mNetwork->front(mServerAddress,mMaxRecvSize)?mNetwork->front(mServerAddress,mMaxRecvSize)->size():0;
    }

    // Returns the total amount of space that can be allocated for the destination
    uint32 maxSize() const {
        return 0;
    }
};
}

#endif
