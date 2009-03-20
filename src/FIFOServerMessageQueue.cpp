#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR {

FIFOServerMessageQueue::FIFOServerMessageQueue(Network* net, uint32 bytes_per_second, Trace* trace)
 : ServerMessageQueue(trace),
   mNetwork(net),
   mRate(bytes_per_second),
   mRemainderBytes(0),
   mLastTime(0)
{
}

bool FIFOServerMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    mQueue.push(ServerMessagePair(destinationServer,msg));
    return true;
}
void FIFOServerMessageQueue::service(const Time& t){
    Duration sinceLast = t - mLastTime;
    uint32 free_bytes = mRemainderBytes + (uint32)(sinceLast.seconds() * mRate);

    while(!mQueue.empty() && mQueue.front().data().size() <= free_bytes) {
        bool ok=mNetwork->send(mQueue.front().dest(),mQueue.front().data(),false,true,1);
        free_bytes -= mQueue.front().data().size();
        assert(ok&&"Network Send Failed");
        mTrace->packetSent(t, mQueue.front().dest(), GetMessageUniqueID(mQueue.front().data()), mQueue.front().data().size());
        mQueue.pop();
    }

    mRemainderBytes = mQueue.empty() ? 0 : free_bytes;

    mLastTime = t;
}

void FIFOServerMessageQueue::setServerWeight(ServerID sid, float weight) {
}



}
