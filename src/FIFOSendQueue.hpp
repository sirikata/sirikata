#ifndef _CBR_FIFOSENDQUEUE_HPP
#define _CBR_FIFOSENDQUEUE_HPP
#include "SendQueue.hpp"
namespace CBR {
class FIFOSendQueue:public SendQueue {
    std::queue<std::pair<ServerID,Network::Chunk> > mQueue;
    Network * mNetwork;
public:
    FIFOSendQueue(Network*net):mNetwork(net){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg);
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj);
    virtual void service(const Time& t);
};
}
#endif
