#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FIFOObjectMessageQueue.hpp"
#include "Message.hpp"

namespace CBR {

FIFOObjectMessageQueue::FIFOObjectMessageQueue(ServerMessageQueue* sm, LocationService* loc, CoordinateSegmentation* cseg, uint32 bytes_per_second, Trace* trace)
 : ObjectMessageQueue(sm, loc, cseg, trace),
   mQueue(),
   mLastTime(0),
   mRate(bytes_per_second),
   mRemainderBytes(0)
{
}

bool FIFOObjectMessageQueue::send(ObjectToObjectMessage* msg) {
    UUID src_uuid = msg->sourceObject();
    UUID dest_uuid = msg->destObject();
    ServerID dest_server_id = lookup(dest_uuid);

    Network::Chunk msg_serialized;
    msg->serialize(msg_serialized, 0);

    return mQueue.push(src_uuid, new ServerMessagePair(dest_server_id,msg_serialized))==QueueEnum::PushSucceeded;
}

void FIFOObjectMessageQueue::service(const Time& t){
    uint64 bytes = mRate * (t - mLastTime).seconds() + mRemainderBytes;

    ServerMessagePair* next_msg = NULL;
    while( bytes > 0 && (next_msg = mQueue.front(&bytes)) != NULL ) {
        if (mServerMessageQueue->addMessage(next_msg->dest(), next_msg->data())) {
            ServerMessagePair* next_msg_popped = mQueue.pop(&bytes);
            assert(next_msg_popped == next_msg);
        }
    }

    mRemainderBytes = mQueue.empty() ? 0 : bytes;
    mLastTime = t;
}

void FIFOObjectMessageQueue::registerClient(UUID sid, float weight) {

}


}
