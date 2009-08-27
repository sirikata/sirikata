#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FIFOObjectMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"

namespace CBR {

FIFOObjectMessageQueue::FIFOObjectMessageQueue(ServerMessageQueue* sm, ObjectSegmentation* oseg, uint32 bytes_per_second, Trace* trace)
 : ObjectMessageQueue(sm, oseg, trace),
   mQueue(GetOption(OBJECT_QUEUE_LENGTH)->as<uint32>() * 32), // FIXME * numObjects?
   mLastTime(Time::null()),
   mRate(bytes_per_second),
   mRemainderBytes(0)
{
}

// bool FIFOObjectMessageQueue::send(CBR::Protocol::Object::ObjectMessage* msg) {
//     UUID src_uuid = msg->source_object();
//     UUID dest_uuid = msg->dest_object();
//     ServerID dest_server_id = lookup(dest_uuid);
//     UniqueMessageID msg_id = msg->unique();

//     ObjectMessage* obj_msg = new ObjectMessage(mServerMessageQueue->getSourceServer(), *msg);
//     Network::Chunk msg_serialized;
//     obj_msg->serialize(msg_serialized, 0);

//     ServerMessagePair* smp = new ServerMessagePair(dest_server_id,msg_serialized,msg_id);
//     bool success = mQueue.push(src_uuid,smp)==QueueEnum::PushSucceeded;
//     if (!success) delete smp;
//     return success;
// }



//template <class Queue> bool FairObjectMessageQueue<Queue>::beginSend(CBR::Protocol::Object::ObjectMessage* msg, &fromBegin)
bool FIFOObjectMessageQueue::beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend & fromBegin)
{
  fromBegin.src_uuid = msg->source_object();
  fromBegin.dest_uuid = msg->dest_object();
  fromBegin.msg_id = msg->unique();

  ObjectMessage* obj_msg = new ObjectMessage(mServerMessageQueue->getSourceServer(), *msg);
  obj_msg->serialize(fromBegin.msg_serialized, 0);

  return true;
}

bool FIFOObjectMessageQueue::endSend(const ObjMessQBeginSend& fromBegin, ServerID dest_server_id)
{
  ServerMessagePair* smp = new ServerMessagePair(dest_server_id,fromBegin.msg_serialized,fromBegin.msg_id);
  bool success = mQueue.push(fromBegin.src_uuid,smp)==QueueEnum::PushSucceeded;
  if (!success) delete smp;
  return success;
}



  

  
void FIFOObjectMessageQueue::service(const Time& t){
    uint64 bytes = mRate * (t - mLastTime).toSeconds() + mRemainderBytes;

    while( bytes > 0) {
        ServerMessagePair* next_msg = mQueue.front(&bytes);
        if (next_msg == NULL) break;

        bool sent_success = mServerMessageQueue->addMessage(next_msg->dest(), next_msg->data());
        if (!sent_success) break;

        mTrace->serverDatagramQueued(t, next_msg->dest(), next_msg->id(), next_msg->data().size());

        ServerMessagePair* next_msg_popped = mQueue.pop(&bytes);
        assert(next_msg_popped == next_msg);
        delete next_msg;
    }

    mRemainderBytes = mQueue.empty() ? 0 : bytes;
    mLastTime = t;
}

void FIFOObjectMessageQueue::registerClient(const UUID& sid, float weight) {
}

void FIFOObjectMessageQueue::unregisterClient(const UUID& sid) {
}


}
