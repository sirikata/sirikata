#ifndef _NETWORK_QUEUE_WRAPPER
#define _NETWORK_QUEUE_WRAPPER

#include "Network.hpp"
#include "Message.hpp"
#include "Statistics.hpp"

namespace CBR {
class NetworkQueueWrapper {
    Context* mContext;
    Network::ReceiveStream* mReceiveStream;
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

        if (msg->source_server() != mReceiveStream->id()) {
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

    NetworkQueueWrapper(Context* ctx, Network::ReceiveStream* rstrm, Trace::MessagePath tag)
     : mContext(ctx),
       mReceiveStream(rstrm),
       mFront(NULL),
       mPathTag(tag)
    {}

    ~NetworkQueueWrapper(){}

    QueueEnum::PushResult push(const Message *msg){
        return QueueEnum::PushExceededMaximumSize;
    }

    Message* front() {
        if (mFront == NULL) {
            Chunk* c = mReceiveStream->front();
            if (c != NULL)
                mFront = parse(c);
        }

        return mFront;
    }

    Message* pop(){
        Chunk* c = mReceiveStream->pop();

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

    bool empty() const {
        return mReceiveStream->front() == NULL;
    }
};
}

#endif
