#include "Network.hpp"
#include "Server.hpp"
#include "Queue.hpp"
#include "LossyQueue.hpp"
#include "FairServerMessageQueue.hpp"
#include "FairObjectMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"
namespace CBR{
template <class Queue> FairObjectMessageQueue<Queue>::FairObjectMessageQueue(ServerMessageQueue* sm, LocationService* loc, CoordinateSegmentation* cseg, uint32 bytes_per_second, Trace* trace)
 : ObjectMessageQueue(sm, loc, cseg, trace),
   mClientQueues(0,false),
   mLastTime(0),
   mRate(bytes_per_second),
   mRemainderBytes(0)
{
}

template <class Queue> bool FairObjectMessageQueue<Queue>::send(ObjectToObjectMessage* msg) {
    UUID src_uuid = msg->sourceObject();
    UUID dest_uuid = msg->destObject();
    ServerID dest_server_id = lookup(dest_uuid);
   
    Network::Chunk msg_serialized;
    msg->serialize(msg_serialized, 0);
    if (dest_server_id==mServerMessageQueue->getSourceServer()) {
        static bool isReorder=(GetOption(OBJECT_QUEUE)->as<String>()!="fairfifo");
        if (!isReorder)
            return mServerMessageQueue->addMessage(dest_server_id,msg_serialized);
    }
    return mClientQueues.push(src_uuid,new ServerMessagePair(dest_server_id,msg_serialized))==QueueEnum::PushSucceeded;
}

template <class Queue> void FairObjectMessageQueue<Queue>::service(const Time&t){
    aggregateLocationMessages();

    uint64 bytes = mRate * (t - mLastTime).seconds() + mRemainderBytes;

    ServerMessagePair* next_msg = NULL;
    UUID objectName;
    unsigned int retryMax=mClientQueues.numQueues(),retryCount=0;
    while( bytes > 0 && (next_msg = mClientQueues.front(&bytes,&objectName)) != NULL ) {
        bool sent_success = mServerMessageQueue->addMessage(next_msg->dest(), next_msg->data());
        if (!sent_success) {
            mClientQueues.deprioritize(objectName,.75,0,0,1);
            if (++retryCount>retryMax) {
                break;//potentially need to retry all queues to find the one that can push ok
            }
        }else {
            mClientQueues.reprioritize(objectName,1.0,.0625,0,1);
            ServerMessagePair* next_msg_popped = mClientQueues.pop(&bytes);
            assert(next_msg_popped == next_msg);
            delete next_msg;
        }
    }

    mRemainderBytes = mClientQueues.empty() ? 0 : bytes;
    mLastTime = t;
}

template <class Queue> void FairObjectMessageQueue<Queue>::registerClient(UUID sid, float weight) {
   if (!mClientQueues.hasQueue(sid)) {
       mClientQueues.addQueue(new Queue(1024*1024)/*FIXME*/,sid,weight);
   }
}
template <class Queue> void FairObjectMessageQueue<Queue>::removeClient(UUID sid) {
    mClientQueues.removeQueue(sid);
}
template class FairObjectMessageQueue<PartiallyOrderedList<FairObjectMessageNamespace::ServerMessagePair*,ServerID> >;
template class FairObjectMessageQueue<Queue<FairObjectMessageNamespace::ServerMessagePair*> >;
template class FairObjectMessageQueue<LossyQueue<FairObjectMessageNamespace::ServerMessagePair*> >;
}
