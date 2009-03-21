#include "Network.hpp"
#include "Server.hpp"
#include "FairServerMessageQueue.hpp"
#include "FairObjectMessageQueue.hpp"
#include "Message.hpp"

namespace CBR{
FairObjectMessageQueue::FairObjectMessageQueue(uint32 bytes_per_second, Trace *trace, ServerMessageQueue*sm)
    : ObjectMessageQueue(trace,sm),
      mClientQueues(bytes_per_second,0,false)
{
}

bool FairObjectMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj){
    return mClientQueues.queueMessage(src_obj,new ServerMessagePair(destinationServer,msg))==QueueEnum::PushSucceeded;
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
