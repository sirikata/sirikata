#ifndef _CBR_FIFOSENDQUEUE_HPP
#define _CBR_FIFOSENDQUEUE_HPP
#include "SendQueue.hpp"
namespace CBR {
class FIFOSendQueue:public SendQueue {
    std::queue<std::pair<ServerID,Network::Chunk> > mQueue;
    Network * mNetwork;
    uint32 mRate;
    uint32 mRemainderBytes;
    Time mLastTime;
public:
    FIFOSendQueue(Network* net, uint32 bytes_per_second);
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg);
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj);
    virtual void service(const Time& t);
};
}
#endif
