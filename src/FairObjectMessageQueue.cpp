#include "Network.hpp"
#include "Server.hpp"
#include "FairServerMessageQueue.hpp"
#include "FairObjectMessageQueue.hpp"
#include "Message.hpp"

namespace CBR{
FairObjectMessageQueue::FairObjectMessageQueue(ServerMessageQueue* sm, LocationService* loc, CoordinateSegmentation* cseg, uint32 bytes_per_second, Trace* trace)
 : ObjectMessageQueue(sm, loc, cseg, trace),
   mClientQueues(bytes_per_second,0,false)
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

    std::vector<ServerMessagePair*> finalSendMessages=mClientQueues.tick(t);
    size_t count=0;
    for (count=0;count<finalSendMessages.size();++count) {
        if (mServerMessageQueue->addMessage(finalSendMessages[count]->dest(),
                                            finalSendMessages[count]->data())) {
        }else {
            assert(false&&"out of queue space");
        }
    }
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
