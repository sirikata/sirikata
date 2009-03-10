#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOSendQueue.hpp"
#include "Message.hpp"

namespace CBR {

FIFOSendQueue::FIFOSendQueue(Network* net, uint32 bytes_per_second, BandwidthStatistics* bstats)
 : SendQueue(bstats),
   mNetwork(net),
   mRate(bytes_per_second),
   mRemainderBytes(0),
   mLastTime(0)
{
}

bool FIFOSendQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    mQueue.push(std::pair<ServerID,Network::Chunk>(destinationServer,msg));
    return true;
}
bool FIFOSendQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj){
    mQueue.push(std::pair<ServerID,Network::Chunk>(destinationServer,msg));
    return true;
}
void FIFOSendQueue::service(const Time& t){
    Duration sinceLast = t - mLastTime;
    uint32 free_bytes = mRemainderBytes + (uint32)(sinceLast.seconds() * mRate);

    while(!mQueue.empty() && mQueue.front().second.size() <= free_bytes) {
        bool ok=mNetwork->send(mQueue.front().first,mQueue.front().second,false,true,1);
        free_bytes -= mQueue.front().second.size();
        assert(ok&&"Network Send Failed");
        mBandwidthStats->sent( mQueue.front().first, GetMessageUniqueID(mQueue.front().second), mQueue.front().second.size(), t);
        mQueue.pop();
    }

    mRemainderBytes = mQueue.empty() ? 0 : free_bytes;

    mLastTime = t;
}

void FIFOSendQueue::registerServer(ServerID sid, float weight) {

}

void FIFOSendQueue::registerClient(UUID sid, float weight) {

}


}
