#ifndef _NETWORK_QUEUE_WRAPPER
#define _NETWORK_QUEUE_WRAPPER

#include "Network.hpp"

namespace CBR {
class NetworkQueueWrapper {
    Network * mNetwork;
    ServerID mServerID;
    uint32 mMaxRecvSize;
    Address4 mServerAddress;
    ServerMessagePair* mFront;
    typedef Network::Chunk Chunk;
public:
    typedef ServerMessagePair* ElementType;
    typedef std::tr1::function<void(const ElementType&)> PopCallback;

    NetworkQueueWrapper(ServerID sid, Network*net,ServerIDMap*idmap) {
        mServerID=sid;
        mNetwork=net;
        mMaxRecvSize=(1<<30);
        mServerAddress=*idmap->lookupInternal(sid);
        mFront = NULL;
    }
    ~NetworkQueueWrapper(){}

    QueueEnum::PushResult push(const ServerMessagePair *msg){
        return QueueEnum::PushExceededMaximumSize;
    }

    ServerMessagePair* front() {
        if (mFront == NULL) {
            Chunk* c = mNetwork->front(mServerAddress, mMaxRecvSize);

            if (c != NULL) {
                CBR::Protocol::Server::ServerMessage msg;
                bool parsed = parsePBJMessage(&msg, *c);
                assert( parsed && msg.source_server() == mServerID );
                mFront = new ServerMessagePair(mServerID, *c);
            }
        }

        return mFront;
    }

    ServerMessagePair* pop(){
        Chunk* c = mNetwork->receiveOne(mServerAddress,mMaxRecvSize);

        if (c == NULL) {
            assert(mFront == NULL);
            return NULL;
        }

        ServerMessagePair* result = NULL;
        if (mFront != NULL) {
            result = mFront;
            mFront = NULL;
        }
        else {
            result = new ServerMessagePair(mServerID, *c);
        }

        delete c;
        return result;
    }

    ServerMessagePair* pop(const PopCallback& cb) {
        ServerMessagePair* popped = pop();
        if (cb != 0)
            cb(popped);
        return popped;
    }

    bool empty() const{
        return mNetwork->front(mServerAddress,mMaxRecvSize)==NULL;
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
