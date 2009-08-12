#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FIFOObjectMessageQueue.hpp"
#include "Message.hpp"
#include "Options.hpp"

namespace CBR {

FIFOObjectMessageQueue::FIFOObjectMessageQueue(ServerMessageQueue* sm, LocationService* loc, CoordinateSegmentation* cseg, uint32 bytes_per_second, Trace* trace)
 : ObjectMessageQueue(sm, loc, cseg, trace),
   mQueue(GetOption(OBJECT_QUEUE_LENGTH)->as<uint32>() * 32), // FIXME * numObjects?
   mLastTime(Time::null()),
   mRate(bytes_per_second),
   mRemainderBytes(0)
{
}

bool FIFOObjectMessageQueue::send(Message* msg, const UUID& source, const UUID& dest) {
    UUID src_uuid = source;
    UUID dest_uuid = dest;
    ServerID dest_server_id = lookup(dest_uuid);
    UniqueMessageID msg_id = msg->id();

    Network::Chunk msg_serialized;
    msg->serialize(msg_serialized, 0);

    ServerMessagePair* smp = new ServerMessagePair(dest_server_id,msg_serialized,msg_id);
    bool success = mQueue.push(src_uuid,smp)==QueueEnum::PushSucceeded;
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

void FIFOObjectMessageQueue::registerClient(UUID sid, float weight) {

}


}
