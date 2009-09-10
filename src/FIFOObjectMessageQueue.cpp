#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FIFOObjectMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "Statistics.hpp"
#include "Forwarder.hpp"
namespace CBR {

FIFOObjectMessageQueue::FIFOObjectMessageQueue(SpaceContext* ctx, Forwarder* sm, uint32 bytes_per_second)
 : ObjectMessageQueue(ctx, sm),
   mQueue(GetOption(OBJECT_QUEUE_LENGTH)->as<uint32>() * 32), // FIXME * numObjects?
   mRate(bytes_per_second),
   mRemainderBytes(0)
{
}

bool FIFOObjectMessageQueue::beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend & fromBegin)
{
    fromBegin.data = (void*)NULL;
    fromBegin.dest_uuid = msg->dest_object();

    Network::Chunk chunk;
    ObjectMessage obj_msg(mContext->id(), *msg);
    //obj_msg.serialize(chunk, 0);

    ServerProtocolMessagePair* smp = new ServerProtocolMessagePair(NULL, obj_msg, obj_msg.id());
    bool success = mQueue.push(msg->source_object(),smp)==QueueEnum::PushSucceeded;

    if (!success)
        delete smp;
    else
        fromBegin.data = (void*)smp;

    return success;
}

void FIFOObjectMessageQueue::endSend(const ObjMessQBeginSend& fromBegin, ServerID dest_server_id)
{
    ((ServerMessagePair*)fromBegin.data)->dest(dest_server_id);
}






void FIFOObjectMessageQueue::service(){
    uint64 bytes = mRate * mContext->sinceLast.toSeconds() + mRemainderBytes;

    while( bytes > 0) {
        ServerProtocolMessagePair* next_msg = mQueue.front(&bytes);
        if (next_msg == NULL) break;
        if (next_msg->dest() == NullServerID) break; // FIXME head of line blocking...

        /*bool sent_success = */mForwarder->route( new Protocol::Object::ObjectMessage(next_msg->data().contents), next_msg->dest() );
        //if (!sent_success) break;//FIXME

        mContext->trace()->serverDatagramQueued(mContext->time, next_msg->dest(), next_msg->id(), next_msg->size());

        ServerProtocolMessagePair* next_msg_popped = mQueue.pop(&bytes);
        assert(next_msg_popped == next_msg);
        delete next_msg;
    }

    mRemainderBytes = mQueue.empty() ? 0 : bytes;
}

void FIFOObjectMessageQueue::registerClient(const UUID& sid, float weight) {
}

void FIFOObjectMessageQueue::unregisterClient(const UUID& sid) {
}


}
