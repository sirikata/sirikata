#ifndef _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_FIFO_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "Statistics.hpp"
#include "ObjectMessageQueue.hpp"
#include "FIFOQueue.hpp"

namespace CBR{
class FIFOObjectMessageQueue:public ObjectMessageQueue {
public:

    FIFOObjectMessageQueue(ServerMessageQueue* sm, Trace* trace, uint32 bytes_per_second);
    virtual ~FIFOObjectMessageQueue(){}

    virtual bool beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend& );
    virtual void endSend(const ObjMessQBeginSend&, ServerID dest_server_id);

    virtual void service(const Time& t);

    virtual void registerClient(const UUID& oid,float weight=1);
    virtual void unregisterClient(const UUID& oid);
private:
    FIFOQueue<ServerMessagePair,UUID> mQueue;
    Time mLastTime;
    uint32 mRate;
    uint32 mRemainderBytes;
};
}
#endif
