#include "Network.hpp"
#include "Server.hpp"
#include "Queue.hpp"
#include "LossyQueue.hpp"
#include "FairServerMessageQueue.hpp"
#include "FairObjectMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "Statistics.hpp"
#include "Forwarder.hpp"
#include "ServerProtocolMessagePair.hpp"
namespace CBR{
template <class Queue> FairObjectMessageQueue<Queue>::FairObjectMessageQueue(SpaceContext* ctx, Forwarder* sm, uint32 bytes_per_second)
 : ObjectMessageQueue(ctx, sm),
   mClientQueues( HasDestServerCanSendPredicate(this) ),
   mRate(bytes_per_second),
   mRemainderBytes(0)
{
}

template <class Queue> bool FairObjectMessageQueue<Queue>::beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend &fromBegin)
{
    fromBegin.data = (void*)NULL;
    fromBegin.dest_uuid = msg->dest_object();
    ObjectMessage obj_msg(mContext->id(), *msg);

    ServerProtocolMessagePair* smp = new ServerProtocolMessagePair(NULL,obj_msg, obj_msg.id());
    bool success = mClientQueues.push(msg->source_object(),smp)==QueueEnum::PushSucceeded;

    if (!success)
        delete smp;
    else
        fromBegin.data = (void*)smp;

    return success;
}


template <class Queue> void FairObjectMessageQueue<Queue>::endSend(const ObjMessQBeginSend& fromBegin, ServerID dest_server_id)
{
    ((ServerMessagePair*)fromBegin.data)->dest(dest_server_id);
}





template <class Queue> void FairObjectMessageQueue<Queue>::service(){
    aggregateLocationMessages();

    uint64 bytes = mRate * mContext->sinceLast.toSeconds() + mRemainderBytes;

    ServerProtocolMessagePair* next_msg = NULL;
    UUID objectName;
    unsigned int retryMax=mClientQueues.numQueues(),retryCount=0;
    while( bytes > 0 && (next_msg = mClientQueues.front(&bytes,&objectName)) != NULL ) {
        bool sent_success=true;//FIXME
        /*bool sent_success = */mForwarder->routeObjectMessageToServer(new Protocol::Object::ObjectMessage(next_msg->data().contents),next_msg->dest());
        if (!sent_success)
            break;

        mContext->trace()->serverDatagramQueued(mContext->time, next_msg->dest(), next_msg->id(), next_msg->size());
        ServerProtocolMessagePair* next_msg_popped = mClientQueues.pop(&bytes);
        assert(next_msg_popped == next_msg);
        delete next_msg;
    }

    mRemainderBytes = mClientQueues.empty() ? 0 : bytes;
}

template <class Queue> void FairObjectMessageQueue<Queue>::registerClient(const UUID& sid, float weight) {
   if (!mClientQueues.hasQueue(sid)) {
       mClientQueues.addQueue(new Queue( GetOption(OBJECT_QUEUE_LENGTH)->as<uint32>()),sid,weight);
   }
}
template <class Queue> void FairObjectMessageQueue<Queue>::unregisterClient(const UUID& sid) {
    mClientQueues.removeQueue(sid);
}
template class FairObjectMessageQueue<PartiallyOrderedList<ServerProtocolMessagePair*,ServerID> >;
template class FairObjectMessageQueue<Queue<ServerProtocolMessagePair*> >;
template class FairObjectMessageQueue<LossyQueue<ServerProtocolMessagePair*> >;
}
