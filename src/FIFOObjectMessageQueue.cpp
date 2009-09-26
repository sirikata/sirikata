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

FIFOObjectMessageQueue::FIFOObjectMessageQueue(SpaceContext* ctx, Forwarder* sm)
 : ObjectMessageQueue(ctx, sm),
   mFrontInput(NULL),
   mFrontOutput(NULL),
   mQueue(GetOption(OBJECT_QUEUE_LENGTH)->as<uint32>() * 32) // FIXME * numObjects?
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

bool FIFOObjectMessageQueue::hasClient(const UUID&) const {
    return true;
}

void FIFOObjectMessageQueue::registerClient(const UUID& sid, float weight) {
}

void FIFOObjectMessageQueue::unregisterClient(const UUID& sid) {
}


}
