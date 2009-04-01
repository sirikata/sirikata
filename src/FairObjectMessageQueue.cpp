#include "Network.hpp"
#include "Server.hpp"
#include "FairServerMessageQueue.hpp"
#include "FairObjectMessageQueue.hpp"
#include "Message.hpp"

namespace CBR{
FairObjectMessageQueue::FairObjectMessageQueue(ServerMessageQueue* sm, LocationService* loc, CoordinateSegmentation* cseg, uint32 bytes_per_second, Trace* trace)
 : ObjectMessageQueue(sm, loc, cseg, trace),
   mClientQueues(0,false),
   mLastTime(0),
   mRate(bytes_per_second),
   mRemainderBytes(0)
{
}

bool FairObjectMessageQueue::send(ObjectToObjectMessage* msg) {
    UUID src_uuid = msg->sourceObject();
    UUID dest_uuid = msg->destObject();
    ServerID dest_server_id = lookup(dest_uuid);

    Network::Chunk msg_serialized;
    msg->serialize(msg_serialized, 0);

    return mClientQueues.push(src_uuid,new ServerMessagePair(dest_server_id,msg_serialized))==QueueEnum::PushSucceeded;
}

void FairObjectMessageQueue::service(const Time&t){
    bool freeClientTicks=true;

    aggregateLocationMessages();

    uint64 bytes = mRate * (t - mLastTime).seconds() + mRemainderBytes;

    ServerMessagePair* next_msg = NULL;
    while( bytes > 0 && (next_msg = mClientQueues.pop(&bytes)) != NULL ) {
        if (!mServerMessageQueue->addMessage(next_msg->dest(), next_msg->data()))
            assert(false&&"out of queue space");
    }

    mRemainderBytes = mClientQueues.empty() ? 0 : bytes;
    mLastTime = t;
}

void FairObjectMessageQueue::registerClient(UUID sid, float weight) {
   if (!mClientQueues.hasQueue(sid)) {
       mClientQueues.addQueue(new Queue<ServerMessagePair*>(1024*1024)/*FIXME*/,sid,weight);
   }
}
void FairObjectMessageQueue::removeClient(UUID sid) {
    mClientQueues.removeQueue(sid);
}

}
