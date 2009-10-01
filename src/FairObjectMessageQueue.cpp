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

template <class TQueue> bool FairObjectMessageQueue<TQueue> ::HasDestServerCanSendPredicate::operator()(const UUID& key, const typename TQueue::Type msg) const {
    return msg->dest() != NullServerID && fq->canAddMessage(msg->dest(), msg->data().size());
}

template <class Queue> FairObjectMessageQueue<Queue>::FairObjectMessageQueue(SpaceContext* ctx, Forwarder* sm, ServerMessageQueue*smq)
 : ObjectMessageQueue(ctx, sm),
   mClientQueues( HasDestServerCanSendPredicate(smq) ),
   mFrontInput(NULL),
   mFrontOutput(NULL)
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

template <class Queue> void FairObjectMessageQueue<Queue>::service() {
    // Service the fair queue to ensure front() will be setup properly, could be affected by oseg lookups finishing.
    mClientQueues.service();
}

template <class Queue> void FairObjectMessageQueue<Queue>::registerClient(const UUID& sid, float weight) {
   if (!mClientQueues.hasQueue(sid)) {
       mClientQueues.addQueue(new Queue( GetOption(OBJECT_QUEUE_LENGTH)->as<uint32>()),sid,weight);
   }
}
template <class Queue> bool FairObjectMessageQueue<Queue>::hasClient(const UUID&sid) const{
    return mClientQueues.hasQueue(sid);
}

template <class Queue> void FairObjectMessageQueue<Queue>::unregisterClient(const UUID& sid) {
    mClientQueues.removeQueue(sid);
}
template class FairObjectMessageQueue<PartiallyOrderedList<ServerProtocolMessagePair*,ServerID> >;
template class FairObjectMessageQueue<Queue<ServerProtocolMessagePair*> >;
template class FairObjectMessageQueue<LossyQueue<ServerProtocolMessagePair*> >;
}
