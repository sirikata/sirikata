#ifndef _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "Statistics.hpp"
#include "ObjectMessageQueue.hpp"

namespace CBR{
class FIFOObjectMessageQueue:public ObjectMessageQueue {
public:
    FIFOObjectMessageQueue(ServerMessageQueue* sm, LocationService* loc, CoordinateSegmentation* cseg, Trace* trace)
     : ObjectMessageQueue(sm, loc, cseg, trace)
    {}

    virtual ~FIFOObjectMessageQueue(){}
    virtual bool send(ObjectToObjectMessage* msg);
    virtual void service(const Time& t);

    virtual void registerClient(UUID oid,float weight=1);

};
}
#endif
