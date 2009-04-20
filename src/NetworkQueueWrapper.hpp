#ifndef _NETWORK_QUEUE_WRAPPER
#define _NETWORK_QUEUE_WRAPPER
#include "Network.hpp"
namespace CBR {
class NetworkQueueWrapper {
    Network * mNetwork;
    ServerID mServerID;
    uint32 mMaxRecvSize;
    Address4 mServerAddress;
    typedef Network::Chunk Chunk;
public:
    NetworkQueueWrapper(ServerID sid, Network*net,ServerIDMap*idmap) {
        mServerID=sid;
        mNetwork=net;
        mMaxRecvSize=(1<<30);
        mServerAddress=*idmap->lookup(sid);
    }
    ~NetworkQueueWrapper(){}

    QueueEnum::PushResult push(const ServerMessagePair *msg){
        return QueueEnum::PushExceededMaximumSize;
    }
    void deprioritize(){}
    const Chunk* front() const{
        return mNetwork->front(mServerAddress,mMaxRecvSize);
    }

    Chunk* front() {
        return mNetwork->front(mServerAddress,mMaxRecvSize);
    }

    Chunk* pop(){
        return mNetwork->receiveOne(mServerAddress,mMaxRecvSize);
    }

    bool empty() const{
        return mNetwork->front(mServerAddress,mMaxRecvSize)==NULL;
    }

    uint32 size() const {
        return mNetwork->front(mServerAddress,mMaxRecvSize)?mNetwork->front(mServerAddress,mMaxRecvSize)->size():0;
    }

    // Returns the total amount of space that can be allocated for the destination
    uint32 maxSize() const {
        return 0;
    }
};
}

#endif
