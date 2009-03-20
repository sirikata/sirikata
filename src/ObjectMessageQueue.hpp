#ifndef _CBR_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "Statistics.hpp"

namespace CBR{
class ServerMessageQueue;
class ObjectMessageQueue {
public:
    ObjectMessageQueue(Trace* trace, ServerMessageQueue*sm)
      : mServerMessageQueue(sm),mTrace(trace)
    {}

    virtual ~ObjectMessageQueue(){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg, const  UUID&src_uuid)=0;
    virtual void service(const Time& t)=0;

    virtual void registerClient(UUID oid,float weight=1) = 0;

protected:
    ServerMessageQueue *mServerMessageQueue;
    Trace* mTrace;
};
}
#endif
