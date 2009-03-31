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

    return mClientQueues.queueMessage(src_uuid,new ServerMessagePair(dest_server_id,msg_serialized))==QueueEnum::PushSucceeded;
}

void FairObjectMessageQueue::service(const Time&t){
    bool freeClientTicks=true;

    aggregateLocationMessages();

    uint64 bytes = mRate * (t - mLastTime).seconds() + mRemainderBytes;
    uint64 leftover_bytes = 0;

    std::vector<ServerMessagePair*> finalSendMessages = mClientQueues.tick(bytes, &leftover_bytes);
    size_t count=0;
    for (count=0;count<finalSendMessages.size();++count) {
        if (mServerMessageQueue->addMessage(finalSendMessages[count]->dest(),
                                            finalSendMessages[count]->data())) {
        }else {
            assert(false&&"out of queue space");
        }
    }

    mRemainderBytes = mClientQueues.empty() ? 0 : leftover_bytes;
    mLastTime = t;
}

void FairObjectMessageQueue::registerClient(UUID sid, float weight) {
   if (!mClientQueues.hasQueue(sid)) {
       mClientQueues.addQueue(new Queue<ServerMessagePair*>(65536),sid,weight);
   }
}
void FairObjectMessageQueue::removeClient(UUID sid) {
    mClientQueues.removeQueue(sid);
}

}
