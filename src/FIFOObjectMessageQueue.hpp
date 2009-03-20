#ifndef _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "Statistics.hpp"
#include "ObjectMessageQueue.hpp"
namespace CBR{
class FIFOObjectMessageQueue:public ObjectMessageQueue {
public:
    FIFOObjectMessageQueue(Trace* trace, ServerMessageQueue*sm)
        : ObjectMessageQueue(trace,sm)
    {}

    virtual ~FIFOObjectMessageQueue(){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg, const UUID&src_obj);
    virtual void service(const Time& t);

    virtual void registerClient(UUID oid,float weight=1);

};
}
#endif
