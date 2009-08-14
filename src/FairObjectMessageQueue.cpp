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
   mClientQueues(),
   mLastTime(Time::null()),
   mRate(bytes_per_second),
   mRemainderBytes(0)
{
}

template <class Queue> bool FairObjectMessageQueue<Queue>::send(CBR::Protocol::Object::ObjectMessage* msg) {
    UUID src_uuid = msg->source_object();
    UUID dest_uuid = msg->dest_object();
    ServerID dest_server_id = lookup(dest_uuid);
    UniqueMessageID msg_id = msg->unique();

    ObjectMessage* obj_msg = new ObjectMessage(mServerMessageQueue->getSourceServer(), *msg);
    Network::Chunk msg_serialized;
    obj_msg->serialize(msg_serialized, 0);

    ServerMessagePair* smp = new ServerMessagePair(dest_server_id,msg_serialized,msg_id);
    bool success = mClientQueues.push(src_uuid,smp)==QueueEnum::PushSucceeded;
    if (!success) delete smp;
    return success;
}

template <class Queue> void FairObjectMessageQueue<Queue>::service(const Time&t){
    aggregateLocationMessages();

    uint64 bytes = mRate * (t - mLastTime).toSeconds() + mRemainderBytes;

    ServerMessagePair* next_msg = NULL;
    UUID objectName;
    unsigned int retryMax=mClientQueues.numQueues(),retryCount=0;
    while( bytes > 0 && (next_msg = mClientQueues.front(&bytes,&objectName)) != NULL ) {
        bool sent_success = mServerMessageQueue->addMessage(next_msg->dest(), next_msg->data());
        if (!sent_success) {
            break;
/*
            mClientQueues.deprioritize(objectName,.75,0,0,1);
            if (++retryCount>retryMax) {
                break;//potentially need to retry all queues to find the one that can push ok
            }
*/
        }else {
            mTrace->serverDatagramQueued(t, next_msg->dest(), next_msg->id(), next_msg->data().size());

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
       mClientQueues.addQueue(new Queue( GetOption(OBJECT_QUEUE_LENGTH)->as<uint32>()),sid,weight);
   }
}
template <class Queue> void FairObjectMessageQueue<Queue>::removeClient(UUID sid) {
    mClientQueues.removeQueue(sid);
}
template class FairObjectMessageQueue<PartiallyOrderedList<FairObjectMessageNamespace::ServerMessagePair*,ServerID> >;
template class FairObjectMessageQueue<Queue<FairObjectMessageNamespace::ServerMessagePair*> >;
template class FairObjectMessageQueue<LossyQueue<FairObjectMessageNamespace::ServerMessagePair*> >;
}
