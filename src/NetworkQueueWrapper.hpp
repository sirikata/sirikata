#ifndef _NETWORK_QUEUE_WRAPPER
#define _NETWORK_QUEUE_WRAPPER
#include "Network.hpp"
#include "ServerNetworkImpl.hpp"
namespace CBR {
class NetworkQueueWrapper {
    Network * mNetwork;
    ServerID mServerID;
    uint32 mMaxRecvSize;
    Address4 mServerAddress;
    ServerMessagePair* mFront;
    typedef Network::Chunk Chunk;
public:
    NetworkQueueWrapper(ServerID sid, Network*net,ServerIDMap*idmap) {
        mServerID=sid;
        mNetwork=net;
        mMaxRecvSize=(1<<30);
        mServerAddress=*idmap->lookup(sid);
        mFront = NULL;
    }
    ~NetworkQueueWrapper(){}

    QueueEnum::PushResult push(const ServerMessagePair *msg){
        return QueueEnum::PushExceededMaximumSize;
    }
    void deprioritize(){}

    ServerMessagePair* front() {
        if (mFront == NULL) {
            Chunk* c = mNetwork->front(mServerAddress, mMaxRecvSize);

            if (c != NULL) {
                uint32 offset = 0;
                ServerMessageHeader hdr = ServerMessageHeader::deserialize(*c, offset);
                assert(hdr.sourceServer() == mServerID);
                Network::Chunk payload;
                payload.insert(payload.begin(), c->begin() + offset, c->end());
                assert( payload.size() == c->size() - offset );

                mFront = new ServerMessagePair(mServerID, payload);
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
